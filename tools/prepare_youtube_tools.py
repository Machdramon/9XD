"""Stage optional official YouTube tools for local 9XD development (not bundled).
No PATH changes or browser credentials; each GitHub asset must carry SHA-256.
"""
from pathlib import Path
import hashlib,json,urllib.request,zipfile,subprocess,shutil
ROOT=Path(__file__).resolve().parents[1]/'build/reconstruction-tools/youtube'
def fetch(url):
    return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'9XD-tool-preparation'}),timeout=60)
def asset(repo,name):
    with fetch(f'https://api.github.com/repos/{repo}/releases/latest') as r:release=json.load(r)
    item=next(a for a in release['assets'] if a['name']==name)
    digest=item.get('digest','')
    if not digest.startswith('sha256:'):raise RuntimeError('Missing upstream SHA-256: '+name)
    folder=ROOT/release['tag_name']/repo.split('/')[0];folder.mkdir(parents=True,exist_ok=True)
    dest=folder/name;part=dest.with_suffix(dest.suffix+'.part')
    if not dest.is_file() or hashlib.sha256(dest.read_bytes()).hexdigest()!=digest[7:]:
        with fetch(item['browser_download_url']) as r,part.open('wb') as out:
            size=0
            while chunk:=r.read(1024*1024):
                size+=len(chunk)
                if size>160*1024*1024:raise RuntimeError('Tool exceeds download limit')
                out.write(chunk)
        if hashlib.sha256(part.read_bytes()).hexdigest()!=digest[7:]:raise RuntimeError('Checksum mismatch')
        part.replace(dest)
    return dest,{'repository':repo,'version':release['tag_name'],'url':item['browser_download_url'],'sha256':digest[7:],'bytes':dest.stat().st_size}
def main():
    yt,y=asset('yt-dlp/yt-dlp','yt-dlp.exe')
    archive,d=asset('denoland/deno','deno-x86_64-pc-windows-msvc.zip')
    deno=archive.parent/'deno.exe'
    with zipfile.ZipFile(archive) as z, z.open('deno.exe') as src,deno.open('wb') as out:shutil.copyfileobj(src,out)
    for exe,row in ((yt,y),(deno,d)):
        check=subprocess.run([str(exe),'--version'],capture_output=True,text=True,timeout=30,check=True,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));row['versionOutput']=check.stdout.strip();row['executable']=str(exe)
    receipt={'tools':[y,d],'purpose':'Optional YouTube import; not part of the portable release'}
    ROOT.mkdir(parents=True,exist_ok=True);(ROOT/'receipt.json').write_text(json.dumps(receipt,indent=2),encoding='utf-8');print(json.dumps(receipt,indent=2))
if __name__=='__main__':main()
