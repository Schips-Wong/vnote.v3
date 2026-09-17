#include "autotagdialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include <core/vnotex.h>
#include <notebook/node.h>
#include <notebook/notebook.h>
#include <notebook/tagi.h>

using namespace vnotex;

namespace {
const int c_tagNameRole = Qt::UserRole;
}

AutoTagDialog::AutoTagDialog(Node *p_node, const QStringList &p_candidates,
                             const QStringList &p_preselectedTags, QWidget *p_parent)
    : Dialog(p_parent), m_node(p_node) {
  setupUI(p_candidates, p_preselectedTags);
}

void AutoTagDialog::setupUI(const QStringList &p_candidates,
                            const QStringList &p_preselectedTags) {
  auto mainWidget = new QWidget(this);
  setCentralWidget(mainWidget);

  auto mainLayout = new QVBoxLayout(mainWidget);

  m_nodeNameLabel = new QLabel(mainWidget);
  m_nodeNameLabel->setText(tr("Note: %1").arg(m_node ? m_node->getName() : QString()));
  mainLayout->addWidget(m_nodeNameLabel);

  auto hint = new QLabel(
      tr("Tags detected from the note content. Check the ones you want to add:"), mainWidget);
  hint->setWordWrap(true);
  mainLayout->addWidget(hint);

  {
    auto btnLayout = new QHBoxLayout();
    auto selectAllBtn = new QPushButton(tr("Select All"), mainWidget);
    connect(selectAllBtn, &QPushButton::clicked, this, [this]() { setAllChecked(true); });
    auto unselectAllBtn = new QPushButton(tr("Unselect All"), mainWidget);
    connect(unselectAllBtn, &QPushButton::clicked, this, [this]() { setAllChecked(false); });
    btnLayout->addWidget(selectAllBtn);
    btnLayout->addWidget(unselectAllBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);
  }

  m_tagList = new QListWidget(mainWidget);
  const auto existingTags = m_node ? m_node->getTags() : QStringList();
  for (const auto &tag : p_candidates) {
    auto item = new QListWidgetItem(tag, m_tagList);
    item->setData(c_tagNameRole, tag);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    // Only the keyword-dictionary matches are pre-selected.
    item->setCheckState(p_preselectedTags.contains(tag) ? Qt::Checked : Qt::Unchecked);
    if (existingTags.contains(tag)) {
      // Already attached: listed for information only, not editable.
      item->setText(tr("%1 (already tagged)").arg(tag));
      item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
  }
  mainLayout->addWidget(m_tagList);

  setDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

  setWindowTitle(tr("Auto Tag Detection"));
}

void AutoTagDialog::setAllChecked(bool p_checked) {
  for (int i = 0; i < m_tagList->count(); ++i) {
    auto item = m_tagList->item(i);
    if (!item->flags().testFlag(Qt::ItemIsEnabled)) {
      continue;
    }
    item->setCheckState(p_checked ? Qt::Checked : Qt::Unchecked);
  }
}

QStringList AutoTagDialog::selectedTags() const {
  QStringList tags;
  for (int i = 0; i < m_tagList->count(); ++i) {
    const auto item = m_tagList->item(i);
    if (item->checkState() == Qt::Checked) {
      tags << item->data(c_tagNameRole).toString();
    }
  }
  return tags;
}

void AutoTagDialog::applyTags() {
  if (!m_node) {
    return;
  }

  auto tagI = m_node->getNotebook()->tag();
  if (!tagI) {
    return;
  }

  const QStringList existing = m_node->getTags();
  QStringList tags = existing;

  const auto selected = selectedTags();
  for (const auto &tag : selected) {
    if (tags.contains(tag)) {
      continue;
    }

    if (!tagI->findTag(tag)) {
      tagI->newTag(tag, QString());
    }
    tags << tag;
  }

  if (tags != existing) {
    if (tagI->updateNodeTags(m_node, tags)) {
      VNoteX::getInst().showStatusMessageShort(
          tr("Tags updated: %1").arg(tags.join(QLatin1String("; "))));
    } else {
      VNoteX::getInst().showStatusMessageShort(tr("Failed to update tags."));
    }
  }
}

void AutoTagDialog::acceptedButtonClicked() {
  applyTags();
  accept();
}
