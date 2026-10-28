# Interface modes — roadmap

Owner: Claude (UI). Started 2026-10-27. Concepts: the "9XD UI Concepts" design canvas (nine boards). Code entry point:
`src/interface_modes.h`.

An **interface mode** is a complete presentation of 9XD, not just a colour theme. Since 2026-10-28 it is three
independent choices that combine freely (Settings › Appearance › Interface mode):

| Axis | Choices | Key |
|---|---|---|
| **Style**: how things are drawn | Classic, Hackerman CRT, Aqua 2003, Aero Glass (planned: Glass Nines, Carbon Pit) | `interface/style` (migrates `interface/mode`) |
| **Panel layout**: how panels are organised | Classic, Inspector stack (planned: Command Deck, Floating HUD, Pipeline) | `interface/layout` |
| **Colours** | `style`: the style's own palette (a normal included theme); `theme`: the user's theme from Themes | `interface/colors` |

Styles never hard-code colours. Every gradient is derived from the palette in use plus four theme colours in the
**Interface Styles** group of the theme editor: `style.bar` (Aqua menu/status bars), `style.glass` (Aero bars),
`style.panelTitle` (Aqua panel titles and rail) and `style.progress` (progress bars). They default to values derived
from the palette; the Aqua 2003 and Aero Glass themes set them. So any theme can colour any style, and a duplicated
theme can retune a style's colours. Picking a theme from the Theme menu while a style is active switches Colours to
"My theme".

The layers a mode touches:

| Layer | What it controls | Where it lives |
|---|---|---|
| Theme | Palette, fonts (e.g. `options.monospace`) | `included_themes.h`, `theme.h` |
| Shell layout | Module rail, terminal status line, later top tabs, dock or pipeline columns | `WorkspaceWindow` (`applyInterfaceMode`) |
| Chrome | How menus, panel titles, corners and buttons are drawn | `interface_modes.h` (`Chrome`), `theme.h` CSS block, `WorkbenchStyle` |
| Effects | Decorative passes: scanline texture, text glow, scan sweep; later glass, vignette, reflections | `theme.h`, `scan_sweep.h`; Vulkan overlay pass (planned) |

## Principles

1. **Classic (0.3.7) is the reference and stays as it is.** It is the default. Every mode feature is off in Classic, and
   switching back to Classic restores the user's theme, panel titles and rail.
2. **Opt-in and reversible.** Modes never touch files, documents or the saved Classic panel layout. A mode that needs a
   different dock arrangement gets its own saved layout key (`layouts/<mode>/…`), never the Classic one.
3. **One registry.** Every mode is listed in `InterfaceModes::all()` with a status: `stable`, `experimental` (selectable)
   or `planned` (visible in Settings but disabled).
4. **Settings own the switches.** Settings › Appearance › Interface mode selects the mode. Each mode's options are stored
   under `interface/<mode>/<key>`, with defaults that give the intended look.
5. **Cost budget.** There is no per-frame repaint of the whole window. Animations only run on small areas and while
   something happens (scan sweep). Textures are palette brushes and are painted only when a widget repaints anyway.
6. **Accessibility.** Text keeps 4.5:1 contrast with every effect on, and all effects can be turned off per mode.

## Technical constraints (read before designing a mode)

- **The Vulkan viewport is a native window.** Qt then makes its ancestors native as well (and their siblings, because
  `AA_DontCreateNativeWidgetSiblings` is not set). A transparent overlay over the whole window is therefore hidden
  behind those native containers, or is drawn black on Windows. Rules:
  - an overlay must be a child of the widget it covers, inside the same native window (the scan sweep works this way:
    it is a child of the hex table viewport);
  - anything drawn over the 3D view (glass, scanlines, vignette, reflections) must be drawn by the Vulkan overlay pass,
    the same way as the corner title and Controls text.
- **Qt stylesheets cannot do** text-transform, text-shadow, backdrop-filter, or per-pixel transparency of child widgets.
  Use these instead:
  - `WorkbenchStyle`, the `QProxyStyle` in `theme.h`, for glow, custom indicators and custom frames;
  - title decoration in code (`InterfaceModes::Chrome`);
  - textured `QBrush` in the palette, which keeps the brush's colour.
- **Real blur (glass)** needs a captured background:
  - for panels over the 3D view, a downsampled copy of the Vulkan frame, blurred in a compute or fragment pass and
    drawn under the panel rectangle;
  - for ordinary widgets, a cached `grab()` of the area beneath, blurred once per layout change. Never per frame.
- **Ninja/MSVC dependencies are unreliable** for some translation units (see COORDINATION-PLAN). After changing a class
  layout in a widely included header, touch the consumers (`magic_panel.cpp`, `format_panel.cpp`, `main.cpp`) or do a
  clean build.

## Shared infrastructure, in build order

| # | Building block | Needed by | Status |
|---|---|---|---|
| I1 | Mode registry, settings page, `interface/mode` and per-mode keys | all | **done** |
| I2 | Chrome CSS block (terminal menus, square corners, status line, gel, glass) and style hooks (glow) | CRT, Aqua, Aero, Carbon | **done** for CRT, Aqua and Aero slice 1 |
| I3 | Panel title decoration (box titles) | CRT | **done** |
| I4 | Shell slots: rail ↔ status line | CRT | **done**. Next: top mode tabs (Command Deck), bottom dock (HUD, sidebar variant C), labelled tree rail (sidebar variant B) |
| I5 | Per-layout saved dock arrangements (`interface/layouts/<layout>/<module>`; Classic stays in `moduleLayouts/v1/<module>`), saved and restored on switch (`WorkspaceWindow::applyPanelLayout`) | Inspector stack, Command Deck, HUD, Pipeline | **done** |
| I6 | Vulkan overlay pass for effects over the viewport: scanlines, vignette, phosphor tint, blurred panel backdrops | CRT slice 2, Glass, Aero, HUD | planned (largest piece) |
| I7 | Custom panel frames (`QDockWidget::setTitleBarWidget` per mode): gel bars, glass bars, box frames | Aqua, Aero, CRT slice 3 | planned |
| I8 | Texture assets (pinstripes, carbon, brushed metal) as palette brushes and border images, generated in code or embedded in resources | Aqua, Carbon | planned |
| I9 | New components: accordion inspector stack, activity timeline, radial menu, snapping HUD cards, mode dock, pipeline columns | Command Deck, HUD, Pipeline | planned |
| I10 | Mode switch tests: round trip Classic → mode → Classic restores the theme, titles, rail and shortcuts; planned modes are not selectable | all | planned |

## Modes

### Hackerman CRT — experimental (first)

Concept board 5. Theme: Hackerman (phosphor green on black, terminal font).

**Slice 1 (done 2026-10-27)**
- Terminal status line `[9XD] 1:hex 2:3d 3:2d 4:recon 5:script 6:tools`, plus document name and size, `ctrl+k` and
  `settings`. It replaces the module rail; Alt+1…6 switch modes.
- Box-drawn panel titles `┤ INSPECTOR ├`. Titles that code changes later are decorated again; Classic restores the originals.
- Terminal menus: inverted selection, 1 px border, square corners on buttons, fields, tabs and cards.
- Scanline texture on window and list backgrounds (strength 0–100 in Settings).
- Phosphor glow on labels, buttons, menus and tabs (`WorkbenchStyle::drawItemText`).
- It inherits the green scan sweep and the Controls overlay.

**Slice 2**
- Vulkan overlay pass (I6): scanlines and vignette over the 3D view, plus a green phosphor tint option for the model shading.
- Box frames around inspector cards (I7) and an ASCII progress bar style for scan progress (`[#####.....]`).

**Slice 3**
- Console-style status messages (timestamped log with typewriter reveal), blinking block cursor in fields and a
  `:command` prompt in the status line that is wired to the command palette.

Options to add: flicker (off by default), curvature (only in the Vulkan pass), amber or white phosphor variants.

### Glass Nines — planned

Concept board 1. The full-window 3D viewport with floating frosted panels.

- Needs I5 (its own layout), I6 (blurred backdrops behind panels over the viewport) and I9 (floating cards).
- Without I6 a fallback is possible: semi-transparent tinted panels without blur.
- Risk: performance of blurring each frame. Blur at ¼ resolution and only when the camera moves.

### Aqua 2003 — experimental

Concept board 2. Light gel style from the early 2000s. Theme: Aqua 2003 (light, blue accent).

**Slice 1 (done 2026-10-28)**, all in the stylesheet and palette (no new widgets):
- Gel buttons: glossy two-tone gradient, rounded 11 px. Primary actions and checked viewport segments are blue gel.
- Pinstripe window background (2 px rows, palette texture). Lists stay white.
- Task-pane look: blue gradient panel titles with white bold text, a blue gradient module rail, and beige menu and
  status bars.
- Block progress bars: green chunks 8 px wide.

Options (Settings › Interface mode › Aqua 2003): gel buttons, pinstripes, task panes.

**Slice 2**
- Custom dock title bars (I7) with a collapse chevron, like XP task panes.
- A "File Tasks" pane: context actions such as Scan, Render All and Export as links.
- Pinstripe toolbars and gel tabs.
- Window-frame colouring on Windows (DWM caption colour API).

### Aero Glass — experimental

Concept board 3. Black glass style from the late 2000s. Theme: Aero Glass (dark blue-black, cyan accent).

**Slice 1 (done 2026-10-28)**, in the stylesheet, palette and style hook:
- Black glass bars: glossy menu bar, toolbars, status bar, panel titles, viewport toolbar and table headers.
- Blue glass highlight on menu items.
- Glass buttons: dark glossy, turning blue on hover. Primary actions are blue glass.
- Shiny green progress bars.
- Soft text glow (the shared `drawItemText` hook).

Options: glass bars, glass buttons, glow.

**Slice 2**
- Real translucency of the window frame on Windows: DWM blur-behind or a Mica/Acrylic backdrop, behind a
  frameless window with a custom caption that has a glowing title and glass caption buttons.
- Diagonal reflection streaks drawn by the style on bars.
- Gadget-style inspector cards (I7).

**Slice 3**
- Glass over the 3D viewport (I6).

### Carbon Pit — planned

Concept board 4. Carbon fibre and brushed aluminium, mode LEDs, gauges for memory budget and scan rate, a
segmented decode bar.

- Needs I8 (carbon and aluminium brushes) and a small gauge widget (a QPainter arc) fed by the existing
  memory budget and batch rate.
- No Vulkan work.

### Inspector stack — experimental (layout)

Concept board 6 (the inspector column of Command Deck). `InterfaceModes::DockStack`.

**Slice 1 (2026-10-28)**
- Each module's side panels (and, by default, bottom panels) are untabified into one column on the right. Work
  surfaces stay where they are: 3D viewport, UV canvas, script editor and console, reconstruction canvases, AI chat.
- Clicking a panel title folds or opens it (`▾` / `▸` before the title; the Windows menu keeps the plain name). Folding
  hides the content and caps the height at the title bar. Dragging the title still moves or floats the panel.
- Options: one panel open at a time (default on); move bottom panels into the stack (default on).
- The arrangement is saved per layout, so Classic comes back untouched. Reset Panel Layout rebuilds the stack.
- Works with every style (box titles, gel and glass title bars are composed with the arrow).

**Next**: summaries in folded titles (e.g. selection offset and value for the Inspector, result count for File Scan),
pinning a panel open, keyboard folding (Alt+click / arrows), then the Command Deck pieces below.

### Command Deck — planned (layout mode)

Concept board 6. Modes as top tabs with counts, a command search centred in the header, inspectors as a
collapsible stack with summaries, and an activity timeline at the bottom.

- Needs I4 (top tabs slot), I5 (its own layout) and I9 (accordion stack that re-hosts existing inspector widgets;
  activity chips fed by scan and render progress).
- The cheapest layout mode, because it re-hosts existing widgets.

### Floating HUD — planned (layout mode)

Concept board 7. Full-screen viewport, snapping HUD cards, a minimap navigator, a radial context menu and a mode dock.

- Needs I5, I6 (if the cards are blurred), I9 (HUD cards as frameless tool windows or Vulkan-drawn cards, a radial
  menu, the dock).
- Risk: floating docks are separate native windows. Keep them tool windows with snapping and save their positions per mode.

### Pipeline — planned (layout mode)

Concept board 8. Source → Detect → Output columns, with a stage rail, entropy strip, confidence bars and inline inspectors.

- Needs I9 (columns that host the hex view, scan results and previews; grouping of results by profile; confidence data
  from the scanners).
- This is the largest change and the most valuable for new users. Plan it after Command Deck.

### Sidebar variants (board 9)

These are mode options rather than separate modes:

| Variant | Description |
|---|---|
| A | Today's rail with a hover flyout (counts and open documents) |
| B | Labelled tree rail |
| C | Bottom dock |
| D | Mode wheel on held Tab |

A and B can become an option of Classic-like modes once I4 exists. Classic itself stays as it is.

## Settings model

| Key | Meaning |
|---|---|
| `interface/style` | `classic` (default) or a style id; falls back to the older `interface/mode` |
| `interface/layout` | `classic` (default) or `inspector-stack` |
| `interface/colors` | `style` (default: the style's palette) or `theme` (the user's theme) |
| `interface/inspector-stack/{exclusive,collectBottom}` | bool, default true |
| `interface/layouts/<layout>/<module>` | saved dock state of a non-Classic layout |
| `interface/hackerman-crt/statusLine` | bool, default true |
| `interface/hackerman-crt/boxTitles` | bool, default true |
| `interface/hackerman-crt/terminalMenus` | bool, default true (also square corners) |
| `interface/hackerman-crt/glow` | bool, default true |
| `interface/hackerman-crt/scanlines` | 0–100, default 35 |
| `interface/aqua-2003/{gel,pinstripes,taskPanes}` | bool, default true |
| `interface/aero-glass/{glassBars,glassButtons,glow}` | bool, default true |
| `interface/detailedTooltips` | global, default false |

Planned shared chrome options, which each mode will expose where they make sense:

- menu style (standard / terminal / glass / gel);
- panel title style (standard / box / gel / glass);
- corner radius (rounded / square);
- density (compact / normal / comfortable);
- effects intensity;
- motion (full / reduced / none).

## Adding a mode: checklist

1. Add it to `InterfaceModes::all()` as `planned`. It appears disabled in Settings.
2. Add its theme to `included_themes.h`, if it has one.
3. Add its options to `chromeFor()` and to the Settings page (a group box shown only for that mode), stored under
   `interface/<id>/…`.
4. Implement each piece behind its chrome flag (CSS block in `theme.h`, style hook, shell slot, effect). Classic must be
   a no-op.
5. Add round-trip tests (I10) and verify live in both a dark and a light Classic theme after switching back.
6. Promote it to `experimental`, then to `stable` once the owner has signed it off.
