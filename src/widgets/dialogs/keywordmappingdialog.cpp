#include "keywordmappingdialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <core/vnotex.h>
#include <notebook/keywordtagmapper.h>
#include <notebook/notebook.h>

#include "../messageboxhelper.h"
#include "../widgetsfactory.h"

using namespace vnotex;

KeywordMappingDialog::KeywordMappingDialog(Notebook *p_notebook, const QString &p_tagName,
                                           QWidget *p_parent)
    : Dialog(p_parent), m_notebook(p_notebook), m_tagName(p_tagName) {
  setupUI();

  updateStatus();

  setMinimumWidth(620);
}

void KeywordMappingDialog::setupUI() {
  auto mainWidget = new QWidget(this);
  setCentralWidget(mainWidget);

  auto mainLayout = new QVBoxLayout(mainWidget);

  auto hint = new QLabel(
      tr("After mapping a keyword to a tag, it can be matched as a candidate tag in the note's "
         "auto tag detection."),
      mainWidget);
  hint->setWordWrap(true);
  mainLayout->addWidget(hint);

  auto usage = new QLabel(
      tr("Update sets the tag's keyword mappings to exactly the keywords in the box (separated "
         "by ';'). Remove a keyword by deleting it from the box; clear all mappings by emptying "
         "the box."),
      mainWidget);
  usage->setWordWrap(true);
  mainLayout->addWidget(usage);

  m_statusLabel = new QLabel(mainWidget);
  m_statusLabel->setWordWrap(true);
  mainLayout->addWidget(m_statusLabel);

  {
    auto formLayout = WidgetsFactory::createFormLayout(mainWidget);
    m_keywordLineEdit = WidgetsFactory::createLineEdit(mainWidget);
    m_keywordLineEdit->setToolTip(tr("Separate multiple keywords with ';'."));
    formLayout->addRow(tr("Keyword:"), m_keywordLineEdit);
    mainLayout->addLayout(formLayout);
  }

  mainLayout->addStretch();

  setDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  if (auto okBtn = getDialogButtonBox()->button(QDialogButtonBox::Ok)) {
    okBtn->setText(tr("Update"));
  }

  setWindowTitle(tr("Keyword Mapping"));
}

void KeywordMappingDialog::updateStatus() {
  const auto mapping = KeywordTagMapper::load(m_notebook);
  const auto keys = KeywordTagMapper::findKeysForTag(mapping, m_tagName);
  if (keys.isEmpty()) {
    m_statusLabel->setText(tr("No keyword mapping yet."));
    m_keywordLineEdit->setText(m_tagName);
  } else {
    m_statusLabel->setText(tr("Mapped to keyword(s): %1").arg(keys.join(QStringLiteral("; "))));
    m_keywordLineEdit->setText(keys.join(QStringLiteral("; ")));
  }
}

QStringList KeywordMappingDialog::keywordsFromInput() const {
  QStringList keywords;
  const auto parts = m_keywordLineEdit->text().split(QLatin1Char(';'));
  for (const auto &part : parts) {
    const auto trimmed = part.trimmed();
    if (!trimmed.isEmpty() && !keywords.contains(trimmed)) {
      keywords << trimmed;
    }
  }
  return keywords;
}

void KeywordMappingDialog::acceptedButtonClicked() {
  const auto keywords = keywordsFromInput();
  const bool written = KeywordTagMapper::setMappings(m_notebook, keywords, m_tagName);
  if (!written && !keywords.isEmpty()) {
    MessageBoxHelper::notify(MessageBoxHelper::Type::Warning,
                             tr("Failed to save keyword mapping."), this);
    return;
  }

  VNoteX::getInst().showStatusMessageShort(tr("Keyword mapping saved."));
  accept();
}
