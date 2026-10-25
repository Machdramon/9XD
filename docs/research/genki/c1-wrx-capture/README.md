# Racing Battle C1 GP - WRX reference

PCSX2 screen explicitly displayed SUBARU IMPREZA SportsWagon WRX in Challenge Mode. Capture serial SLPM-65897. Captured original GS dump, reference PNG, decompressed commands and initial GS state; GIF structural audit passed.

The UI screenshot around capture includes a fading vehicle transition. Do not infer a unique source model from shared Subaru/WRX branding. 000004b8.dat has Subaru/WRX texture logos, but exact SportsWagon versus sedan correspondence has NOT been established. Earlier first-node geometry preview is not sufficient to confirm exact variant. Source texture/geometry matching is pending.

No application changes integrated from this capture. Register binding counts are not unique image counts. This is graphics data, not CPU RAM.

## Source comparison

Compared all 40 bounded TIM2 pictures in study cases/c1 ps2/000004b8.dat against initial captured GS VRAM, using observed TEX0 dimensions/pitch and GS indexed addressing. Eight distinct source picture records have exact indexed-pixel matches; seven also have exact stored RGBA palette matches with the observed CSA slice. Matches include Subaru emblem, headlamp/taillamp artwork, one bumper and IMPREZA plate, not just branding. This is substantial evidence of the shared source texture set; exact visible mesh/upgrade selection and uniqueness across game variants remain unproven. Main body atlas was not matched in this initial snapshot. Chronological upload/CLUT-cache replay remains pending.
