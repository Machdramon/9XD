#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
// Included dark themes. Same rule for every one: black is the background, the saturated colour is the accent
// (selection, focus, primary actions, 3D selection) and the neutral middle colour carries text, lines and borders.
// Hackerman also sets options.monospace (terminal font for the whole interface).
// They are read-only in the theme editor; duplicating one creates an editable custom copy.
inline const QJsonObject &includedThemes(){static const QJsonObject themes=QJsonDocument::fromJson(R"themes(
{
  "zuz": {
    "format": "binary-workbench-theme", "version": 1, "name": "Zuz", "base": "dark", "options": {},
    "colors": {
      "background.window": "#0b0b0a", "background.surface": "#070707", "background.alternate": "#121211", "background.button": "#171716",
      "text.primary": "#ece9df", "text.secondary": "#8f8d86", "text.disabled": "#5f5e5a", "text.link": "#bcc256",
      "accent.selection": "#4a4f1f", "accent.text": "#f4f1e8", "accent.hover": "#22221f", "accent.focus": "#aab03f", "accent.primary": "#9ea53a",
      "border.color": "#47463f",
      "hex.bytes": "#e4e1d8", "hex.decoded": "#b3b0a7", "hex.offset": "#6f6e69", "hex.modified": "#c7cd52",
      "graphics.background": "#050505", "graphics.grid": "#3d3d39", "graphics.selection": "#c7cd52", "graphics.model": "#d2cec3", "graphics.vertices": "#aab03f", "graphics.wire": "#8f8d86",
      "script.background": "#070707", "script.text": "#ece9df", "console.background": "#070707", "console.text": "#d6d3ca",
      "calculator.digit": "#ece9df", "calculator.hex": "#bcc256", "calculator.operator": "#9c9a93", "calculator.bits": "#bcc256", "calculator.memory": "#8f8d86"
    }
  },
  "nines": {
    "format": "binary-workbench-theme", "version": 1, "name": "Nines", "base": "dark", "options": {},
    "colors": {
      "background.window": "#0a090d", "background.surface": "#060509", "background.alternate": "#110f16", "background.button": "#16141c",
      "text.primary": "#d8dbe2", "text.secondary": "#8d909a", "text.disabled": "#5b5d66", "text.link": "#a98af0",
      "accent.selection": "#38235f", "accent.text": "#f2f3f6", "accent.hover": "#1e1b27", "accent.focus": "#8a62e4", "accent.primary": "#6a40c6",
      "border.color": "#43404f",
      "hex.bytes": "#d8dbe2", "hex.decoded": "#a7aab3", "hex.offset": "#6d6f79", "hex.modified": "#b592ff",
      "graphics.background": "#07060a", "graphics.grid": "#3b3946", "graphics.selection": "#b592ff", "graphics.model": "#c4c8d0", "graphics.vertices": "#9b76f0", "graphics.wire": "#8d909a",
      "script.background": "#060509", "script.text": "#d8dbe2", "console.background": "#060509", "console.text": "#c4c7cf",
      "calculator.digit": "#d8dbe2", "calculator.hex": "#a98af0", "calculator.operator": "#9a9da6", "calculator.bits": "#a98af0", "calculator.memory": "#8d909a"
    }
  },
  "hackerman": {
    "format": "binary-workbench-theme", "version": 1, "name": "Hackerman", "base": "dark", "options": {"monospace": true},
    "colors": {
      "background.window": "#010401", "background.surface": "#000000", "background.alternate": "#03100a", "background.button": "#04170b",
      "text.primary": "#39ff6a", "text.secondary": "#1fa64a", "text.disabled": "#136b30", "text.link": "#8dffad",
      "accent.selection": "#0b4d20", "accent.text": "#c4ffd4", "accent.hover": "#062a13", "accent.focus": "#6cff95", "accent.primary": "#00e64e",
      "border.color": "#0f5a28",
      "hex.bytes": "#39ff6a", "hex.decoded": "#25c957", "hex.offset": "#178a3d", "hex.modified": "#b6ff3c",
      "graphics.background": "#000000", "graphics.grid": "#0c3d1a", "graphics.selection": "#b6ff3c", "graphics.model": "#39ff6a", "graphics.vertices": "#b6ff3c", "graphics.wire": "#1fbf55",
      "script.background": "#000000", "script.text": "#39ff6a", "console.background": "#000000", "console.text": "#39ff6a",
      "calculator.digit": "#39ff6a", "calculator.hex": "#8dffad", "calculator.operator": "#25c957", "calculator.bits": "#8dffad", "calculator.memory": "#1fa64a"
    }
  },
  "aqua": {
    "format": "binary-workbench-theme", "version": 1, "name": "Aqua 2003", "base": "light", "options": {},
    "colors": {
      "background.window": "#e9eef5", "background.surface": "#ffffff", "background.alternate": "#f1f6fd", "background.button": "#f4f7fb",
      "text.primary": "#102040", "text.secondary": "#4b5b75", "text.disabled": "#8a96a8", "text.link": "#0b4fb5",
      "accent.selection": "#b3d1fb", "accent.text": "#0b2f6e", "accent.hover": "#dbe8fb", "accent.focus": "#3d8cf5", "accent.primary": "#2c7ae6",
      "border.color": "#9aaac0",
      "hex.bytes": "#102040", "hex.decoded": "#4b5b75", "hex.offset": "#7a8aa3", "hex.modified": "#c2410c",
      "graphics.background": "#dce7f5", "graphics.grid": "#9fb6d4", "graphics.selection": "#e8711c", "graphics.model": "#3f6fb5", "graphics.vertices": "#e8711c", "graphics.wire": "#2b4a7a",
      "script.background": "#ffffff", "script.text": "#102040", "console.background": "#f7f9fc", "console.text": "#102040",
      "calculator.digit": "#102040", "calculator.hex": "#0b4fb5", "calculator.operator": "#4b5b75", "calculator.bits": "#0b4fb5", "calculator.memory": "#4b5b75",
      "style.bar": "#ece9d8", "style.panelTitle": "#2266d6", "style.progress": "#36b21c"
    }
  },
  "aero": {
    "format": "binary-workbench-theme", "version": 1, "name": "Aero Glass", "base": "dark", "options": {},
    "colors": {
      "background.window": "#0e171f", "background.surface": "#060b10", "background.alternate": "#0b141b", "background.button": "#1a2530",
      "text.primary": "#e8eef4", "text.secondary": "#9db3c4", "text.disabled": "#5f7383", "text.link": "#8fd3ff",
      "accent.selection": "#1d4a6e", "accent.text": "#ffffff", "accent.hover": "#21313e", "accent.focus": "#4fc3ff", "accent.primary": "#2a8ad8",
      "border.color": "#34495a",
      "hex.bytes": "#e8eef4", "hex.decoded": "#9db3c4", "hex.offset": "#5f7a90", "hex.modified": "#7fe0ff",
      "graphics.background": "#04080c", "graphics.grid": "#17344a", "graphics.selection": "#4fc3ff", "graphics.model": "#cfe9ff", "graphics.vertices": "#7fd1ff", "graphics.wire": "#6f8ea8",
      "script.background": "#060b10", "script.text": "#e8eef4", "console.background": "#060b10", "console.text": "#cfe0ee",
      "calculator.digit": "#e8eef4", "calculator.hex": "#8fd3ff", "calculator.operator": "#9db3c4", "calculator.bits": "#8fd3ff", "calculator.memory": "#9db3c4",
      "style.glass": "#0d151c", "style.progress": "#3cb01c"
    }
  },
  "c77-day": {
    "format": "binary-workbench-theme", "version": 1, "name": "C77 Day", "base": "light", "options": {"neon": true, "roundness": 0, "glow": "none"},
    "colors": {
      "background.window": "#f3e600", "background.surface": "#f8ef3c", "background.alternate": "#ebdf00", "background.button": "#e6da00",
      "text.primary": "#0a0a0a", "text.secondary": "#2e2b00", "text.disabled": "#857d00", "text.link": "#006d7a",
      "accent.selection": "#00e5ff", "accent.text": "#000000", "accent.hover": "#ddd100", "accent.focus": "#00b8d4", "accent.primary": "#00e5ff",
      "border.color": "#0a0a0a",
      "hex.bytes": "#0a0a0a", "hex.decoded": "#2e2b00", "hex.offset": "#5e5800", "hex.modified": "#c2185b",
      "graphics.background": "#efe200", "graphics.grid": "#5e5800", "graphics.selection": "#00b8d4", "graphics.model": "#1a1a1a", "graphics.vertices": "#00b8d4", "graphics.wire": "#000000",
      "script.background": "#faf07a", "script.text": "#0a0a0a", "console.background": "#0a0a0a", "console.text": "#f3e600",
      "calculator.digit": "#0a0a0a", "calculator.hex": "#006d7a", "calculator.operator": "#2e2b00", "calculator.bits": "#006d7a", "calculator.memory": "#2e2b00",
      "style.neon": "#00e5ff", "style.panelTitle": "#0a0a0a", "style.progress": "#00e5ff", "style.glass": "#0a0a0a", "style.bar": "#0a0a0a"
    }
  },
  "c77-night-1": {
    "format": "binary-workbench-theme", "version": 1, "name": "C77 Night 1", "base": "dark", "options": {"neon": true, "roundness": 2, "glow": "none"},
    "colors": {
      "background.window": "#2a0407", "background.surface": "#120203", "background.alternate": "#1c0406", "background.button": "#2e0609", "background.gradient": "#000000",
      "text.primary": "#ff5f56", "text.secondary": "#4fe3d0", "text.disabled": "#6b2a2a", "text.link": "#5ffbf1",
      "accent.selection": "#5a0a10", "accent.text": "#ffffff", "accent.hover": "#3d0a0e", "accent.focus": "#ff2a3a", "accent.primary": "#ff1e2d",
      "border.color": "#8a1c22",
      "hex.bytes": "#ff6b63", "hex.decoded": "#4fe3d0", "hex.offset": "#9e3a3a", "hex.modified": "#5ffbf1",
      "graphics.background": "#0a0102", "graphics.grid": "#4a0e12", "graphics.selection": "#5ffbf1", "graphics.model": "#ff5f56", "graphics.vertices": "#5ffbf1", "graphics.wire": "#b0303a",
      "script.background": "#0d0203", "script.text": "#ff7a72", "console.background": "#050102", "console.text": "#4fe3d0",
      "calculator.digit": "#ff5f56", "calculator.hex": "#5ffbf1", "calculator.operator": "#c0484a", "calculator.bits": "#5ffbf1", "calculator.memory": "#c0484a",
      "style.neon": "#ff2a3a", "style.panelTitle": "#ff1e2d", "style.progress": "#ff1e2d", "style.glass": "#1a0305", "style.bar": "#1a0305"
    }
  },
  "c77-night-2": {
    "format": "binary-workbench-theme", "version": 1, "name": "C77 Night 2", "base": "dark", "options": {"neon": true, "roundness": 2, "glow": "none"},
    "colors": {
      "background.window": "#2e2a04", "background.surface": "#0f0e02", "background.alternate": "#1a1803", "background.button": "#2a2705", "background.gradient": "#000000",
      "text.primary": "#f3e600", "text.secondary": "#3fe8ff", "text.disabled": "#6e6a20", "text.link": "#3fe8ff",
      "accent.selection": "#4a4506", "accent.text": "#ffffff", "accent.hover": "#3a3606", "accent.focus": "#fcee0a", "accent.primary": "#fcee0a",
      "border.color": "#8c8414",
      "hex.bytes": "#f3e600", "hex.decoded": "#3fe8ff", "hex.offset": "#8c8414", "hex.modified": "#ff5c8a",
      "graphics.background": "#0a0901", "graphics.grid": "#4a4508", "graphics.selection": "#3fe8ff", "graphics.model": "#f3e600", "graphics.vertices": "#3fe8ff", "graphics.wire": "#b8ae20",
      "script.background": "#0d0c02", "script.text": "#f3e600", "console.background": "#050401", "console.text": "#3fe8ff",
      "calculator.digit": "#f3e600", "calculator.hex": "#3fe8ff", "calculator.operator": "#b8ae20", "calculator.bits": "#3fe8ff", "calculator.memory": "#b8ae20",
      "style.neon": "#fcee0a", "style.panelTitle": "#fcee0a", "style.progress": "#fcee0a", "style.glass": "#1a1803", "style.bar": "#1a1803"
    }
  },
  "emule": {
    "format": "binary-workbench-theme", "version": 1, "name": "eMule", "base": "light", "options": {"roundness": 3, "glow": "none"},
    "colors": {
      "background.window": "#eeece4", "background.surface": "#ffffff", "background.alternate": "#f4f6fb", "background.button": "#f7f6f1",
      "text.primary": "#000000", "text.secondary": "#4a4a4a", "text.disabled": "#9a9a9a", "text.link": "#0033cc",
      "accent.selection": "#316ac5", "accent.text": "#ffffff", "accent.hover": "#dde8fb", "accent.focus": "#0a64d8", "accent.primary": "#1e3cc8",
      "border.color": "#8f9bb3",
      "hex.bytes": "#000000", "hex.decoded": "#1e3cc8", "hex.offset": "#6d6d6d", "hex.modified": "#d42020",
      "graphics.background": "#e9edf5", "graphics.grid": "#9ab0d6", "graphics.selection": "#e03c1e", "graphics.model": "#1e3cc8", "graphics.vertices": "#2ea82e", "graphics.wire": "#102a80",
      "script.background": "#ffffff", "script.text": "#000000", "console.background": "#ffffff", "console.text": "#1e3cc8",
      "calculator.digit": "#000000", "calculator.hex": "#1e3cc8", "calculator.operator": "#d42020", "calculator.bits": "#1e3cc8", "calculator.memory": "#6d6d6d",
      "style.bar": "#ece9d8", "style.panelTitle": "#1e3cc8", "style.progress": "#1e3cc8", "style.glass": "#2a4a8a"
    }
  },
  "ares": {
    "format": "binary-workbench-theme", "version": 1, "name": "Ares", "base": "light", "options": {"roundness": 4, "glow": "none"},
    "colors": {
      "background.window": "#dfe8f3", "background.surface": "#ffffff", "background.alternate": "#f3f6fa", "background.button": "#eef3f9", "background.gradient": "#c9d7e8",
      "text.primary": "#1a1a1a", "text.secondary": "#4b5563", "text.disabled": "#9aa3ae", "text.link": "#1a5fd0",
      "accent.selection": "#cce4f7", "accent.text": "#000000", "accent.hover": "#e5f1fb", "accent.focus": "#3399ff", "accent.primary": "#2d6ff0",
      "border.color": "#a9b8c9",
      "hex.bytes": "#1a1a1a", "hex.decoded": "#2d6ff0", "hex.offset": "#6b7785", "hex.modified": "#c8281e",
      "graphics.background": "#e7edf4", "graphics.grid": "#a9b8c9", "graphics.selection": "#ff8c1a", "graphics.model": "#2d6ff0", "graphics.vertices": "#13a10e", "graphics.wire": "#253f66",
      "script.background": "#ffffff", "script.text": "#1a1a1a", "console.background": "#1e1e1e", "console.text": "#7cd8ff",
      "calculator.digit": "#1a1a1a", "calculator.hex": "#2d6ff0", "calculator.operator": "#c8281e", "calculator.bits": "#2d6ff0", "calculator.memory": "#6b7785",
      "style.bar": "#2b2f36", "style.panelTitle": "#2d6ff0", "style.progress": "#2d6ff0", "style.glass": "#2b2f36"
    }
  },
  "kazaa": {
    "format": "binary-workbench-theme", "version": 1, "name": "Kazaa", "base": "light", "options": {"roundness": 6, "glow": "none"},
    "colors": {
      "background.window": "#e6e8ea", "background.surface": "#ffffff", "background.alternate": "#f2f6ea", "background.button": "#eef0f2",
      "text.primary": "#1c1c1c", "text.secondary": "#4f5a3a", "text.disabled": "#9da3a8", "text.link": "#d46a00",
      "accent.selection": "#b6dc6e", "accent.text": "#1c1c1c", "accent.hover": "#eef6dc", "accent.focus": "#f7941d", "accent.primary": "#7ab800",
      "border.color": "#a3a9ae",
      "hex.bytes": "#1c1c1c", "hex.decoded": "#5a8a00", "hex.offset": "#8a9096", "hex.modified": "#f7941d",
      "graphics.background": "#e4e7ea", "graphics.grid": "#b0b6bb", "graphics.selection": "#f7941d", "graphics.model": "#7ab800", "graphics.vertices": "#f7941d", "graphics.wire": "#3d5c00",
      "script.background": "#ffffff", "script.text": "#1c1c1c", "console.background": "#1c1c1c", "console.text": "#a8e04a",
      "calculator.digit": "#1c1c1c", "calculator.hex": "#5a8a00", "calculator.operator": "#f7941d", "calculator.bits": "#5a8a00", "calculator.memory": "#8a9096",
      "style.bar": "#d9dcdf", "style.panelTitle": "#7ab800", "style.progress": "#7ab800", "style.glass": "#6e757b"
    }
  },
  "limewire": {
    "format": "binary-workbench-theme", "version": 1, "name": "LimeWire", "base": "light", "options": {"roundness": 5, "glow": "none"},
    "colors": {
      "background.window": "#f4f8fd", "background.surface": "#ffffff", "background.alternate": "#eaf2fc", "background.button": "#ffffff",
      "text.primary": "#10233d", "text.secondary": "#3b5a7a", "text.disabled": "#9fb0c3", "text.link": "#1e64c8",
      "accent.selection": "#cfe3fb", "accent.text": "#10233d", "accent.hover": "#e6f0fc", "accent.focus": "#6cbf2a", "accent.primary": "#6cbf2a",
      "border.color": "#a9c3e0",
      "hex.bytes": "#10233d", "hex.decoded": "#3f8f1a", "hex.offset": "#7a91aa", "hex.modified": "#d9a400",
      "graphics.background": "#eef4fb", "graphics.grid": "#b3cbe6", "graphics.selection": "#f5d90a", "graphics.model": "#4c9a1f", "graphics.vertices": "#f5d90a", "graphics.wire": "#1e3c64",
      "script.background": "#ffffff", "script.text": "#10233d", "console.background": "#10233d", "console.text": "#b8f06a",
      "calculator.digit": "#10233d", "calculator.hex": "#3f8f1a", "calculator.operator": "#d9a400", "calculator.bits": "#3f8f1a", "calculator.memory": "#7a91aa",
      "style.bar": "#f8e27a", "style.panelTitle": "#f5c400", "style.progress": "#6cbf2a", "style.glass": "#3b5a7a"
    }
  }
}
)themes").object();return themes;}
// Menu order (keys of includedThemes()).
inline QStringList includedThemeOrder(){return {"zuz","nines","hackerman","aqua","aero","c77-day","c77-night-1","c77-night-2","emule","ares","kazaa","limewire"};}
