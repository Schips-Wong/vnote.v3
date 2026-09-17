#ifndef KEYWORDTAGMAPPER_H
#define KEYWORDTAGMAPPER_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace vnotex {
class Notebook;

// Manages the notebook-level file "vx_keyword_to_tag.json", which maps a keyword (JSON key) to a
// tag name (JSON value), e.g. { "usb": "|bsp|usb" }.
class KeywordTagMapper {
public:
  KeywordTagMapper() = delete;

  // Relative path of the mapping file within the notebook root.
  static QString getFileName();

  // Load the mapping. Return an empty object if the file does not exist or is invalid.
  static QJsonObject load(Notebook *p_notebook);

  // Return the tag values whose keywords appear in @p_content (case-insensitive).
  static QStringList matchTags(const QJsonObject &p_mapping, const QString &p_content);

  // Replace every mapping value equal to @p_oldTagName with @p_newTagName and persist it.
  // Return true if the file has been updated.
  static bool renameTagValue(Notebook *p_notebook, const QString &p_oldTagName,
                             const QString &p_newTagName);
};
} // namespace vnotex

#endif // KEYWORDTAGMAPPER_H
