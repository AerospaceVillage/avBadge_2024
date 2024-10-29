#ifndef WINGLETUI_SCROLLABLEMENU_H
#define WINGLETUI_SCROLLABLEMENU_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QAbstractItemModel>

#include "menuitemwidget.h"

namespace WingletUI {

class ScrollableMenu : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariant currentData READ currentData)
    Q_PROPERTY(bool menuWrap READ menuWrap WRITE setMenuWrap)
    Q_PROPERTY(int maxVisibleItems READ maxVisibleItems WRITE setMaxVisibleItems)
    Q_PROPERTY(int modelColumn READ modelColumn WRITE setModelColumn)
    Q_PROPERTY(bool canExitFromMenu READ canExitFromMenu WRITE setCanExitFromMenu)
    Q_PROPERTY(bool showShrinkFromOutside READ showShrinkFromOutside WRITE setShowShrinkFromOutside)
    Q_PROPERTY(bool shrinkOnSelect READ shrinkOnSelect WRITE setShrinkOnSelect)

    /* Public API */
public:

    explicit ScrollableMenu(QWidget *parent = nullptr);
    ~ScrollableMenu();

    // Data Model Properties
    QAbstractItemModel *model() const { return m_model; }
    void setModel(QAbstractItemModel *model);
    QModelIndex rootModelIndex() const { return m_root; }
    void setRootModelIndex(const QModelIndex &index);
    int modelColumn() const { return m_modelColumn; }
    void setModelColumn(int visibleColumn);
    int currentIndex() const { return m_currentIndex.row(); }
    QVariant currentData(int role = Qt::UserRole) const { return m_currentIndex.data(role); }
    int count() const { return m_model->rowCount(m_root); }

    // Rendering Properties
    int maxVisibleItems() const { return m_numVisibleItems; }
    void setMaxVisibleItems(int maxItems);
    bool menuWrap() const { return m_menuShouldWrap; }
    void setMenuWrap(bool wrapEnable);
    void setFontSizes(int titleSize, int selectedSize, int decreaseRate);
    bool canExitFromMenu() const { return m_canExitFromMenu; }
    void setCanExitFromMenu(bool canExit);
    bool showShrinkFromOutside() const { return m_showShrinkFromOutside; }
    void setShowShrinkFromOutside(bool shrinkFromOutside) { m_showShrinkFromOutside = shrinkFromOutside; }
    bool shrinkOnSelect() const { return m_shrinkOnSelect; }
    void setShrinkOnSelect(bool shrinkOnSelect) { m_shrinkOnSelect = shrinkOnSelect; }

public slots:
    void moveUp();
    void moveDown();
    void setCurrentIndex(int index);
    void selectCurrentItem();
    void goBack();

signals:
    void currentIndexChanged(int index);
    void itemSelected(QModelIndex index);
    void menuExit();
    void startingHideAnimation(unsigned int duration);
    void startingShowAnimation(unsigned int duration);

    /* Event Handling */
protected:
    void wheelEvent(QWheelEvent *ev) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;


    /* Animation Control */
private:
    enum MenuAnimationState { ANIMATE_NONE, ANIMATE_UP, ANIMATE_UP_BOUNCE, ANIMATE_DOWN, ANIMATE_DOWN_BOUNCE,
                              ANIMATE_OUT_GROW, ANIMATE_OUT_SHRINK, ANIMATE_IN_GROW, ANIMATE_IN_SHRINK, ANIMATE_HIDDEN };
    MenuAnimationState animationState;

    QParallelAnimationGroup *animationGroup;

    void moveDownNow();
    void moveUpNow();
    void beginEntriesHide(bool grow);
    void showAnimation(bool isFromAnotherWindow, bool grow);
private slots:
    void animationGroupFinished();


    /* Underlying Rendering Code */
private:
    // Model Storage
    QAbstractItemModel *m_model;
    QPersistentModelIndex m_currentIndex;
    QPersistentModelIndex m_root;
    QPersistentModelIndex m_requestedRoot;
    int m_modelColumn;

    // Model Update Slots
private slots:
    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    void enterMenu();
    void leaveMenu();

    // Widget Storage
    MenuItemWidget** menuItems;
    size_t menuItemsAllocCount;
    int m_numVisibleItems;
    QLabel* title;
    QLabel* selectionPtr;

    int m_titleFontSize;
    int m_selectedFontSize;
    int m_fontDecreaseRate;
    bool m_menuShouldWrap;  // If true, allow the menu to wrap around, if false, don't wrap and just cut it off
    bool m_canExitFromMenu;
    bool m_showShrinkFromOutside;  // If true, the menu initial show animations enter from the outside, if false, they grom from the intsidechanges
    bool m_shrinkOnSelect;  // If true, the menu shrinks rather than growing on select (typically useful if the select causes the menu to exit)

    // Icons (used if model item is marked as checkable)
    QPixmap checkedIcon;
    QPixmap uncheckedIcon;

    // Functions for rerendering UI
    void regenerateLabels();  // Regenerates all labels based on rendering properties (should be called on UI property changes)
    void recomputeLabelText();  // Recomputes all of the text to appear in each label, should be called when index/text content changes
    void recomputeLabelPositioning();  // Recomputes the positioning of the labels, should be called when dimensions change

    // Utility Functions
    int getFontPointFromIdx(float idx);
    void setItemFromRelIdx(MenuItemWidget* item, int relIdx);  // Sets the text for the given label to match
    QPoint getIndexLocation(MenuItemWidget* item, float idx, bool secondLineCanBeVisible,
                            int fontSize = -1, float radiusRatio = 0.9f);  // Converts a given menu item index to its corresponding position on the screen for that label
};

} // namespace WingletUI

#endif // WINGLETUI_SCROLLABLEMENU_H
