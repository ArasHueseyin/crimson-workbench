import struct
import unittest
from audit_build import candidates


class CandidateTests(unittest.TestCase):
    def test_overlapping_false_opcode_cannot_hide_call(self):
        blob = b"\xe8\x90" + b"\xe8" + struct.pack("<i", 0x2000 - (0x1002 + 5))
        self.assertEqual(list(candidates(blob, 0x1000, {0x2000})), [(0x1002, 0x2000)])

    def test_backward_jump_and_filter(self):
        blob = b"\xe9" + struct.pack("<i", 0x1000 - (0x2000 + 5))
        self.assertEqual(list(candidates(blob, 0x2000, {0x1000})), [(0x2000, 0x1000)])
        self.assertEqual(list(candidates(blob, 0x2000, {0x3000})), [])

    def test_truncated_instruction(self):
        self.assertEqual(list(candidates(b"\xe8\x00\x00\x00", 0x1000, {0x1005})), [])


if __name__ == "__main__":
    unittest.main()
