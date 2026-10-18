# Hex Viewer display controls

- **Bytes per row** opens a numeric dialog (1–256). Changing the row width keeps the current byte selected.
- **Offset Format** selects hexadecimal, decimal or octal for row addresses and unprefixed navigation input. Hexadecimal addresses display `0x`, octal addresses display `0c`, and decimal addresses have no prefix. Explicit prefixes override the selected base.
- Click the **Offset** column header to enter a destination directly. Press Enter to navigate or Escape to cancel. The header follows the current cursor in either the hexadecimal or decoded column.
- **Columns** selects hexadecimal and text, hexadecimal only, or text only. Both use the same viewer font and zoom.
- **Byte Group** adds visual spacing after groups of 1, 2, 4, 8, 16 or 32 bytes. Use **...** for a custom size (1–256). Grouping restarts on each row and never changes byte order or data interpretation.

Display controls belong to each open document. Compare Data and additional advanced-search functionality are separate pending work.

