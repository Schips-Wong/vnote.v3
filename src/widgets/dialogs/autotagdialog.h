#ifndef AUTOTAGDIALOG_H
#define AUTOTAGDIALOG_H

#include <QStringList>

#include "dialog.h"

class QLabel;
class QListWidget;

namespace vnotex {
class Node;

// Dialog that presents the candidate tags detected from a note's content. The user checks the
// candidates to keep; accepted ones are created (if not existing yet) and attached to the node.
class AutoTagDialog : public Dialog {
  Q_OBJECT
public:
  AutoTagDialog(Node *p_node, const QStringList &p_candidates, QWidget *p_parent = nullptr);

protected:
  void acceptedButtonClicked() Q_DECL_OVERRIDE;

private:
  void setupUI(const QStringList &p_candidates);

  void setAllChecked(bool p_checked);

  QStringList selectedTags() const;

  void applyTags();

  Node *m_node = nullptr;

  QLabel *m_nodeNameLabel = nullptr;

  QListWidget *m_tagList = nullptr;
};
} // namespace vnotex

#endif // AUTOTAGDIALOG_H
