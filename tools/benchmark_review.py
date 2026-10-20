"""Generate app-oriented handoff files from decoder receipts, not model prose."""
import json
import os
from pathlib import Path

def export_candidates(report,out):
    accepted=[];seen=set()
    lines=['# Open tested candidates in Binary Workbench','',
           'These candidates passed the production decoder. They still need visual review; they are not verified reconstructions of the complete original assets.', '',
           '## Textures','', '| Original file | Offset / surface | Decoded size | Load Recipe |','|---|---|---|---|']
    mesh_lines=[]
    for case in report['cases']:
        if 'facts' not in case:continue
        facts=case['facts'];checks=case.get('baseline',[])+[v for turn in case.get('ai',[]) for v in turn.get('validation',[])]
        for check in checks:
            result=check['result'];proposal=check['proposal']
            if not result.get('accepted'):continue
            settings=proposal['settings'];kind=proposal['kind']
            if kind=='image':key=(facts['sha256'],kind,result['pixelSha256'])
            else:key=(facts['sha256'],kind,settings.get('vertexOffset'),settings.get('indexOffset'),result['vertices'],result['triangles'])
            if key in seen:continue
            seen.add(key);record={'source':facts['name'],'sourceSha256':facts['sha256'],'kind':kind,'settings':settings,'result':result,'reviewStatus':'pending human visual review'}
            accepted.append(record)
            if kind=='image':
                recipe=Path(result['recipe']);relative=os.path.relpath(recipe,out).replace('\\','/')
                lines.append(f"| {facts['name']} | {settings['offset']} / {settings.get('surface',0)} | {result['width']} × {result['height']} | [image.bwir]({relative}) |")
                provenance=recipe.with_name('source-provenance.json')
            else:
                setup=Path(result['settings']);prompt=setup.with_name('mesh-chat-prompt.txt')
                prompt.write_text('With '+facts['name']+' as the active document, fill the 3D settings using set_mesh_settings: '+json.dumps(settings)+'. I will review them and click Render 3D.\n',encoding='utf-8')
                relative=os.path.relpath(prompt,out).replace('\\','/');setup_relative=os.path.relpath(setup,out).replace('\\','/')
                mesh_lines.append(f"- **{facts['name']}**: {result['vertices']} vertices / {result['triangles']} triangles; [Load Mesh Setup JSON]({setup_relative}) ([optional chat prompt]({relative})).")
                provenance=setup.with_name('source-provenance.json')
            provenance.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    lines+=['','Open the original binary first. In **UV/2D > 2D Settings / Image Calculator**, use **Load Recipe**, then **Calculate / Add to Library** (or **Calculate 2D** in chat). No original file is embedded in a recipe.', '', '## Geometry','']+mesh_lines
    lines+=['','Open the named original binary, choose **3D > 3D Settings > Load Mesh Setup...**, select the linked JSON, and verify **Read UVs** / **Read normals** are unchecked for these geometry-only candidates. Then click **Render 3D** in the chat to open the view and decode, or use **Render Current File** in 3D Settings. Direct loading applies exact tested values without asking the model to rewrite them. These are local, untextured geometry tests; node transforms, skins and material assignments have not been imported.',
            '', 'Do not reduce rejected vertex counts merely to pass validation: indices may address vertices outside the truncated region. Proper splitting requires format-aware remapping.']
    (out/'OPEN-IN-WORKBENCH.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    (out/'accepted-candidates.json').write_text(json.dumps(accepted,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
