#include "keywordtagmapper.h"

#include <QByteArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

#include <core/exception.h>
#include <notebookbackend/inotebookbackend.h>

#include "notebook.h"

using namespace vnotex;

QString KeywordTagMapper::getFileName() { return QStringLiteral("vx_keyword_to_tag.json"); }

QJsonObject KeywordTagMapper::load(Notebook *p_notebook) {
  if (!p_notebook) {
    return QJsonObject();
  }

  const auto &backend = p_notebook->getBackend();
  if (!backend) {
    return QJsonObject();
  }

  const QString file = getFileName();
  try {
    if (!backend->existsFile(file)) {
      return QJsonObject();
    }

    const QByteArray data = backend->readFile(file);
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
      qWarning() << "failed to parse keyword-to-tag mapping" << file << error.errorString();
      return QJsonObject();
    }

    return doc.object();
  } catch (Exception &p_e) {
    qWarning() << "failed to read keyword-to-tag mapping" << file << p_e.what();
    return QJsonObject();
  }
}

QStringList KeywordTagMapper::matchTags(const QJsonObject &p_mapping, const QString &p_content) {
  QStringList tags;
  if (p_content.isEmpty()) {
    return tags;
  }

  for (auto it = p_mapping.constBegin(); it != p_mapping.constEnd(); ++it) {
    const QString key = it.key().trimmed();
    const QString value = it.value().toString().trimmed();
    if (key.isEmpty() || value.isEmpty()) {
      continue;
    }

    if (p_content.contains(key, Qt::CaseInsensitive) && !tags.contains(value)) {
      tags << value;
    }
  }

  return tags;
}

QStringList KeywordTagMapper::findKeysForTag(const QJsonObject &p_mapping,
                                             const QString &p_tagName) {
  QStringList keys;
  if (p_tagName.isEmpty()) {
    return keys;
  }

  for (auto it = p_mapping.constBegin(); it != p_mapping.constEnd(); ++it) {
    if (it.value().toString() == p_tagName) {
      keys << it.key();
    }
  }

  return keys;
}

bool KeywordTagMapper::setMappings(Notebook *p_notebook, const QStringList &p_keywords,
                                   const QString &p_tagName) {
  if (!p_notebook || p_tagName.isEmpty()) {
    return false;
  }

  QStringList keywords;
  for (const auto &keyword : p_keywords) {
    const auto trimmed = keyword.trimmed();
    if (!trimmed.isEmpty() && !keywords.contains(trimmed)) {
      keywords << trimmed;
    }
  }

  const auto &backend = p_notebook->getBackend();
  if (!backend) {
    return false;
  }

  // The given keywords become the tag's complete set of mappings: first drop the old ones, then
  // add the new set (a tag may still be matched by several keywords). An empty set clears all the
  // mappings of the tag.
  QJsonObject mapping = load(p_notebook);
  bool changed = false;

  const auto oldKeys = mapping.keys();
  for (const auto &key : oldKeys) {
    if (mapping.value(key).toString() == p_tagName) {
      mapping.remove(key);
      changed = true;
    }
  }

  for (const auto &keyword : keywords) {
    mapping[keyword] = p_tagName;
    changed = true;
  }

  if (!changed) {
    return false;
  }

  try {
    backend->writeFile(getFileName(), mapping);
  } catch (Exception &p_e) {
    qWarning() << "failed to write keyword-to-tag mapping" << getFileName() << p_e.what();
    return false;
  }

  return true;
}

bool KeywordTagMapper::renameTagValue(Notebook *p_notebook, const QString &p_oldTagName,
                                      const QString &p_newTagName) {
  if (!p_notebook || p_oldTagName.isEmpty() || p_newTagName.isEmpty() ||
      p_oldTagName == p_newTagName) {
    return false;
  }

  QJsonObject mapping = load(p_notebook);
  if (mapping.isEmpty()) {
    return false;
  }

  bool changed = false;
  for (auto it = mapping.begin(); it != mapping.end(); ++it) {
    if (it.value().toString() == p_oldTagName) {
      it.value() = p_newTagName;
      changed = true;
    }
  }

  if (!changed) {
    return false;
  }

  const auto &backend = p_notebook->getBackend();
  if (!backend) {
    return false;
  }

  try {
    backend->writeFile(getFileName(), mapping);
  } catch (Exception &p_e) {
    qWarning() << "failed to write keyword-to-tag mapping" << getFileName() << p_e.what();
    return false;
  }

  return true;
}
