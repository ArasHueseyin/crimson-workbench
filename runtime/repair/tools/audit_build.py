"""Read-only repair call-site audit. Emits metadata, never code or patch files.

Requires pefile and capstone. Output goes to stdout so the caller can keep it
under the ignored .local directory. Never opens a process or save file.
"""
import argparse
import bisect
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct

EXPECTED = "6d348be9d52f81bd35cf7c55e73a5dbfc96cc8268438387c91f7f62c82381fa7"
TARGETS = {
    0x240DFB0: "common_repair",
    0x2BE3EE0: "server_repair",
    0x2411F20: "repair_cost",
}


def candidates(blob, base, targets):
    # A lookahead is required: an opcode-looking byte inside the preceding
    # immediate must not consume and hide the following genuine call opcode.
    for match in re.finditer(rb"(?=([\xe8\xe9][\s\S]{4}))", blob):
        address = base + match.start()
        target = address + 5 + struct.unpack_from("<i", match[1], 1)[0]
        if target in targets:
            yield address, target


def audit(path):
    import capstone
    import pefile

    with path.open("rb") as source:
        digest = hashlib.file_digest(source, "sha256").hexdigest()
        if digest != EXPECTED:
            raise ValueError("Unknown EXE; no address search performed")
        with mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
            pe = pefile.PE(data=data, fast_load=True)
            base = pe.OPTIONAL_HEADER.ImageBase
            if pe.FILE_HEADER.Machine != 0x8664 or base != 0x140000000:
                raise ValueError("Unsupported PE layout")
            pdata = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
            ranges = sorted(struct.iter_unpack("<III", pe.get_data(pdata.VirtualAddress, pdata.Size)))
            starts = [r[0] for r in ranges]
            dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
            cache = {}
            refs = []
            for section in pe.sections:
                if not section.Characteristics & 0x20000000:
                    continue
                blob = data[section.PointerToRawData:section.PointerToRawData + section.SizeOfRawData]
                for address, target in candidates(blob, section.VirtualAddress, TARGETS):
                    index = bisect.bisect_right(starts, address) - 1
                    if index < 0:
                        continue
                    start, end, _ = ranges[index]
                    if not start <= address < end or end - start > 256 * 1024:
                        continue
                    if start not in cache:
                        code = pe.get_data(start, end - start)
                        instructions = {i.address - base: i for i in dis.disasm(code, base + start)}
                        cache[start] = instructions, hashlib.sha256(code).hexdigest()
                    instructions, body_hash = cache[start]
                    instruction = instructions.get(address)
                    if instruction is None or instruction.size != 5 or instruction.mnemonic not in ("call", "jmp"):
                        continue
                    if instruction.op_str != hex(base + target):
                        continue
                    refs.append({
                        "target": TARGETS[target], "rva": hex(address),
                        "function_start_rva": hex(start), "function_end_rva": hex(end),
                        "function_sha256": body_hash,
                        "integer_divisions_in_function": [hex(a) for a, i in instructions.items()
                                                          if i.mnemonic in ("div", "idiv")],
                    })
            pe.parse_data_directories(directories=[1])
            imports = [{"dll": d.dll.decode("ascii"),
                        "symbols": [i.name.decode("ascii") if i.name else i.ordinal for i in d.imports]}
                       for d in pe.DIRECTORY_ENTRY_IMPORT
                       if d.dll.decode("ascii").lower() in ("version.dll", "xinput1_4.dll", "dxgi.dll")]
            pe.close()
        source.seek(0)
        if hashlib.file_digest(source, "sha256").hexdigest() != EXPECTED:
            raise ValueError("EXE changed during audit; result refused")
    return {
        "format": 1, "exe_sha256": digest,
        "scope": "direct E8/E9 references validated on decoded instruction boundaries; excludes indirect calls",
        "direct_reference_count": len(refs), "references": sorted(refs, key=lambda r: int(r["rva"], 16)),
        "loader_candidate_imports": imports, "writes": False, "process_access": False,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.exe), indent=2))
