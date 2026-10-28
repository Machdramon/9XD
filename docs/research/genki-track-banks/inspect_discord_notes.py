from pathlib import Path
import re,html,json
root=Path(r'F:\discord dump\gatos')
rows=[]
for name in ['tokyo-xtreme-racer-3','tokyo-xtreme-racer-drift-2','tokyo-xtreme-racer-drift-1','wangan-midnight','0ivx-script']:
 for p in root.glob('*'+name+'*.html'):
  data=p.read_text(encoding='utf-8')
  for block in re.split(r'<div id=chatlog__message-container-',data)[1:]:
   mid=block.split()[0].split('>')[0];text=html.unescape(re.sub('<[^>]+>',' ',block));text=' '.join(text.split())
   if re.search(r'global|local.*textur|textur.*local|weather|climat|rain|cont1_|cont2_',text,re.I):
    rows.append({'channel':name,'message':mid,'text':text[:2500]})
Path('genki-bank-discord-notes.json').write_text(json.dumps(rows,indent=2,ensure_ascii=False),encoding='utf-8')
print('Relevant messages:',len(rows))
for row in rows:
 if re.search(r'weather|climat|rain|global|cont[12]_',row['text'],re.I): print(row['channel'],row['message'],row['text'][:650])
