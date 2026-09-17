#include "locationlist.h"

#include <QDir>
#include <QHeaderView>
#include <QLabel>

#include <notebook/notebook.h>
#include <QToolButton>
#include <QVBoxLayout>

#include "navigationmodemgr.h"
#include "styleditemdelegate.h"
#include "titlebar.h"
#include "treewidget.h"
#include "widgetsfactory.h"

#include <core/thememgr.h>
#include <core/vnotex.h>
#include <utils/iconutils.h>
#include <utils/widgetutils.h>

using namespace vnotex;

QIcon LocationList::s_bufferIcon;

QIcon LocationList::s_fileIcon;

QIcon LocationList::s_folderIcon;

QIcon LocationList::s_notebookIcon;

LocationList::LocationList(QWidget *p_parent) : QFrame(p_parent) {
  setupUI();

  connect(&VNoteX::getInst(), &VNoteX::nodeRenamed, this, &LocationList::handleNodeRenamed);
}

void LocationList::setupUI() {
  auto mainLayout = new QVBoxLayout(this);
  WidgetUtils::setContentsMargins(mainLayout);

  {
    setupTitleBar(QString(), this);
    mainLayout->addWidget(m_titleBar);
  }

  m_tree = new TreeWidget(TreeWidget::Flag::EnhancedStyle, this);
  // When updated, pay attention to the Columns enum.
  m_tree->setHeaderLabels(QStringList() << tr("Path") << tr("Line") << tr("Text"));
  m_tree->header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_tree->header()->setStretchLastSection(true);
  connect(m_tree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *p_item, int p_col) {
    Q_UNUSED(p_col);
    if (!m_callback) {
      return;
    }
    m_callback(getItemLocation(p_item));
  });
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, &QTreeWidget::customContextMenuRequested, this,
          [this](const QPoint &p_pos) {
            auto item = m_tree->itemAt(p_pos);
            if (!item) {
              return;
            }
            emit locationContextMenuRequested(getItemLocation(item),
                                             m_tree->viewport()->mapToGlobal(p_pos));
          });
  mainLayout->addWidget(m_tree);

  m_navigationWrapper.reset(new NavigationModeWrapper<QTreeWidget, QTreeWidgetItem>(m_tree));
  NavigationModeMgr::getInst().registerNavigationTarget(m_navigationWrapper.data());

  setFocusProxy(m_tree);
}

const QIcon &LocationList::getItemIcon(LocationType p_type) {
  if (s_bufferIcon.isNull()) {
    // Init.
    const QString nodeIconFgName = "widgets#locationlist#node_icon#fg";
    const auto &themeMgr = VNoteX::getInst().getThemeMgr();
    const auto fg = themeMgr.paletteColor(nodeIconFgName);

    s_bufferIcon = IconUtils::fetchIcon(themeMgr.getIconFile("buffer.svg"), fg);
    s_fileIcon = IconUtils::fetchIcon(themeMgr.getIconFile("file_node.svg"), fg);
    s_folderIcon = IconUtils::fetchIcon(themeMgr.getIconFile("folder_node.svg"), fg);
    s_notebookIcon = IconUtils::fetchIcon(themeMgr.getIconFile("notebook_default.svg"), fg);
  }

  switch (p_type) {
  case LocationType::Buffer:
    return s_bufferIcon;

  case LocationType::File:
    return s_fileIcon;

  case LocationType::Folder:
    return s_folderIcon;

  case LocationType::Notebook:
    Q_FALLTHROUGH();
  default:
    return s_notebookIcon;
  }
}

void LocationList::setupTitleBar(const QString &p_title, QWidget *p_parent) {
  m_titleBar = new TitleBar(p_title, true, TitleBar::Action::None, p_parent);
  m_titleBar->setActionButtonsAlwaysShown(true);

  {
    auto clearBtn = m_titleBar->addActionButton(QStringLiteral("clear.svg"), tr("Clear"));
    connect(clearBtn, &QToolButton::triggered, this, &LocationList::clear);
  }
}

void LocationList::clear() {
  m_tree->clear();

  m_callback = LocationCallback();

  updateItemsCountLabel();
}

void LocationList::setItemLocationLineAndText(QTreeWidgetItem *p_item,
                                              const ComplexLocation::Line &p_line) {
  p_item->setData(Columns::LineColumn, Qt::UserRole, p_line.m_lineNumber);
  if (p_line.m_lineNumber != -1) {
    p_item->setText(Columns::LineColumn, QString::number(p_line.m_lineNumber + 1));
  }

  // Truncate the text.
  if (p_line.m_text.size() > 500) {
    p_item->setText(Columns::TextColumn, p_line.m_text.left(500));
  } else {
    p_item->setText(Columns::TextColumn, p_line.m_text);
  }

  if (!p_line.m_segments.isEmpty()) {
    p_item->setData(Columns::TextColumn, HighlightsRole, QVariant::fromValue(p_line.m_segments));
  }
}

void LocationList::addLocation(const ComplexLocation &p_location) {
  // Check if this path already exists
  QTreeWidgetItem *existingItem = nullptr;
  for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
    auto item = m_tree->topLevelItem(i);
    if (item->data(Columns::PathColumn, Qt::UserRole).toString() == p_location.m_path) {
      existingItem = item;
      break;
    }
  }

  // Filter out empty lines
  QVector<ComplexLocation::Line> validLines;
  for (const auto &line : p_location.m_lines) {
    if (line.m_lineNumber != -1 && !line.m_text.isEmpty()) {
      validLines.append(line);
    }
  }

  if (validLines.isEmpty()) {
    // No valid lines, only add if this is a new path
    if (!existingItem) {
      auto item = new QTreeWidgetItem(m_tree);
      item->setText(Columns::PathColumn, p_location.m_displayPath);
      item->setIcon(Columns::PathColumn, getItemIcon(p_location.m_type));
      item->setData(Columns::PathColumn, Qt::UserRole, p_location.m_path);
      item->setToolTip(Columns::PathColumn, p_location.m_path);
    }
    return;
  }

  if (existingItem) {
    // Replace existing item with new valid lines
    delete existingItem;
  }

  auto item = new QTreeWidgetItem(m_tree);
  item->setText(Columns::PathColumn, p_location.m_displayPath);
  item->setIcon(Columns::PathColumn, getItemIcon(p_location.m_type));
  item->setData(Columns::PathColumn, Qt::UserRole, p_location.m_path);
  item->setToolTip(Columns::PathColumn, p_location.m_path);

  if (validLines.size() == 1) {
    setItemLocationLineAndText(item, validLines[0]);
  } else if (validLines.size() > 1) {
    // Add sub items.
    for (const auto &line : validLines) {
      auto subItem = new QTreeWidgetItem(item);
      setItemLocationLineAndText(subItem, line);
    }
    // Keep the item collapsed by default.
    item->setExpanded(false);
  }

  if (m_tree->topLevelItemCount() == 1) {
    m_tree->setCurrentItem(item);
  }

  updateItemsCountLabel();
}

void LocationList::startSession(const LocationCallback &p_callback) { m_callback = p_callback; }

void LocationList::handleNodeRenamed(const QString &p_oldPath, const QString &p_newPath,
                                     const QString &p_oldRelativePath,
                                     const QString &p_newRelativePath, Notebook *p_notebook) {
  Q_UNUSED(p_notebook);
  if (p_oldPath == p_newPath) {
    return;
  }

  const QChar sep = QDir::separator();
  const QString prefixOldAbs = p_oldPath + sep;
  const QString prefixNewAbs = p_newPath + sep;
  const QString prefixOldAbsSlash = p_oldPath + QLatin1Char('/');
  const QString prefixNewAbsSlash = p_newPath + QLatin1Char('/');
  const QString prefixOldRel = p_oldRelativePath + sep;
  const QString prefixNewRel = p_newRelativePath + sep;
  const QString prefixOldRelSlash = p_oldRelativePath + QLatin1Char('/');
  const QString prefixNewRelSlash = p_newRelativePath + QLatin1Char('/');

  for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
    auto item = m_tree->topLevelItem(i);

    // Update the stored (absolute) path.
    const QString path = item->data(Columns::PathColumn, Qt::UserRole).toString();
    QString newPath = path;
    if (path == p_oldPath) {
      newPath = p_newPath;
    } else if (path.startsWith(prefixOldAbs)) {
      newPath = prefixNewAbs + path.mid(prefixOldAbs.size());
    } else if (path.startsWith(prefixOldAbsSlash)) {
      newPath = prefixNewAbsSlash + path.mid(prefixOldAbsSlash.size());
    } else {
      // Not related to the renamed node.
      continue;
    }
    item->setData(Columns::PathColumn, Qt::UserRole, newPath);
    item->setToolTip(Columns::PathColumn, newPath);

    // Update the displayed (relative) path/name.
    const QString display = item->text(Columns::PathColumn);
    QString newDisplay = display;
    if (display == p_oldPath) {
      newDisplay = p_newPath;
    } else if (display.startsWith(prefixOldAbs)) {
      newDisplay = prefixNewAbs + display.mid(prefixOldAbs.size());
    } else if (display.startsWith(prefixOldAbsSlash)) {
      newDisplay = prefixNewAbsSlash + display.mid(prefixOldAbsSlash.size());
    } else if (display == p_oldRelativePath) {
      newDisplay = p_newRelativePath;
    } else if (display.startsWith(prefixOldRel)) {
      newDisplay = prefixNewRel + display.mid(prefixOldRel.size());
    } else if (display.startsWith(prefixOldRelSlash)) {
      newDisplay = prefixNewRelSlash + display.mid(prefixOldRelSlash.size());
    }
    item->setText(Columns::PathColumn, newDisplay);
  }
}

Location LocationList::getItemLocation(const QTreeWidgetItem *p_item) const {
  Location loc;

  if (!p_item) {
    return loc;
  }

  auto paItem = p_item->parent() ? p_item->parent() : p_item;
  loc.m_path = paItem->data(Columns::PathColumn, Qt::UserRole).toString();
  loc.m_displayPath = paItem->text(Columns::PathColumn);

  auto lineNumberData = p_item->data(Columns::LineColumn, Qt::UserRole);
  if (lineNumberData.isValid()) {
    loc.m_lineNumber = lineNumberData.toInt();
  }
  return loc;
}

void LocationList::updateItemsCountLabel() {
  const auto cnt = m_tree->topLevelItemCount();
  if (cnt == 0) {
    m_titleBar->setInfoLabel("");
  } else {
    m_titleBar->setInfoLabel(tr("%n Item(s)", "", cnt));
  }
}

QByteArray LocationList::saveState() const { return m_tree->header()->saveState(); }

void LocationList::restoreState(const QByteArray &p_data) {
  m_tree->header()->restoreState(p_data);
}
