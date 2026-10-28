#pragma once
#include <QPalette>
#include <QJsonObject>
#include <QJsonValue>
#include <QColor>
#include <QStringList>
#include <QRegularExpression>
struct ThemeToken {QString key,label,group;int role=-1;};
inline const QList<ThemeToken> &themeTokens(){static const QList<ThemeToken> tokens={
    {"background.window","Windows & panels","Backgrounds",QPalette::Window},{"background.surface","Editor / list surfaces","Backgrounds",QPalette::Base},{"background.alternate","Alternate rows / surfaces","Backgrounds",QPalette::AlternateBase},{"background.button","Control surfaces","Backgrounds",QPalette::Button},{"background.gradient","Window gradient (fades to)","Backgrounds"},
    {"text.primary","Primary text","Text",QPalette::WindowText},{"text.secondary","Secondary text","Text",QPalette::PlaceholderText},{"text.disabled","Disabled text","Text"},{"text.link","Links","Text",QPalette::Link},
    {"accent.selection","Accent & selection","Accent & Highlights",QPalette::Highlight},{"accent.text","Text on selection","Accent & Highlights",QPalette::HighlightedText},{"accent.hover","Hover surface","Accent & Highlights"},{"accent.focus","Keyboard focus","Accent & Highlights"},{"accent.primary","Primary actions","Accent & Highlights"},
    {"border.color","Borders & separators","Borders & Separators",QPalette::Mid},
    {"hex.bytes","Hex Viewer · bytes","Tool Colors"},{"hex.decoded","Hex Viewer · decoded text","Tool Colors"},{"hex.offset","Hex Viewer · offsets","Tool Colors"},{"hex.modified","Hex Viewer · modified bytes","Tool Colors"},
    {"graphics.background","3D / UV · background","Tool Colors"},{"graphics.grid","3D / UV · grid","Tool Colors"},{"graphics.selection","3D / UV · selection","Tool Colors"},{"graphics.model","3D / UV · geometry","Tool Colors"},{"graphics.vertices","3D / UV · vertices","Tool Colors"},{"graphics.wire","3D · wireframe","Tool Colors"},
    {"script.background","Scripting · background","Tool Colors"},{"script.text","Scripting · text","Tool Colors"},{"console.background","Console · background","Tool Colors"},{"console.text","Console · text","Tool Colors"},
    {"calculator.digit","Calculator · numbers","Tool Colors"},{"calculator.hex","Calculator · hexadecimal digits","Tool Colors"},{"calculator.operator","Calculator · operators","Tool Colors"},{"calculator.bits","Calculator · bit operations","Tool Colors"},{"calculator.memory","Calculator · memory","Tool Colors"},
    {"style.bar","Styles · menu and status bars (Aqua)","Interface Styles"},{"style.glass","Styles · glass bars (Aero)","Interface Styles"},{"style.panelTitle","Styles · panel titles and rail (Aqua)","Interface Styles"},{"style.progress","Styles · progress bars (Aqua, Aero)","Interface Styles"},
    {"style.neon","Neon edges and neon glow","Effects"}
};return tokens;}
inline bool validateTheme(const QJsonObject &theme,QString &error){
    if(theme["format"]!="binary-workbench-theme"||theme["version"].toInt()!=1){error="Unsupported theme format or version.";return false;}
    auto name=theme["name"].toString().trimmed();if(name.isEmpty()||name.size()>64){error="Theme names must contain 1–64 characters.";return false;}
    if(!QStringList{"dark","light","system"}.contains(theme["base"].toString())||!theme["colors"].isObject()||(!theme["options"].isUndefined()&&!theme["options"].isObject())){error="Invalid theme base, colors or options.";return false;}
    auto colors=theme["colors"].toObject();for(auto it=colors.begin();it!=colors.end();++it){bool known=false;for(auto &token:themeTokens())known|=token.key==it.key();if(!known||!QRegularExpression("^#[0-9a-fA-F]{6}$").match(it.value().toString()).hasMatch()){error="Unknown color or invalid RGB value: "+it.key();return false;}}
    // Options: fieldBackground, borders, monospace, neon (bool); roundness (0-24 px); glow (style, none, soft, strong, neon).
    auto options=theme["options"].toObject();for(auto it=options.begin();it!=options.end();++it){
        const auto key=it.key();const auto value=it.value();
        const bool valid=(QStringList{"fieldBackground","borders","monospace","neon"}.contains(key)&&value.isBool())||(key=="roundness"&&value.isDouble()&&value.toInt(-1)>=0&&value.toInt(-1)<=24)||(key=="glow"&&value.isString()&&QStringList{"style","none","soft","strong","neon"}.contains(value.toString()));
        if(!valid){error="Invalid theme option: "+key;return false;}
    }
    return true;
}
