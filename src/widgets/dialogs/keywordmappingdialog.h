#ifndef KEYWORDMAPPINGDIALOG_H
#define KEYWORDMAPPINGDIALOG_H

#include <QString>
#include <QStringList>

#include "dialog.h"

class QLabel;
class QLineEdit;

namespace vnotex {
class Notebook;

// Dialog to create/edit the keyword-to-tag mapping ("vx_keyword_to_tag.json") for one tag.
class KeywordMappingDialog : public Dialog {
  Q_OBJECT
public:
  KeywordMappingDialog(Notebook *p_notebook, const QString &p_tagName,
                       QWidget *p_parent = nullptr);

protected:
  void acceptedButtonClicked() Q_DECL_OVERRIDE;

private:
  void setupUI();

  void updateStatus();

  // Split the line edit content into keywords (';'-separated, trimmed, empty items dropped).
  QStringList keywordsFromInput() const;

  Notebook *m_notebook = nullptr;

  QString m_tagName;

  QLabel *m_statusLabel = nullptr;

  QLineEdit *m_keywordLineEdit = nullptr;
};
} // namespace vnotex

#endif // KEYWORDMAPPINGDIALOG_H
