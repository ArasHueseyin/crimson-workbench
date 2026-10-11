import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from multiplayer_static_probe import compile_fingerprint, inventory, pe_sections


def synthetic_pe():
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHI", data, 0x84, 0x8664, 2, 123)
    struct.pack_into("<H", data, 0x94, 240)
    struct.pack_into("<H", data, 0x98, 0x20B)
    struct.pack_into("<I", data, 0xD0, 0x3000)
    for offset, name, rva, raw, flags in (
        (0x188, b".text", 0x1000, 0x200, 0x60000020),
        (0x1B0, b".rdata", 0x2000, 0x300, 0x40000040),
    ):
        data[offset:offset + len(name)] = name
        struct.pack_into("<IIII", data, offset + 8, 0x100, rva, 0x100, raw)
        struct.pack_into("<I", data, offset + 36, flags)
    return data


class StaticProbeTests(unittest.TestCase):
    def probe(self, data, patterns=None):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.exe"
            path.write_bytes(data)
            result = inventory(path, patterns)
            self.assertEqual(path.read_bytes(), data, "Source file must remain unchanged")
            self.assertEqual(result["sha256"], hashlib.sha256(data).hexdigest())
            self.assertFalse(result["runtime_verified"])
            return result

    def test_type_names_are_only_candidates(self):
        data = synthetic_pe()
        name = b".?AVQuestManager@pa@@\0"
        data[0x300:0x300 + len(name)] = name
        result = self.probe(data)
        self.assertEqual(result["rtti_type_count"], 1)
        self.assertEqual(result["candidate_groups"]["quest"]["count"], 1)
        self.assertTrue(result["read_only"])

    def test_data_section_match_is_not_a_code_candidate(self):
        data = synthetic_pe()
        data[0x300:0x308] = bytes.fromhex("40 53 56 57 48 83 EC 70")
        result = self.probe(data, [{"name": "data-only", "pattern": "40 53 56 57 48 83 EC 70"}])
        self.assertEqual(result["fingerprints"][0]["matches"], [])

    def test_overlapping_matches_are_not_unique(self):
        data = synthetic_pe()
        data[0x200:0x209] = b"\xAA" * 9
        result = self.probe(data, [{"name": "overlap", "pattern": "AA AA AA AA AA AA AA AA"}])
        self.assertEqual(len(result["fingerprints"][0]["matches"]), 2)
        self.assertFalse(result["fingerprints"][0]["unique_byte_candidate"])

    def test_unique_candidate_is_not_runtime_evidence(self):
        data = synthetic_pe()
        data[0x200:0x208] = bytes.fromhex("40 53 56 57 48 83 EC 70")
        result = self.probe(data, [{"name": "unique", "pattern": "40 53 56 57 48 83 EC 70"}])
        candidate = result["fingerprints"][0]
        self.assertTrue(candidate["unique_byte_candidate"])
        self.assertFalse(candidate["runtime_verified"])
        self.assertEqual(candidate["matches"][0]["rva"], "0x1000")

    def test_wildcards_and_regex_bytes_are_literal(self):
        pattern = compile_fingerprint("2E 5B 5D 7B 7D 2A ? 0A")
        self.assertIsNotNone(pattern.search(b".[]{}*\xFF\n"))
        self.assertIsNone(pattern.search(b"X[]{}*\xFF\n"))

    def test_invalid_or_weak_fingerprints_are_rejected(self):
        for pattern in ("?? ?? ?? ?? ?? ?? ?? ??", "40 53", "40 53 56 57 ZZ 83 EC 70"):
            with self.subTest(pattern=pattern), self.assertRaises(ValueError):
                compile_fingerprint(pattern)

    def test_malformed_pe_is_rejected(self):
        cases = [b"", b"MZ" + b"\0" * 60]
        wrong_machine = synthetic_pe()
        struct.pack_into("<H", wrong_machine, 0x84, 0x14C)
        cases.append(wrong_machine)
        truncated_section = synthetic_pe()
        struct.pack_into("<I", truncated_section, 0x188 + 16, 0x1000)
        cases.append(truncated_section)
        for data in cases:
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                pe_sections(data)


if __name__ == "__main__":
    unittest.main()
