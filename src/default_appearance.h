#pragma once
#include <QJsonDocument>
#include <QJsonObject>
// Approved product appearance. Saved personal preferences take priority.
inline QJsonObject standardAppearance(){static const auto theme=QJsonDocument::fromJson(R"theme(
{
  "base": "dark",
  "colors": {
    "background.window": "#0e1115"
  },
  "format": "binary-workbench-theme",
  "name": "My Theme",
  "options": {},
  "version": 1
}
)theme").object();return theme;}
