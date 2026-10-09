"""Read-only RTTI/pdata observation for the updated EXE; never execute game code.

Output is restricted to a project-local research report. This does not admit a
runtime binding. Requires the research environment's pefile and capstone modules.
"""
from pathlib import Path
import hashlib
import json
import mmap
import struct
import sys

import capstone
import pefile


EXPECTED = "a9e5ca2076367e7995b81a3a4803f7259ab7dac3415df8ea949043ef635a174a"
ROOT = Path(__file__).resolve().parents[3]
SOURCE = Path(sys.argv[1]) if len(sys.argv) == 2 else Path(
    "C:/Program Files (x86)/Steam/steamapps/common/Crimson Desert/bin64/CrimsonDesert.exe"
)
TARGET = ROOT / ".local/repair-build25455892-registry-observation.json"
if len(sys.argv) > 2:
    raise RuntimeError("Expected at most one read-only EXE path")
if not TARGET.resolve().is_relative_to(ROOT):
    raise RuntimeError("Research output must remain inside the project")


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


if digest(SOURCE) != EXPECTED:
    raise RuntimeError("Unknown EXE; no native observations recorded")
pe = pefile.PE(str(SOURCE), fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
if base != 0x140000000 or pe.FILE_HEADER.Machine != 0x8664:
    raise RuntimeError("Unexpected PE architecture/base")


with SOURCE.open("rb") as stream, mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
    def raw(rva, count=1):
        for section in pe.sections:
            relative = rva - section.VirtualAddress
            if 0 <= relative and relative + count <= section.SizeOfRawData:
                offset = section.PointerToRawData + relative
                if offset + count <= len(data):
                    return offset
        raise ValueError("RVA outside backed sections")

    def rva(offset):
        for section in pe.sections:
            relative = offset - section.PointerToRawData
            if 0 <= relative < section.SizeOfRawData:
                return section.VirtualAddress + relative
        raise ValueError("Offset outside backed sections")

    def read(address, count):
        offset = raw(address, count)
        return data[offset:offset + count]

    def occurrences(pattern):
        offset = data.find(pattern)
        while offset >= 0:
            yield offset
            offset = data.find(pattern, offset + 1)

    exceptions = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    if exceptions.Size % 12:
        raise RuntimeError("Invalid pdata size")
    pdata = {begin: (end, unwind) for begin, end, unwind in
             struct.iter_unpack("<III", read(exceptions.VirtualAddress, exceptions.Size))}
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    decoder.detail = True

    def function(address):
        if address not in pdata:
            # Leaf functions or thunks need a separate bound; never invent one.
            return {"rva": hex(address), "pdata_start": False}
        end, unwind = pdata[address]
        if not 0 < end - address <= 0x10000:
            raise RuntimeError("Unexpected method size")
        code = read(address, end - address)
        calls = []
        for instruction in decoder.disasm(code, base + address):
            if instruction.mnemonic in ("call", "jmp"):
                operand = instruction.operands[0]
                calls.append({
                    "rva": hex(instruction.address - base),
                    "instruction": instruction.mnemonic + " " + instruction.op_str,
                    "direct_target_rva": hex(operand.imm - base)
                    if operand.type == capstone.x86.X86_OP_IMM else None,
                })
        return {"rva": hex(address), "pdata_start": True, "bytes": len(code),
                "sha256": hashlib.sha256(code).hexdigest(), "unwind_rva": hex(unwind),
                "unwind_header": read(unwind, 4).hex(), "calls": calls}

    def primary_vtable(name):
        matches = []
        for string in occurrences(name.encode("ascii") + b"\0"):
            type_rva = rva(string - 16)
            for type_ref in occurrences(struct.pack("<I", type_rva)):
                if type_ref < 12:
                    continue
                try:
                    col = rva(type_ref - 12)
                    signature, offset, _, type_info, _, self_rva = struct.unpack("<6I", read(col, 24))
                    if signature != 1 or offset != 0 or type_info != type_rva or self_rva != col:
                        continue
                    for table_ref in occurrences(struct.pack("<Q", base + col)):
                        table = rva(table_ref + 8)
                        if table not in matches:
                            matches.append(table)
                except (ValueError, struct.error):
                    continue
        if len(matches) != 1:
            raise RuntimeError(f"Ambiguous/missing primary vtable: {name}: {matches}")
        return matches[0]

    specifications = {
        ".?AVClientActorManager@pa@@": [0x20, 0x28, 0x30],
        ".?AVServerActorManager@pa@@": [0x20, 0x28, 0x30],
        ".?AVCommonActor@pa@@": [8, 0xb8, 0xc0],
        ".?AVServerNormalInGameActor@pa@@": [8, 0xb8, 0xc0],
        ".?AVClientNormalInGameActor@pa@@": [8, 0xb8, 0xc0],
        ".?AVServerUserActor@pa@@": [8, 0xb8, 0xc0],
        ".?AVClientUserActor@pa@@": [8, 0xb8, 0xc0],
        ".?AVWindowsRWLockBase@pa@@": [0x18, 0x20, 0x28],
    }
    tables = []
    for name, slots in specifications.items():
        table = primary_vtable(name)
        methods = {}
        for slot in slots:
            address = struct.unpack("<Q", read(table + slot, 8))[0] - base
            if not any(section.Characteristics & 0x20000000 and
                       section.VirtualAddress <= address < section.VirtualAddress + section.SizeOfRawData
                       for section in pe.sections):
                raise RuntimeError("Vtable method outside executable sections")
            methods[hex(slot)] = function(address)
        tables.append({"name": name, "vtable_rva": hex(table), "methods": methods})

pe.close()
if digest(SOURCE) != EXPECTED:
    raise RuntimeError("EXE changed during observation; result not published")
result = {"exe_version": "1.0.0.2949", "steam_buildid": "25455892", "exe_sha256": EXPECTED,
          "native_execution": False, "runtime_admitted": False, "tables": tables}
TARGET.parent.mkdir(parents=True, exist_ok=True)
TARGET.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"report": str(TARGET), "vtables": len(tables), "native_execution": False}))
