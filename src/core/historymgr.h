#ifndef HISTORYMGR_H
#define HISTORYMGR_H

#include <QObject>
#include <QSharedPointer>
#include <QVector>

#include "global.h"
#include "historyitem.h"
#include "noncopyable.h"

namespace vnotex {
class Notebook;

struct HistoryItemFull {
  bool operator<(const HistoryItemFull &p_other) const;

  HistoryItem m_item;

  QString m_notebookName;
};

// Combine the history from all notebooks and from SessionConfig.
// SessionConfig will store history about external files.
// Also provide stack of files accessed during current session, which could be re-opened
// via Ctrl+Shit+T.
class HistoryMgr : public QObject, private Noncopyable {
  Q_OBJECT
public:
  struct LastClosedFile {
    QString m_path;

    int m_lineNumber = 0;

    ViewWindowMode m_mode = ViewWindowMode::Read;

    bool m_readOnly = false;
  };

  static HistoryMgr &getInst() {
    static HistoryMgr inst;
    return inst;
  }

  const QVector<QSharedPointer<HistoryItemFull>> &getHistory() const;

  void add(const QString &p_path, int p_lineNumber, ViewWindowMode p_mode, bool p_readOnly,
           Notebook *p_notebook);

  void remove(const QVector<QString> &p_paths, Notebook *p_notebook);

  // Rename history items whose path matches @p_oldPath (or is located under it, when a
  // folder is renamed). @p_oldPath and @p_newPath are absolute file paths.
  void renamePath(const QString &p_oldPath, const QString &p_newPath, Notebook *p_notebook);

  void clear();

  LastClosedFile popLastClosedFile();

  static void removeHistoryItem(QVector<HistoryItem> &p_history, const QString &p_itemPath);

  static void insertHistoryItem(QVector<HistoryItem> &p_history, const HistoryItem &p_item);

  // Rename history items in @p_history (paths are matched exactly or by parent-folder prefix).
  static void renameHistoryItem(QVector<HistoryItem> &p_history, const QString &p_oldPath,
                               const QString &p_newPath);

signals:
  void historyUpdated();

private:
  HistoryMgr();

  void loadHistory();

  // Sorted by last accessed time ascendingly.
  QVector<QSharedPointer<HistoryItemFull>> m_history;

  void removeFromHistory(const QString &p_itemPath);

  QVector<LastClosedFile> m_lastClosedFiles;

  const bool m_perNotebookHistoryEnabled = false;
};
} // namespace vnotex

#endif // HISTORYMGR_H
