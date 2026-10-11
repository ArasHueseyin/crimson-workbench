"""Read-only PE/type/fingerprint inventory; never resolves or calls game functions."""

import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct


GROUPS = {
    "player_actor": r"UserActor|PlayerActor|PlayerController|Playable|HumanActor|ActorManager",
    "network_messages": r"TrocTr|Network|SyncActor|ClientServer|ServerActor",
    "combat_pvp": r"Damage|Combat|Attack|Aggro|PvP|Pvp|PVP|Team",
    "quest": r"Quest|Mission|StageState|GamePlayTrigger|Cutscene",
    "dungeon_world": r"Dungeon|Instance|FieldGimmick|StageConnect|NavMesh|Navigation|WorldSector|WorldChunk",
}


def pe_sections(data):
    if len(data) < 64 or data[:2] != b"MZ":
        raise ValueError("Not a PE file")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if pe + 24 > len(data) or data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("Invalid PE header")
    machine, count, timestamp = struct.unpack_from("<HHI", data, pe + 4)
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if optional_size < 60 or optional + optional_size > len(data):
        raise ValueError("Invalid optional header")
    if machine != 0x8664 or struct.unpack_from("<H", data, optional)[0] != 0x20B:
        raise ValueError("Expected Windows x64 PE32+")
    table = optional + optional_size
    if not 1 <= count <= 96 or table + count * 40 > len(data):
        raise ValueError("Invalid section table")
    sections = []
    for index in range(count):
        start = table + index * 40
        name = data[start : start + 8].rstrip(b"\0").decode("ascii", errors="replace")
        _, rva, size, offset = struct.unpack_from("<IIII", data, start + 8)
        flags = struct.unpack_from("<I", data, start + 36)[0]
        if offset + size > len(data):
            raise ValueError("Section extends beyond file")
        sections.append({"name": name, "offset": offset, "size": size,
                         "rva": rva, "executable": bool(flags & 0x20000000)})
    return timestamp, struct.unpack_from("<I", data, optional + 56)[0], sections


def compile_fingerprint(pattern):
    tokens = pattern.split()
    if not 8 <= len(tokens) <= 256:
        raise ValueError("Fingerprints must contain 8 to 256 bytes")
    if sum(token not in ("?", "??") for token in tokens) < 4:
        raise ValueError("Fingerprint needs at least four concrete bytes")
    pieces = []
    for token in tokens:
        if token in ("?", "??"):
            pieces.append(b".")
        elif re.fullmatch(r"[0-9a-fA-F]{2}", token):
            pieces.append(re.escape(bytes.fromhex(token)))
        else:
            raise ValueError("Invalid fingerprint token")
    return re.compile(b"".join(pieces), re.DOTALL)


def fingerprint_matches(compiled, data, start, end):
    # Advance one byte after each match to include overlaps. Keeping the
    # literal regex prefix avoids an expensive zero-width scan of large EXEs.
    cursor = start
    for _ in range(4):
        match = compiled.search(data, cursor, end)
        if match is None:
            break
        yield match
        cursor = match.start() + 1


def inventory(path, fingerprints=None):
    with path.open("rb") as source:
        digest = hashlib.file_digest(source, "sha256").hexdigest()
        source.seek(0)
        with mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
            timestamp, image_size, sections = pe_sections(data)
            names = sorted({match.group().rstrip(b"\0").decode("ascii")
                            for match in re.finditer(rb"\.\?A[UV][A-Za-z0-9_?$@]{1,240}@@\x00", data)})
            groups = {}
            for group, keywords in GROUPS.items():
                selected = [name for name in names if re.search(keywords, name)]
                groups[group] = {"count": len(selected), "samples": selected[:12]}
            pattern_results = []
            for candidate in (fingerprints or []):
                compiled = compile_fingerprint(candidate["pattern"])
                matches = []
                for section in sections:
                    if not section["executable"]:
                        continue
                    for match in fingerprint_matches(compiled, data, section["offset"],
                                                      section["offset"] + section["size"]):
                        matches.append({"section": section["name"],
                                        "rva": hex(section["rva"] + match.start() - section["offset"])})
                pattern_results.append({"name": candidate["name"],
                                        "match_count_capped_per_section_at": 4,
                                        "matches": matches,
                                        "unique_byte_candidate": len(matches) == 1,
                                        "runtime_verified": False})
    return {"schema": 1, "read_only": True, "file": path.name, "sha256": digest,
            "pe_timestamp": timestamp, "image_size": image_size, "rtti_type_count": len(names),
            "candidate_groups": groups, "fingerprints": pattern_results,
            "runtime_verified": False,
            "limitations": "Names and byte matches are research leads only; no actor, combat, quest, or map functionality is proven."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("--fingerprints", type=Path, help="Optional local JSON array of name/pattern objects")
    parser.add_argument("--output", type=Path, help="New evidence file; existing files are never overwritten")
    args = parser.parse_args()
    fingerprints = json.loads(args.fingerprints.read_text(encoding="utf-8-sig")) if args.fingerprints else None
    if args.output and args.output.resolve() == args.exe.resolve():
        parser.error("Output must not be the input file")
    result = json.dumps(inventory(args.exe, fingerprints), indent=2)
    if args.output:
        with args.output.open("x", encoding="utf-8") as output:
            output.write(result + "\n")
    else:
        print(result)


if __name__ == "__main__":
    main()
