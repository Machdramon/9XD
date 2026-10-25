from pathlib import Path
import json,hashlib,struct
root=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]');out=Path('txrd2-march-capture');toc=(root/'BUILD.TOC').read_bytes();entry=json.loads((out/'toc-link-investigation.json').read_text())['ordinalCandidate'];source=root/'2022/000002a0.sdb';fixture=Path(r'C:\Users\Jarky\Documents\study cases\txrd2\car\000002a0.sdb');fresh=out/'provenance-reextract/march-from-build.sdb';digest=lambda b:hashlib.sha256(b).hexdigest();data=fresh.read_bytes();assert data==source.read_bytes()==fixture.read_bytes();assert len(data)==entry['unpackedBytes']
with (root/'BUILD.DAT').open('rb') as f:f.seek(entry['offset']);packed=f.read(entry['packedBytes'])
report={'status':'exact_match','scope':'Archive entry to extracted source to test fixture; not proof of exact runtime geometry/LOD selection','game':'Tokyo Xtreme Racer Drift 2','serial':'SLUS-21394','archive':{'path':str(root/'BUILD.DAT'),'bytes':(root/'BUILD.DAT').stat().st_size},'toc':{'path':str(root/'BUILD.TOC'),'bytes':len(toc),'sha256':digest(toc),'header':list(struct.unpack_from('<4I',toc)),'recordBytes':16,'entryIndexBase':0,'declaredEntries':19869,'allPackedRangesInArchive':True,'extractedOrdinalBase':0,'emittedEntries':19798},'entry':{**entry,'recordHex':toc[entry['tocRecordOffset']:entry['tocRecordOffset']+16].hex(),'packedSha256':digest(packed),'uclMagic':packed[:8].hex()},'extracted':{'source':str(source),'fixture':str(fixture),'reextracted':str(fresh),'bytes':len(data),'sha256':digest(data),'allThreeByteIdentical':True},'verification':{'tool':'User-installed QuickBMS 0.7.6a; own two-line bounded uclpack extraction script','script':'march-provenance.bms','sourceModified':False,'readerIntegrated':False},'captureSha256':json.loads((out/'capture-info.json').read_text())['sha256'],'remaining':['Per-draw material binding and visibility/LOD','Unresolved image and alias slots','Native bank-reader integration owned by Claude']}
(out/'source-provenance.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
(out/'SOURCE-PROVENANCE.md').write_text('''# TXRD2 March — verified source provenance

Updated: 2026-10-25. Status: **exact match** for archive entry → extracted source → test fixture.

- Original archive: BUILD.DAT, 1,789,513,728 bytes; game serial SLUS-21394.
- BUILD.TOC: 317,920 bytes; 19,869 records of 16 bytes after a 16-byte header. This TXRD2 TOC uses 16-byte records, unlike the 20-byte Kaido case.
- Zero-based TOC entry **672**, record byte offset **10,768 (0x2a10)**.
- Sector **89,280**, archive byte offset **182,845,440**; sector size 2,048.
- Compressed length **181,593** bytes; decoded length **626,528** bytes; flag 0.
- Extraction ordinal is also 672 (0x2a0) in this case; this coincidence must not become a general index rule. There are 19,798 emitted records overall because empty records are skipped.
- A bounded two-line research script reextracted only this UCL entry using the already installed QuickBMS. Original archive and samples were not changed.
- Reextracted payload, original `2022/000002a0.sdb`, and `study cases/txrd2/car/000002a0.sdb` are byte-for-byte identical.
- SHA-256 of all three: `fbf9042dde61c5de82e6b458c83d003c7c36fd9433b2d6f425ff99349aa618ed`.

`source-provenance.json` preserves full local paths, TOC hash, raw record, compressed-entry hash and capture hash. `march-provenance.bms` is the bounded extraction recipe. QuickBMS was a research verification tool, not a new 9XD runtime dependency.

The separately recorded 512×512 and 256×256 indexed images and both RGBA palettes match the March capture exactly. This proves the stored texture data was resident, but does not uniquely establish the visible geometry/kit/LOD variant or complete per-draw material bindings. Those tasks remain open. This report hands verified provenance to the next research and native-reader work; it does not declare the reader implemented.
''',encoding='utf-8')
print('Provenance report verified and saved')
