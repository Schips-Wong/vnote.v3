#ifndef HISTORYI_H
#define HISTORYI_H

#include <QVector>

#include <core/historyitem.h>

namespace vnotex {
// History interface for notebook.
class HistoryI {
public:
  virtual ~HistoryI() = default;

  virtual const QVector<HistoryItem> &getHistory() const = 0;

  virtual void addHistory(const HistoryItem &p_item) = 0;

  virtual void removeHistory(const QString &p_itemPath) = 0;

  virtual void renameHistory(const QString &p_oldPath, const QString &p_newPath) = 0;

  virtual void clearHistory() = 0;
};
} // namespace vnotex
#endif // HISTORYI_H
