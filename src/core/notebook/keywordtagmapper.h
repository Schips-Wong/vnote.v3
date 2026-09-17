#ifndef KEYWORDTAGMAPPER_H
#define KEYWORDTAGMAPPER_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace vnotex {
class Notebook;

// Manages "vx_keyword_to_tag.json" inside the notebook config folder (e.g.
// "<notebook>/vx_notebook/vx_keyword_to_tag.json"). It maps a keyword (JSON key) to a tag name
// (JSON value), e.g. { "usb": "|bsp|usb" }.
class KeywordTagMapper {
public:
  KeywordTagMapper() = delete;

  // Relative path of the mapping file within the notebook (inside the config folder).
  static QString getFileName();

  // Load the mapping. Return an empty object if the file does not exist or is invalid.
  static QJsonObject load(Notebook *p_notebook);

  // Return the tag values whose keywords appear in @p_content (case-insensitive).
  static QStringList matchTags(const QJsonObject &p_mapping, const QString &p_content);

  // Return all keys whose value equals @p_tagName (empty if the tag is not mapped).
  // A tag may be mapped from several keywords, so this returns a list.
  static QStringList findKeysForTag(const QJsonObject &p_mapping, const QString &p_tagName);

  // Make @p_keywords the tag's complete set of keyword mappings: @p_keywords -> @p_tagName.
  // Keywords that used to map to this tag but are not in @p_keywords are removed; an empty set
  // clears all the mappings of the tag.
  // Return true if the file has been written.
  static bool setMappings(Notebook *p_notebook, const QStringList &p_keywords,
                          const QString &p_tagName);

  // Replace every mapping value equal to @p_oldTagName with @p_newTagName and persist it.
  // Return true if the file has been updated.
  static bool renameTagValue(Notebook *p_notebook, const QString &p_oldTagName,
                             const QString &p_newTagName);
};
} // namespace vnotex

#endif // KEYWORDTAGMAPPER_H
