# Theme editor

Open **Other Tools > Settings > Appearance > Themes**. Dark, Light and System are included and read-only. Use **New** or **Duplicate** to create a named custom theme, then choose a base palette.

The editor groups colors under **Backgrounds**, **Text**, **Accent & Highlights**, **Borders & Separators** and **Tool Colors**. Each element has a color swatch and **Use theme default**. A default follows the base/general palette where applicable; custom colors override only that element. **Reset Group** clears the overrides in the selected group.

Backgrounds includes an option to remove input-field backgrounds. Borders & Separators includes an option to hide control borders. Focus indicators remain available for keyboard use. The live preview includes controls, hex values, geometry, scripting and calculator samples.

Tool colors currently cover hex bytes, decoded text, offsets and modified bytes; 3D/UV background, grid, geometry, vertices, wireframe and selection; scripting/console backgrounds and text; and calculator number/function groups. Explicit byte formatting rules and graphics colors chosen directly in a viewer retain priority. Syntax highlighting categories, image-transparency checker colors and result/status-specific color groups are not yet part of this first version.

Edits update the preview only. **Apply** saves the theme library and selection and applies them across the application; **OK** applies and closes. **Cancel** discards edits since the last Apply. **Restore Defaults** selects Dark without deleting user themes; Apply or OK commits the selection. Themes persist in the user profile, independently of application updates.

Use **Rename** and **Delete** for custom themes. **Export** saves one versioned `.bwtheme` JSON file; **Import** validates and copies it into the user's theme library without overwriting an existing theme. Files contain a name, base palette, color overrides and options, without scripts, resource URLs or document paths. Theme files are limited to 64 KiB and opaque RGB colors.
