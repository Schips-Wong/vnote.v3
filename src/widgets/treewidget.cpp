#include "treewidget.h"

#include <QContextMenuEvent>
#include <QDropEvent>
#include <QEvent>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyleOptionViewItem>

#include "styleditemdelegate.h"
#include <core/thememgr.h>
#include <core/vnotex.h>
#include <utils/widgetutils.h>

using namespace vnotex;

namespace {
// Maximum number of items pinned on the top of the viewport.
const int c_maxStickyItemCount = 7;

// Maximum ratio of the viewport height the pinned items may occupy.
const double c_maxStickyHeightRatio = 0.4;
} // namespace

TreeWidget::TreeWidget(QWidget *p_parent) : QTreeWidget(p_parent) {}

TreeWidget::TreeWidget(TreeWidget::Flags p_flags, QWidget *p_parent)
    : QTreeWidget(p_parent), m_flags(p_flags) {
  if (m_flags & Flag::EnhancedStyle) {
    auto interface = QSharedPointer<StyledItemDelegateTreeWidget>::create(this);
    auto delegate = new StyledItemDelegate(interface, StyledItemDelegate::Highlights, this);
    setItemDelegate(delegate);
  }
}

bool TreeWidget::isStickyParentItemsEnabled() const {
  return m_flags.testFlag(Flag::StickyParentItems);
}

void TreeWidget::setStickyParentItemsEnabled(bool p_enabled) {
  if (isStickyParentItemsEnabled() == p_enabled) {
    return;
  }

  m_flags.setFlag(Flag::StickyParentItems, p_enabled);
  viewport()->update();
}

void TreeWidget::mousePressEvent(QMouseEvent *p_event) {
  if (handleStickyItemMouseEvent(p_event, false)) {
    return;
  }

  QTreeWidget::mousePressEvent(p_event);

  if (m_flags & Flag::ClickSpaceToClearSelection) {
    auto idx = indexAt(p_event->pos());
    if (!idx.isValid()) {
      clearSelection();
      setCurrentItem(NULL);
    }
  }
}

void TreeWidget::mouseDoubleClickEvent(QMouseEvent *p_event) {
  if (handleStickyItemMouseEvent(p_event, true)) {
    return;
  }

  QTreeWidget::mouseDoubleClickEvent(p_event);
}

void TreeWidget::paintEvent(QPaintEvent *p_event) {
  QTreeWidget::paintEvent(p_event);

  if (isStickyParentItemsEnabled()) {
    QPainter painter(viewport());
    paintStickyItems(&painter);
  }
}

bool TreeWidget::event(QEvent *p_event) {
  switch (p_event->type()) {
  case QEvent::StyleChange:
  case QEvent::PaletteChange:
    // Invalidate the cached background color.
    m_stickyBackground = QColor();
    break;

  default:
    break;
  }

  return QTreeWidget::event(p_event);
}

bool TreeWidget::viewportEvent(QEvent *p_event) {
  if (p_event->type() == QEvent::ContextMenu && isStickyParentItemsEnabled()) {
    const QPoint pos = static_cast<QContextMenuEvent *>(p_event)->pos();
    for (const auto &sticky : calcStickyItems()) {
      if (sticky.m_rect.contains(pos)) {
        // The pinned item is an overlay. Block the context menu here to avoid operating
        // on the item actually hidden below it.
        p_event->accept();
        return true;
      }
    }
  }

  return QTreeWidget::viewportEvent(p_event);
}

void TreeWidget::scrollContentsBy(int dx, int dy) {
  QTreeWidget::scrollContentsBy(dx, dy);

  if (isStickyParentItemsEnabled()) {
    // The viewport content is scrolled as a pixmap. Repaint the whole viewport so that
    // the pinned items are not scrolled away.
    viewport()->update();
  }
}

void TreeWidget::setupSingleColumnHeaderlessTree(QTreeWidget *p_widget, bool p_contextMenu,
                                                 bool p_extendedSelection) {
  p_widget->setColumnCount(1);
  p_widget->setHeaderHidden(true);
  if (p_contextMenu) {
    p_widget->setContextMenuPolicy(Qt::CustomContextMenu);
  }
  if (p_extendedSelection) {
    p_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  }
}

void TreeWidget::showHorizontalScrollbar(QTreeWidget *p_tree) {
  p_tree->header()->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  p_tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  p_tree->header()->setStretchLastSection(false);
}

QTreeWidgetItem *TreeWidget::findItem(const QTreeWidget *p_widget, const QVariant &p_data,
                                      int p_column) {
  int nrTop = p_widget->topLevelItemCount();
  for (int i = 0; i < nrTop; ++i) {
    auto item = findItemHelper(p_widget->topLevelItem(i), p_data, p_column);
    if (item) {
      return item;
    }
  }

  return nullptr;
}

QTreeWidgetItem *TreeWidget::findItemHelper(QTreeWidgetItem *p_item, const QVariant &p_data,
                                            int p_column) {
  if (!p_item) {
    return nullptr;
  }

  if (p_item->data(0, Qt::UserRole) == p_data) {
    return p_item;
  }

  int nrChild = p_item->childCount();
  for (int i = 0; i < nrChild; ++i) {
    auto item = findItemHelper(p_item->child(i), p_data, p_column);
    if (item) {
      return item;
    }
  }

  return nullptr;
}

void TreeWidget::keyPressEvent(QKeyEvent *p_event) {
  if (WidgetUtils::processKeyEventLikeVi(this, p_event)) {
    return;
  }

  // On Mac OS X, it is `Command+O` to activate an item, instead of Return.
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC)
  if (p_event->key() == Qt::Key_Return || p_event->key() == Qt::Key_Enter) {
    if (auto item = currentItem()) {
      emit itemActivated(item, currentColumn());
    }
    return;
  }
#endif

  QTreeWidget::keyPressEvent(p_event);
}

QTreeWidgetItem *TreeWidget::nextItem(const QTreeWidget *p_tree, QTreeWidgetItem *p_item,
                                      bool p_forward) {
  QTreeWidgetItem *nItem = NULL;
  if (p_forward) {
    if (p_item->isExpanded() && p_item->childCount() > 0) {
      nItem = p_item->child(0);
    } else {
      while (!nItem && p_item) {
        nItem = nextSibling(p_tree, p_item, true);
        p_item = p_item->parent();
      }
    }
  } else {
    nItem = nextSibling(p_tree, p_item, false);
    if (!nItem) {
      nItem = p_item->parent();
    } else {
      nItem = lastItemOfTree(nItem);
    }
  }

  return nItem;
}

QTreeWidgetItem *TreeWidget::lastItemOfTree(QTreeWidgetItem *p_item) {
  if (p_item->isExpanded() && p_item->childCount() > 0) {
    return p_item->child(p_item->childCount() - 1);
  } else {
    return p_item;
  }
}

QTreeWidgetItem *TreeWidget::nextSibling(const QTreeWidget *p_tree, QTreeWidgetItem *p_item,
                                         bool p_forward) {
  if (!p_item) {
    return NULL;
  }

  QTreeWidgetItem *pa = p_item->parent();
  if (pa) {
    int idx = pa->indexOfChild(p_item);
    if (p_forward) {
      ++idx;
      if (idx >= pa->childCount()) {
        return NULL;
      }
    } else {
      --idx;
      if (idx < 0) {
        return NULL;
      }
    }

    return pa->child(idx);
  } else {
    // Top level item.
    int idx = p_tree->indexOfTopLevelItem(p_item);
    if (p_forward) {
      ++idx;
      if (idx >= p_tree->topLevelItemCount()) {
        return NULL;
      }
    } else {
      --idx;
      if (idx < 0) {
        return NULL;
      }
    }

    return p_tree->topLevelItem(idx);
  }
}

QVector<QTreeWidgetItem *> TreeWidget::getVisibleItems(const QTreeWidget *p_widget) {
  QVector<QTreeWidgetItem *> items;

  auto firstItem = p_widget->itemAt(0, 0);
  if (!firstItem) {
    return items;
  }

  auto lastItem = p_widget->itemAt(p_widget->viewport()->rect().bottomLeft());

  auto item = firstItem;
  while (item) {
    items.append(item);
    if (item == lastItem) {
      break;
    }

    item = nextItem(p_widget, item, true);
  }

  return items;
}

QTreeWidgetItem *TreeWidget::ancestorUnderPrevious(QTreeWidgetItem *p_node,
                                                   QTreeWidgetItem *p_previous) const {
  auto current = p_node;
  auto parentOfCurrent = current ? current->parent() : nullptr;
  while (parentOfCurrent) {
    if (parentOfCurrent == p_previous) {
      return current;
    }

    current = parentOfCurrent;
    parentOfCurrent = current->parent();
  }

  if (!p_previous) {
    // Return the top level ancestor.
    return current;
  }

  return nullptr;
}

bool TreeWidget::isUncollapsedParent(const QTreeWidgetItem *p_item) {
  return p_item && p_item->isExpanded() && p_item->childCount() > 0;
}

QTreeWidgetItem *TreeWidget::lastDescendant(QTreeWidgetItem *p_item) const {
  auto item = p_item;
  while (item->isExpanded() && item->childCount() > 0) {
    item = item->child(item->childCount() - 1);
  }

  return item;
}

bool TreeWidget::hasNextSibling(const QTreeWidgetItem *p_item) {
  if (!p_item) {
    return false;
  }

  auto widget = p_item->treeWidget();
  return widget && nextSibling(widget, const_cast<QTreeWidgetItem *>(p_item), true);
}

QColor TreeWidget::stickyBackgroundColor() const {
  if (m_stickyBackground.isValid()) {
    return m_stickyBackground;
  }

  QColor color(VNoteX::getInst().getThemeMgr().paletteColor("widgets#qtreeview#bg"));
  if (!color.isValid()) {
    color = viewport()->palette().color(QPalette::Base);
  }

  m_stickyBackground = color;
  return color;
}

int TreeWidget::stickyItemPosition(QTreeWidgetItem *p_item, int p_baseTop, int p_height) const {
  const QRect lastRect = visualItemRect(lastDescendant(p_item));
  if (!lastRect.isValid()) {
    return p_baseTop;
  }

  // Bottom of the whole subtree of @p_item, in viewport coordinates.
  const int bottomOfLast = lastRect.top() + lastRect.height();
  if (p_baseTop + p_height > bottomOfLast && p_baseTop <= bottomOfLast) {
    // The subtree is leaving the pinned area: slide the pinned item up along with it.
    return bottomOfLast - p_height;
  }

  return p_baseTop;
}

QVector<TreeWidget::StickyItem> TreeWidget::calcStickyItems() const {
  QVector<StickyItem> sticks;

  if (topLevelItemCount() == 0 || viewport()->height() <= 0) {
    return sticks;
  }

  // Nothing is scrolled away from the top.
  if (verticalScrollBar()->value() <= 0) {
    return sticks;
  }

  // The item located at the very top of the viewport. It may be covered by the pinned items.
  auto nodeUnderWidget = itemAt(QPoint(0, 0));
  if (!nodeUnderWidget) {
    return sticks;
  }

  const int viewportWidth = viewport()->width();
  const int columnPos = columnViewportPosition(0);
  const int maxHeight = qMax(1, static_cast<int>(viewport()->height() * c_maxStickyHeightRatio));

  QTreeWidgetItem *previous = nullptr;
  int stickyHeight = 0;

  while (sticks.size() < c_maxStickyItemCount) {
    auto item = ancestorUnderPrevious(nodeUnderWidget, previous);
    if (!item) {
      break;
    }

    // The item under the pinned items is pinned only if it is an expanded parent.
    if (item == nodeUnderWidget && !isUncollapsedParent(nodeUnderWidget)) {
      break;
    }

    const QRect itemRect = visualItemRect(item);
    if (!itemRect.isValid()) {
      break;
    }

    // Not pinned yet if its top is at or below the already pinned area.
    if (stickyHeight <= itemRect.top()) {
      break;
    }

    const int height = itemRect.height();
    const int position = stickyItemPosition(item, stickyHeight, height);

    StickyItem sticky;
    sticky.m_item = item;
    sticky.m_rect = QRect(0, position, viewportWidth, height);
    sticky.m_indent = itemRect.left() - columnPos;
    sticks.append(sticky);

    stickyHeight += height;

    // Continue with the item right below the item just pinned.
    auto below = itemAt(QPoint(0, position + height));
    if (!below) {
      break;
    }

    nodeUnderWidget = below;
    previous = item;
  }

  // Constrain the pinned items inside the viewport.
  for (int i = 0; i < sticks.size(); ++i) {
    if (sticks.at(i).m_rect.bottom() + 1 > maxHeight || i >= c_maxStickyItemCount) {
      sticks.resize(i);
      break;
    }
  }

  return sticks;
}

void TreeWidget::paintStickyItems(QPainter *p_painter) {
  const auto sticks = calcStickyItems();
  if (sticks.isEmpty()) {
    return;
  }

  const QRect viewportRect = viewport()->rect();
  int containerBottom = 0;
  for (const auto &sticky : sticks) {
    if (sticky.m_rect.bottom() < 0 || sticky.m_rect.top() > viewportRect.bottom()) {
      continue;
    }

    p_painter->save();
    p_painter->setClipRect(sticky.m_rect, Qt::IntersectClip);
    paintStickyItem(p_painter, sticky);
    p_painter->restore();

    containerBottom = qMax(containerBottom, sticky.m_rect.bottom() + 1);
  }

  // Separator below the pinned area.
  if (containerBottom > 0 && containerBottom <= viewportRect.bottom() + 1) {
    const QColor backgroundColor = stickyBackgroundColor();
    const QColor lineColor = backgroundColor.lightness() < 128 ? backgroundColor.lighter(150)
                                                              : backgroundColor.darker(115);
    p_painter->setPen(lineColor);
    p_painter->drawLine(viewportRect.left(), containerBottom - 1, viewportRect.right(),
                        containerBottom - 1);
  }
}

void TreeWidget::paintStickyItem(QPainter *p_painter, const StickyItem &p_sticky) {
  auto item = p_sticky.m_item;
  const QModelIndex index = indexFromItem(item, 0);
  if (!index.isValid()) {
    return;
  }

  const QRect &rect = p_sticky.m_rect;
  const QColor backgroundColor = stickyBackgroundColor();

  QBrush backgroundBrush(backgroundColor);
  const QVariant backgroundData = item->data(0, Qt::BackgroundRole);
  if (backgroundData.canConvert<QBrush>()) {
    const QBrush brush = backgroundData.value<QBrush>();
    if (brush.style() != Qt::NoBrush) {
      backgroundBrush = brush;
    }
  }

  p_painter->fillRect(rect, backgroundBrush);

  const int columnPos = columnViewportPosition(0);
  const int indentationWidth = indentation();

  // Expand indicator.
  if (item->childCount() > 0) {
    const QRect arrowRect(columnPos + p_sticky.m_indent - indentationWidth, rect.top(),
                          indentationWidth, rect.height());
    QStyleOptionViewItem option = viewOptions();
    option.rect = arrowRect;
    option.state = QStyle::State_Item | QStyle::State_Enabled | QStyle::State_Children
                   | (item->isExpanded() ? QStyle::State_Open : QStyle::State_None)
                   | (hasNextSibling(item) ? QStyle::State_Sibling : QStyle::State_None);
    style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, p_painter, this);
  }

  // Item content, rendered by the item delegate for consistency.
  QStyleOptionViewItem option = viewOptions();
  option.rect = rect;
  option.rect.setLeft(columnPos + p_sticky.m_indent);
  option.showDecorationSelected = false;
  option.state &= ~QStyle::State_MouseOver;
  if (item->isSelected()) {
    option.state |= QStyle::State_Selected;
  } else {
    option.state &= ~QStyle::State_Selected;
  }

  if (auto delegate = itemDelegate(index)) {
    delegate->paint(p_painter, option, index);
  }
}

bool TreeWidget::handleStickyItemMouseEvent(QMouseEvent *p_event, bool p_doubleClick) {
  if (!isStickyParentItemsEnabled()) {
    return false;
  }

  auto sticks = calcStickyItems();
  if (sticks.isEmpty()) {
    return false;
  }

  const QPoint pos = p_event->pos();
  const int columnPos = columnViewportPosition(0);
  const int indentationWidth = indentation();

  for (int i = sticks.size() - 1; i >= 0; --i) {
    const auto &sticky = sticks.at(i);
    if (!sticky.m_rect.contains(pos)) {
      continue;
    }

    auto item = sticky.m_item;
    const QRect arrowRect(columnPos + sticky.m_indent - indentationWidth, sticky.m_rect.top(),
                          indentationWidth, sticky.m_rect.height());
    if (item->childCount() > 0 && arrowRect.contains(pos)) {
      item->setExpanded(!item->isExpanded());
      return true;
    }

    if (p_doubleClick) {
      if (item->childCount() > 0) {
        item->setExpanded(!item->isExpanded());
      }
    } else {
      setCurrentItem(item, 0, QItemSelectionModel::ClearAndSelect);
    }

    return true;
  }

  return false;
}

void TreeWidget::dropEvent(QDropEvent *p_event) {
  auto dragItems = selectedItems();

  QTreeWidget::dropEvent(p_event);

  if (dragItems.size() == 1) {
    emit itemMoved(dragItems[0]);
  }
}

void TreeWidget::forEachItem(const QTreeWidget *p_widget,
                             const std::function<bool(QTreeWidgetItem *p_item)> &p_func) {
  const int cnt = p_widget->topLevelItemCount();
  for (int i = 0; i < cnt; ++i) {
    if (!forEachItem(p_widget->topLevelItem(i), p_func)) {
      break;
    }
  }
}

bool TreeWidget::forEachItem(QTreeWidgetItem *p_item,
                             const std::function<bool(QTreeWidgetItem *p_item)> &p_func) {
  if (!p_item) {
    return true;
  }

  if (!p_func(p_item)) {
    return false;
  }

  const int cnt = p_item->childCount();
  for (int i = 0; i < cnt; ++i) {
    if (!forEachItem(p_item->child(i), p_func)) {
      return false;
    }
  }

  return true;
}

void TreeWidget::mark(QTreeWidgetItem *p_item, int p_column) {
  p_item->setData(p_column, Qt::ForegroundRole, StyledItemDelegate::s_highlightForeground);
  p_item->setData(p_column, Qt::BackgroundRole, StyledItemDelegate::s_highlightBackground);
}

void TreeWidget::unmark(QTreeWidgetItem *p_item, int p_column) {
  p_item->setData(p_column, Qt::ForegroundRole, QVariant());
  p_item->setData(p_column, Qt::BackgroundRole, QVariant());
}

void TreeWidget::expandRecursively(QTreeWidgetItem *p_item) {
  if (!p_item) {
    return;
  }

  p_item->setExpanded(true);
  const int cnt = p_item->childCount();
  if (cnt == 0) {
    return;
  }

  for (int i = 0; i < cnt; ++i) {
    expandRecursively(p_item->child(i));
  }
}

void TreeWidget::selectParentItem(QTreeWidget *p_widget) {
  auto item = p_widget->currentItem();
  if (item) {
    auto pitem = item->parent();
    if (pitem) {
      p_widget->setCurrentItem(pitem, 0, QItemSelectionModel::ClearAndSelect);
    }
  }
}

static bool isItemTreeExpanded(const QTreeWidgetItem *p_item) {
  if (!p_item) {
    return true;
  }

  if (p_item->isHidden() || !p_item->isExpanded()) {
    return false;
  }

  int cnt = p_item->childCount();
  for (int i = 0; i < cnt; ++i) {
    if (!isItemTreeExpanded(p_item->child(i))) {
      return false;
    }
  }

  return true;
}

bool TreeWidget::isExpanded(const QTreeWidget *p_widget) {
  int cnt = p_widget->topLevelItemCount();
  for (int i = 0; i < cnt; ++i) {
    if (!isItemTreeExpanded(p_widget->topLevelItem(i))) {
      return false;
    }
  }

  return true;
}
