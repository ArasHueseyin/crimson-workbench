"""Read-only signature observations; never treats reference matches as approval."""
from pathlib import Path
import argparse, bisect, hashlib, json, re, struct
import capstone, pefile

EXPECTED = '57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7'

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    data = a.exe.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED:
        raise SystemExit('Unsupported executable; no observation performed.')
    pe = pefile.PE(data=data, fast_load=True)
    pd = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    functions = sorted(struct.iter_unpack('<III', pe.get_data(pd.VirtualAddress, pd.Size)))
    starts = [f[0] for f in functions]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    a.output.mkdir(parents=True, exist_ok=True)
    observations = []
    names = ['kSig_InvGetHolder', 'kSig_InvHolderInsert', 'kSig_InvCommit',
             'kSig_TrItemValueCtor', 'kSig_InvCommitPlacement', 'kSig_InvFreePlacements',
             'kSig_TrItemValueDtor', 'kSig_MoveUpdate', 'kSig_InvCoreGlobal']
    source = a.reference.read_text(encoding='utf-8-sig')
    for name in names:
        m = re.search(r'\b' + name + r'\s*=\s*((?:"[^"]*"\s*)+);', source)
        if not m:
            raise SystemExit('Missing reference signature: ' + name)
        tokens = ''.join(re.findall(r'"([^"]*)"', m[1])).split()
        pattern = b''.join(b'.' if t == '?' else re.escape(bytes.fromhex(t)) for t in tokens)
        matches = []
        for sec in pe.sections:
            if not sec.Characteristics & 0x20000000:
                continue
            for hit in re.finditer(pattern, sec.get_data(), re.S):
                rva = sec.VirtualAddress + hit.start()
                i = bisect.bisect_right(starts, rva) - 1
                f = functions[i] if i >= 0 and functions[i][0] <= rva < functions[i][1] else None
                entry = {'rva': hex(rva), 'function': list(map(hex, f)) if f else None}
                if f:
                    blob = pe.get_data(f[0], f[1] - f[0])
                    entry['sha256'] = hashlib.sha256(blob).hexdigest()
                    listing = '\n'.join(f'{ins.address:08x} {ins.bytes.hex():24} {ins.mnemonic:8} {ins.op_str}' for ins in md.disasm(blob, f[0]))
                    (a.output / f'{name}-{rva:x}.txt').write_text(listing + '\n')
                matches.append(entry)
        observations.append({'name': name, 'matches': matches})
    report = {'exe_sha256': digest, 'timestamp': pe.FILE_HEADER.TimeDateStamp,
              'image_size': pe.OPTIONAL_HEADER.SizeOfImage, 'game_access': False,
              'approved_runtime_binding': False, 'observations': observations}
    (a.output / 'observations.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
