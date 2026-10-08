#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QColor>
#include <QRect>
#include <QTreeWidget>
#include <QVariant>
#include <QVector>

namespace vnotex {
class TreeWidget : public QTreeWidget {
  Q_OBJECT
public:
  enum Flag {
    None = 0,
    ClickSpaceToClearSelection = 0x1,
    EnhancedStyle = 0x2,
    // Keep the expanded parent items pinned at the top of the viewport while their
    // descendants are being scrolled.
    StickyParentItems = 0x4
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  explicit TreeWidget(QWidget *p_parent = nullptr);

  TreeWidget(TreeWidget::Flags p_flags, QWidget *p_parent = nullptr);

  void mark(QTreeWidgetItem *p_item, int p_column);

  void unmark(QTreeWidgetItem *p_item, int p_column);

  static void setupSingleColumnHeaderlessTree(QTreeWidget *p_widget, bool p_contextMenu,
                                              bool p_extendedSelection);

  static void showHorizontalScrollbar(QTreeWidget *p_tree);

  static QTreeWidgetItem *findItem(const QTreeWidget *p_widget, const QVariant &p_data,
                                   int p_column = 0);

  // Next visible item.
  static QTreeWidgetItem *nextItem(const QTreeWidget *p_tree, QTreeWidgetItem *p_item,
                                   bool p_forward);

  static QVector<QTreeWidgetItem *> getVisibleItems(const QTreeWidget *p_widget);

  // @p_func: return false to abort the iteration.
  static void forEachItem(const QTreeWidget *p_widget,
                          const std::function<bool(QTreeWidgetItem *p_item)> &p_func);

  static void expandRecursively(QTreeWidgetItem *p_item);

  static void selectParentItem(QTreeWidget *p_widget);

  static bool isExpanded(const QTreeWidget *p_widget);

  bool isStickyParentItemsEnabled() const;

  void setStickyParentItemsEnabled(bool p_enabled);

signals:
  // Emit when single item is selected and Drag&Drop to move internally.
  void itemMoved(QTreeWidgetItem *p_item);

protected:
  void mousePressEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;

  void mouseDoubleClickEvent(QMouseEvent *p_event) Q_DECL_OVERRIDE;

  void keyPressEvent(QKeyEvent *p_event) Q_DECL_OVERRIDE;

  void paintEvent(QPaintEvent *p_event) Q_DECL_OVERRIDE;

  bool event(QEvent *p_event) Q_DECL_OVERRIDE;

  bool viewportEvent(QEvent *p_event) Q_DECL_OVERRIDE;

  void scrollContentsBy(int dx, int dy) Q_DECL_OVERRIDE;

  void dropEvent(QDropEvent *p_event) Q_DECL_OVERRIDE;

private:
  static QTreeWidgetItem *findItemHelper(QTreeWidgetItem *p_item, const QVariant &p_data,
                                         int p_column);

  static QTreeWidgetItem *nextSibling(const QTreeWidget *p_widget, QTreeWidgetItem *p_item,
                                      bool p_forward);

  static QTreeWidgetItem *lastItemOfTree(QTreeWidgetItem *p_item);

  // @p_func: return false to abort the iteration.
  // Return false to abort the ieration.
  static bool forEachItem(QTreeWidgetItem *p_item,
                          const std::function<bool(QTreeWidgetItem *p_item)> &p_func);

  // A parent item pinned at the top of the viewport.
  struct StickyItem {
    QTreeWidgetItem *m_item = nullptr;

    // Rectangle of the pinned item in viewport coordinates.
    QRect m_rect;

    // Indentation of the item relative to the tree column origin.
    int m_indent = 0;
  };

  // Return the ancestor of @p_node which is a direct child of @p_previous, or the top level
  // ancestor of @p_node if @p_previous is null.
  QTreeWidgetItem *ancestorUnderPrevious(QTreeWidgetItem *p_node,
                                         QTreeWidgetItem *p_previous) const;

  // Whether @p_item is an expanded parent (aka. a container with visible children).
  static bool isUncollapsedParent(const QTreeWidgetItem *p_item);

  // The last item of the visible subtree rooted at @p_item.
  QTreeWidgetItem *lastDescendant(QTreeWidgetItem *p_item) const;

  static bool hasNextSibling(const QTreeWidgetItem *p_item);

  // Position (viewport coordinate) of a pinned item which is stacked below @p_baseTop.
  int stickyItemPosition(QTreeWidgetItem *p_item, int p_baseTop, int p_height) const;

  // Compute the list of parent items to pin, from the outermost one to the innermost one.
  QVector<StickyItem> calcStickyItems() const;

  QColor stickyBackgroundColor() const;

  void paintStickyItems(QPainter *p_painter);

  void paintStickyItem(QPainter *p_painter, const StickyItem &p_sticky);

  // Return true if the event is consumed by the pinned items.
  bool handleStickyItemMouseEvent(QMouseEvent *p_event, bool p_doubleClick);

  Flags m_flags = Flag::None;

  // Cached background color of the pinned items. Invalidated on style change.
  mutable QColor m_stickyBackground;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TreeWidget::Flags)
} // namespace vnotex

#endif // TREEWIDGET_H
