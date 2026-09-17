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
// Candidates from the keyword dictionary (@p_preselectedTags) are checked by default; all the
// others start unchecked.
class AutoTagDialog : public Dialog {
  Q_OBJECT
public:
  AutoTagDialog(Node *p_node, const QStringList &p_candidates,
                const QStringList &p_preselectedTags, QWidget *p_parent = nullptr);

protected:
  void acceptedButtonClicked() Q_DECL_OVERRIDE;

private:
  void setupUI(const QStringList &p_candidates, const QStringList &p_preselectedTags);

  void setAllChecked(bool p_checked);

  QStringList selectedTags() const;

  void applyTags();

  Node *m_node = nullptr;

  QLabel *m_nodeNameLabel = nullptr;

  QListWidget *m_tagList = nullptr;
};
} // namespace vnotex

#endif // AUTOTAGDIALOG_H
