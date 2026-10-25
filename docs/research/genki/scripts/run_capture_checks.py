from pathlib import Path
import json,hashlib
from gs_capture_checks import run,source_pictures
root=Path.cwd();checks=Path('capture-checks');checks.mkdir(exist_ok=True)
txrd2=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer Drift 2 (USA)\Tokyo Xtreme Racer - Drift 2 [NTSC-U] [SLUS-21394]\2022')
zero=Path(r'F:\copia de seguridad 08.04.2019\downloads\Tokyo Xtreme Racer - Zero (USA)\Tokyo Xtreme Racer - Zero (USA)\unpack')
c1=Path(r'F:\copia de seguridad 08.04.2019\downloads\Racing Battle C1 Grand Prix [NTSC-J]\Racing Battle C1 Grand Prix [NTSC-J]\Racing Battle C1 Grand Prix\2025')
config={
 'txrd2-march-capture':([txrd2/'000002a0.sdb'],Path('txrd2-march-capture/headerless-bank-audit.json')),
 'txrd2-capture':([txrd2/'00000127.sdb'],Path('txrd2-capture/headerless-bank-audit.json')),
 'txrd2-hakone-capture':([txrd2/(n+'.dat') for n in ['00001701','00001726','00001759','000017b8']],None),
 'txr3-evo5-capture':([Path(r'C:\Users\Jarky\Documents\study cases\txr3\000000c4.nfc')],None),
 'txrd1-alto-capture':([Path(r'C:\Users\Jarky\Documents\study cases\txrd1\00001363.dat')],None),
 'txrd1-hakone-capture':([],None),
 'txrzero-ae86-capture':([zero/'0x00000014.dat'],None),
 'c1-wrx-capture':([Path(r'C:\Users\Jarky\Documents\study cases\c1 ps2\000004b8.dat')],None),
 'c1-nikko-capture':([c1/(n+'.dat') for n in ['00002498','00002532','00002566']],None)}
summary=[]
for folder,(files,header) in config.items():
 try:
  sources=[]
  for f in files:sources+=source_pictures(f,header)
  report=run(Path(folder),sources);report['sourceFileSha256']={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in files};(checks/(folder+'.json')).write_text(json.dumps(report,indent=2),encoding='utf-8');row={k:report[k] for k in ['capture','integrity','gifTags','drawBindingKicks','sourceImages','distinctMatchedSourcePictures','distinctMatchedAtDraw','unsupported']};row['hostTransfers']=len(report['hostTransfers']);row['replayedTransfers']=sum(x['replayed'] for x in report['hostTransfers']);row['distinctDrawMatchesWithStoredPalette']=len({(m['source'],m['pictureOffset']) for m in report['matches'] if m['stage']=='draw-binding' and m['storedPaletteEqual']});summary.append(row);print(json.dumps(row),flush=True)
 except Exception as e:summary.append({'capture':folder,'error':str(e)});print(folder,type(e).__name__,str(e),flush=True)
(checks/'summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
