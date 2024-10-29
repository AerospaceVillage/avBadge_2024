#include "scrollablemenu.h"
#include "winglet-ui/theme.h"
#include <QResizeEvent>
#include <QPalette>
#include <math.h>
#include <QGraphicsOpacityEffect>

#include "wingletgui.h"

namespace WingletUI {

const static int selectionPtrXPos = 25;

ScrollableMenu::ScrollableMenu(QWidget *parent)
    : QWidget{parent},
      animationState(ANIMATE_NONE),
      m_model(NULL), m_modelColumn(0),
      menuItems(NULL), menuItemsAllocCount(0), m_numVisibleItems(5),
      m_titleFontSize(26), m_selectedFontSize(22), m_fontDecreaseRate(5),
      m_menuShouldWrap(false), m_canExitFromMenu(true), m_showShrinkFromOutside(false),
      m_shrinkOnSelect(false),

      checkedIcon(":/icons/settings/checkbox_true.png"),
      uncheckedIcon(":/icons/settings/checkbox_false.png")
{

    const int selectionCaretFontSize = 26;

    // Create title
    title = new QLabel(this);
    title->setWordWrap(true);
    title->setAlignment(Qt::AlignCenter);
    title->setForegroundRole(QPalette::Text);
    title->resize(175, 100);
    // Font size in regenerate labels

    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(title);
    effect->setOpacity(1.0);
    title->setGraphicsEffect(effect);

    // Create selection ptr
    selectionPtr = new QLabel(this);
    selectionPtr->setText(">");
    selectionPtr->setForegroundRole(QPalette::HighlightedText);
    selectionPtr->setAlignment(Qt::AlignCenter);
    selectionPtr->setFont(QFont(activeTheme->titleFont, selectionCaretFontSize, QFont::Bold));
    selectionPtr->setFixedSize(selectionPtr->sizeHint());

    // Menu entries are dynamically created in regenerateLabels

    // Create Animation
    animationGroup = new QParallelAnimationGroup(this);
    connect(animationGroup, SIGNAL(finished()), this, SLOT(animationGroupFinished()));

    regenerateLabels();
}

ScrollableMenu::~ScrollableMenu() {
    delete animationGroup;

    for (size_t i = 0; i < menuItemsAllocCount; i++) {
        delete menuItems[i];
    }
    if (menuItems)
        delete[] menuItems;

    delete title;
    delete selectionPtr;

    if (m_model && m_model->QObject::parent() == this) {
        delete m_model;
    }
}

// ========================================
// Event Handling
// ========================================

void ScrollableMenu::wheelEvent(QWheelEvent *ev) {
    if (ev->angleDelta().y() < 0) {
        if (WingletGUI::inst->settings.invertedScrollDirection())
            moveUp();
        else
            moveDown();
    }
    else {
        if (WingletGUI::inst->settings.invertedScrollDirection())
            moveDown();
        else
            moveUp();
    }
    ev->accept();
}

void ScrollableMenu::keyPressEvent(QKeyEvent *ev) {
    switch (ev->key()) {
    case Qt::Key::Key_Up:
        moveDown();
        break;
    case Qt::Key::Key_Down:
        moveUp();
        break;
    case Qt::Key::Key_Return:
    case Qt::Key::Key_A:
        selectCurrentItem();
        break;
    case Qt::Key::Key_B:
        goBack();
        break;
    default:
        ev->ignore();
    }
}

void ScrollableMenu::resizeEvent(QResizeEvent *event) {
    (void) event;
    recomputeLabelPositioning();
}

void ScrollableMenu::showEvent(QShowEvent *event)
{
    (void) event;
    showAnimation(true, showShrinkFromOutside());
}

// ========================================
// Movement Animation
// ========================================

void ScrollableMenu::animationGroupFinished() {
    // NOTE: This class assumes that all possible codepaths eventually call recomputeLabelText()
    // This allows functions to defer redrawing if an animation is in progress (so animations don't get abruptly cut)

    switch (animationState) {
    case ANIMATE_UP:
        moveUpNow();
        break;
    case ANIMATE_DOWN:
        moveDownNow();
        break;
    case ANIMATE_UP_BOUNCE:
    case ANIMATE_DOWN_BOUNCE:
    case ANIMATE_IN_GROW:
    case ANIMATE_IN_SHRINK:
        recomputeLabelText();
        break;
    case ANIMATE_OUT_GROW:
        enterMenu();
        return;
    case ANIMATE_OUT_SHRINK:
        leaveMenu();
        return;
    default:
        qWarning("ScrollableMenu::animationGroupFinished: "
                 "Invalid Animation State when animate completed: %d", animationState);
    }

    animationState = ANIMATE_NONE;
}

void ScrollableMenu::moveUp() {
    if (animationState != ANIMATE_NONE && animationState != ANIMATE_UP) {
        // Only allow interrupting animation in the same direction (if not it causes weird visual glitches)
        return;
    }

    // Stop and clear any previous animation data
    animationGroup->clear();

    // If animations disabled, just move now
    if (activeTheme->animationDuration == 0) {
        moveUpNow();
        return;
    }

    // See if we are allowed to move up
    bool moveUpAllowed = true;
    if (!m_menuShouldWrap && currentIndex() == count() - 1) {
        moveUpAllowed = false;
    }

    // Load the temporary label with the text flying in from the top
    setItemFromRelIdx(menuItems[m_numVisibleItems], m_numVisibleItems/2 + 1);
    menuItems[m_numVisibleItems]->move(getIndexLocation(menuItems[m_numVisibleItems], m_numVisibleItems, false));
    menuItems[m_numVisibleItems]->setVisible(true);

    for (int i = 0; i < m_numVisibleItems + 1; i++) {
        // Don't animate blank labels
        if (menuItems[i]->blank())
            continue;

        bool needsToHideSecondLine = (i == m_numVisibleItems / 2) && menuItems[i]->hasSecondLine();
        bool needsToShowSecondLine = (i == m_numVisibleItems / 2 + 1) && menuItems[i]->hasSecondLine();

        // Create movement animation
        QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "pos");
        anim->setStartValue(menuItems[i]->pos());
        if (moveUpAllowed) {
            anim->setDuration(activeTheme->animationDuration);
            anim->setKeyValueAt(0.33, getIndexLocation(menuItems[i], i-0.33, needsToHideSecondLine));
            anim->setKeyValueAt(0.66, getIndexLocation(menuItems[i], i-0.66, needsToShowSecondLine));
            anim->setEndValue(getIndexLocation(menuItems[i], i-1, needsToShowSecondLine, getFontPointFromIdx(i-1)));
        }
        else {
            anim->setDuration(activeTheme->animationDuration*2);
            anim->setKeyValueAt(0.5, getIndexLocation(menuItems[i], i-0.3, needsToHideSecondLine));
            anim->setEndValue(menuItems[i]->pos());
            anim->setEasingCurve(QEasingCurve::OutInCirc);
        }
        animationGroup->addAnimation(anim);

        // Create font size animation if available
        if (m_fontDecreaseRate && moveUpAllowed) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "fontSize");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(menuItems[i]->fontSize());
            anim->setEndValue(getFontPointFromIdx(i-1));
            animationGroup->addAnimation(anim);
        }

        // Fade in/out the second line if visiblle
        if (needsToHideSecondLine) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "secondLineOpacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(1.0);
            anim->setKeyValueAt(0.33, 0.0);
            anim->setEndValue(0.0);
            animationGroup->addAnimation(anim);
        }

        if (needsToShowSecondLine) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "secondLineOpacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(0.0);
            anim->setKeyValueAt(0.66, 0.0);
            anim->setEndValue(1.0);
            animationGroup->addAnimation(anim);
        }

        // Fade in/out items at end
        if (moveUpAllowed && i == m_numVisibleItems) {
            // Fade in the new item
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "opacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            animationGroup->addAnimation(anim);
        }

        if (moveUpAllowed && i == 0) {
            // Fade out the old item
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "opacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            animationGroup->addAnimation(anim);
        }
    }
    animationState = moveUpAllowed ? ANIMATE_UP : ANIMATE_UP_BOUNCE;
    animationGroup->start();
}

void ScrollableMenu::moveUpNow() {
    int idx = currentIndex();
    if (idx == count() - 1) {
        if (m_menuShouldWrap) {
            setCurrentIndex(0);
        }
    }
    else {
        setCurrentIndex(idx + 1);
    }
}

void ScrollableMenu::moveDown() {
    if (animationState != ANIMATE_NONE && animationState != ANIMATE_DOWN) {
        // Only allow interrupting animation in the same direction (if not it causes weird visual glitches)
        return;
    }

    // Stop and clear any previous animation data
    animationGroup->clear();

    // If animations disabled, just move now
    if (activeTheme->animationDuration == 0) {
        moveDownNow();
        return;
    }

    // See if we are allowed to move up
    bool moveDownAllowed = true;
    if (!m_menuShouldWrap && currentIndex() == 0) {
        moveDownAllowed = false;
    }

    // Load the temporary label with the text flying in from the top
    setItemFromRelIdx(menuItems[m_numVisibleItems], - m_numVisibleItems/2 - 1);
    menuItems[m_numVisibleItems]->move(getIndexLocation(menuItems[m_numVisibleItems], -1, false));
    menuItems[m_numVisibleItems]->setVisible(true);

    // Prepare the animation routine
    for (int i = 0; i < m_numVisibleItems + 1; i++) {
        // Don't animate blank labels
        if (menuItems[i]->blank())
            continue;

        // Special handling since last label is moved to the start
        int menuIdx = i;
        if (i == m_numVisibleItems) {
            menuIdx = -1;
        }

        bool needsToHideSecondLine = (i == m_numVisibleItems / 2) && menuItems[i]->hasSecondLine();
        bool needsToShowSecondLine = (i == m_numVisibleItems / 2 - 1) && menuItems[i]->hasSecondLine();

        // Create the movement animation
        QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "pos");
        anim->setStartValue(menuItems[i]->pos());
        if (moveDownAllowed) {
            anim->setDuration(activeTheme->animationDuration);
            anim->setKeyValueAt(0.33, getIndexLocation(menuItems[i], menuIdx+0.33, needsToHideSecondLine));
            anim->setKeyValueAt(0.66, getIndexLocation(menuItems[i], menuIdx+0.66, needsToShowSecondLine));
            anim->setEndValue(getIndexLocation(menuItems[i], menuIdx+1, needsToShowSecondLine, getFontPointFromIdx(menuIdx+1)));
        }
        else {
            // Show bounce on failure
            anim->setDuration(activeTheme->animationDuration*2);
            anim->setKeyValueAt(0.5, getIndexLocation(menuItems[i], menuIdx+0.3, needsToHideSecondLine));
            anim->setEndValue(menuItems[i]->pos());
            anim->setEasingCurve(QEasingCurve::OutInCirc);
        }
        animationGroup->addAnimation(anim);

        // Create font size animation if available
        if (m_fontDecreaseRate && moveDownAllowed) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "fontSize");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(menuItems[i]->fontSize());
            anim->setEndValue(getFontPointFromIdx(menuIdx+1));
            animationGroup->addAnimation(anim);
        }

        // Fade in/out the second line if visiblle
        if (needsToHideSecondLine) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "secondLineOpacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(1.0);
            anim->setKeyValueAt(0.33, 0.0);
            anim->setEndValue(0.0);
            animationGroup->addAnimation(anim);
        }

        if (needsToShowSecondLine) {
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "secondLineOpacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(0.0);
            anim->setKeyValueAt(0.66, 0.0);
            anim->setEndValue(1.0);
            animationGroup->addAnimation(anim);
        }

        // Fade in/out items at end
        if (moveDownAllowed && i == m_numVisibleItems) {
            // Fade in the new item
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "opacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            animationGroup->addAnimation(anim);
        }

        if (moveDownAllowed && i == m_numVisibleItems - 1) {
            // Fade out the old item
            QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "opacity");
            anim->setDuration(activeTheme->animationDuration);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            animationGroup->addAnimation(anim);
        }
    }
    animationState = moveDownAllowed ? ANIMATE_DOWN : ANIMATE_DOWN_BOUNCE;
    animationGroup->start();
}

void ScrollableMenu::moveDownNow() {
    int idx = currentIndex();
    if (idx == 0) {
        if (m_menuShouldWrap) {
            setCurrentIndex(count() - 1);
        }
    }
    else {
        setCurrentIndex(idx - 1);
    }
}

void ScrollableMenu::selectCurrentItem() {
    if (animationState != ANIMATE_NONE) {
        // Don't go into submenu until all other animations complete
        return;
    }

    if (!m_currentIndex.isValid()) {
        // Can't select current item if current index is invalid
        return;
    }

    // If the item is a checkbox, then toggle the item check state rather than trying to navigate into it
    Qt::ItemFlags flags = m_model->flags(m_currentIndex);
    QVariant checkState = m_model->data(m_currentIndex, Qt::CheckStateRole);
    if ((flags & Qt::ItemIsUserCheckable) && !checkState.isNull()) {
        if (flags & Qt::ItemIsEnabled) {
            // If item is enabled, then toggle the checkbox when selected
            Qt::CheckState newState = (checkState.value<Qt::CheckState>() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            m_model->setData(m_currentIndex, newState, Qt::CheckStateRole);
            // TODO: Animate check state if this returns true
        }
        return;
    }


    // If animations disabled, just perform action now
    if (activeTheme->animationDuration == 0) {
        enterMenu();
        return;
    }

    // Begin the hide animation
    beginEntriesHide(true);
}

void ScrollableMenu::enterMenu()
{
    if (m_currentIndex.isValid()) {
        if (m_model->rowCount(m_currentIndex)) {
            // It's a submenu, traverse into it
            m_root = m_currentIndex;
            setCurrentIndex(0);
            showAnimation(false, false);
        }
        else {
            // No children, it is a final item
            animationState = ANIMATE_HIDDEN;
            hide();

            // Emit the selected event
            emit itemSelected(m_currentIndex);
        }
    }
}

void ScrollableMenu::goBack() {
    if (animationState != ANIMATE_NONE) {
        // Don't go back until all other animations complete
        return;
    }

    if (m_root == m_requestedRoot || !m_root.isValid()) {
        // Requested exit, but at top level
        // If exit from menu not allowed, ignore the request
        if (!canExitFromMenu())
            return;
    }

    // If animations disabled, just perform action now
    if (activeTheme->animationDuration == 0) {
        leaveMenu();
        return;
    }

    // Begin the animation
    beginEntriesHide(false);
}

void ScrollableMenu::leaveMenu()
{
    if (m_root.isValid() && m_root != m_requestedRoot) {
        // Only traverse up if it could have a parent and it's not the requested root from the parent caller
        int prevIndex = m_root.row();
        m_root = m_root.parent();
        setCurrentIndex(prevIndex);

        showAnimation(false, true);
    }
    else {
        // We couldn't traverse up anymore, request must have been a menu exit
        animationState = ANIMATE_HIDDEN;
        hide();

        // Emit menu exit signal
        emit menuExit();
    }
}

void ScrollableMenu::beginEntriesHide(bool grow)
{
    if (animationState != ANIMATE_NONE) {
        // Don't allow animation while another animation in progress
        return;
    }

    // Stop and clear any previous animation data
    animationGroup->clear();

    // Animate all the labels
    for (int i = 0; i < m_numVisibleItems; i++) {
        // Don't animate blank labels
        if (menuItems[i]->blank())
            continue;

        // Create movement animation
        QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "pos");
        anim->setStartValue(menuItems[i]->pos());
        anim->setDuration(activeTheme->animationDuration);
        // Allow overidding shrink direction from property (but only on final item
        if (grow && (!shrinkOnSelect() || m_model->rowCount(m_currentIndex)))
            anim->setEndValue(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2, -1, 1.75));
        else
            anim->setEndValue(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2, -1, 0.2));
        animationGroup->addAnimation(anim);
    }

    QPropertyAnimation* anim = new QPropertyAnimation(title->graphicsEffect(), "opacity");
    anim->setStartValue(1);
    anim->setDuration(activeTheme->animationDuration);
    anim->setEndValue(0);
    animationGroup->addAnimation(anim);

    anim = new QPropertyAnimation(selectionPtr, "pos");
    anim->setStartValue(selectionPtr->pos());
    anim->setDuration(activeTheme->animationDuration);
    if (grow && (!shrinkOnSelect() || m_model->rowCount(m_currentIndex)))
        anim->setEndValue(QPoint(selectionPtr->x() - ((1.75 - 0.9) * (width()/2)), selectionPtr->y()));
    else
        anim->setEndValue(QPoint(selectionPtr->x() + ((0.9 - 0.2) * (width()/2)), selectionPtr->y()));
    animationGroup->addAnimation(anim);

    animationState = (grow ? ANIMATE_OUT_GROW : ANIMATE_OUT_SHRINK);
    animationGroup->start();

    // Notify that we're starting to hide if we're actually selecting the item (and not just going into a submenu)
    if (grow && m_currentIndex.isValid() && !m_model->rowCount(m_currentIndex)) {
        emit startingHideAnimation(activeTheme->animationDuration);
    }
}

void ScrollableMenu::showAnimation(bool isFromAnotherWindow, bool grow)
{
    assert(animationState == ANIMATE_NONE || animationState == ANIMATE_HIDDEN || animationState == ANIMATE_OUT_GROW || animationState == ANIMATE_OUT_SHRINK);

    // Stop and clear any previous animation data
    animationGroup->clear();

    // If animations disabled, just rerender all the items now
    if (activeTheme->animationDuration == 0) {
        recomputeLabelText();
        return;
    }

    for (int i = 0; i < m_numVisibleItems; i++) {
        // Don't animate blank labels
        if (menuItems[i]->blank())
            continue;

        // Create movement animation
        QPropertyAnimation* anim = new QPropertyAnimation(menuItems[i], "pos");
        if (grow)
            anim->setStartValue(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2, -1, 1.75));
        else
            anim->setStartValue(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2, -1, 0.2));
        anim->setDuration(activeTheme->animationDuration);
        anim->setEndValue(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2));
        animationGroup->addAnimation(anim);
    }

    QPropertyAnimation* anim = new QPropertyAnimation(title->graphicsEffect(), "opacity");
    anim->setStartValue(0);
    anim->setDuration(activeTheme->animationDuration);
    anim->setEndValue(1);
    animationGroup->addAnimation(anim);

    anim = new QPropertyAnimation(selectionPtr, "pos");
    if (grow)
        anim->setStartValue(QPoint(selectionPtrXPos - ((1.75 - 0.9) * (width()/2)) - selectionPtr->width()/2, height()/2 - selectionPtr->height() / 2 - 3));
    else
        anim->setStartValue(QPoint(selectionPtrXPos + ((0.9 - 0.2) * (width()/2)) - selectionPtr->width()/2, height()/2 - selectionPtr->height() / 2 - 3));
    anim->setDuration(activeTheme->animationDuration);
    anim->setEndValue(QPoint(selectionPtrXPos - selectionPtr->width()/2, height()/2 - selectionPtr->height() / 2 - 3));
    animationGroup->addAnimation(anim);

    animationState = (grow ? ANIMATE_IN_GROW : ANIMATE_IN_SHRINK);
    animationGroup->start();

    if (isFromAnotherWindow) {
        // Only emit we're starting show if we're coming from another application window (and not just navigating through menus)
        emit startingShowAnimation(activeTheme->animationDuration);
    }
}

// ========================================
// Underlying Rendering Code
// ========================================

void ScrollableMenu::setModel(QAbstractItemModel *model) {

    if (Q_UNLIKELY(!model)) {
        qWarning("ScrollableMenu::setModel: cannot set a 0 model");
        return;
    }

    if (model == m_model)
        return;

    if (m_model) {
        // Unsubscribe from all events
        disconnect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)),
                   this, SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&, const QList<int>&)));

        if (m_model->QObject::parent() == this) {
            delete m_model;
        }
    }

    // Update internal variables
    m_model = model;
    m_root = QPersistentModelIndex();
    m_requestedRoot = QPersistentModelIndex();
    setCurrentIndex(0);

    // Connect all signals required for receiving updates
    connect(m_model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)),
            this, SLOT(modelDataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)));
}

void ScrollableMenu::setRootModelIndex(const QModelIndex &index) {
    m_root = QPersistentModelIndex(index);
    m_requestedRoot = m_root;
    setCurrentIndex(0);
}

void ScrollableMenu::setModelColumn(int visibleColumn) {
    m_modelColumn = visibleColumn;
    recomputeLabelText();
}

void ScrollableMenu::setMenuWrap(bool wrapEnable) {
    m_menuShouldWrap = wrapEnable;
    recomputeLabelText();
}

void ScrollableMenu::setCanExitFromMenu(bool canExit) {
    if (!canExit && animationState == ANIMATE_OUT_SHRINK) {
        qWarning("ScrollableMenu::setCanExitFromMenu: Disabling menu exiting while exit in progress, this may break the ui");
    }
    m_canExitFromMenu = canExit;
}

void ScrollableMenu::setCurrentIndex(int index) {
    QModelIndex mi = m_model->index(index, m_modelColumn, m_root);
    if (!mi.isValid()) {
        // Fallback to valid model index if none implemented
        mi = m_model->index(0, m_modelColumn, m_root);
    }
    if (mi != m_currentIndex) {
        m_currentIndex = QPersistentModelIndex(mi);
        recomputeLabelText();
        emit currentIndexChanged(index);
    }
}

void ScrollableMenu::setMaxVisibleItems(int maxItems) {
    m_numVisibleItems = maxItems;
    regenerateLabels();
}

void ScrollableMenu::setFontSizes(int titleSize, int selectedSize, int decreaseRate) {
    m_titleFontSize = titleSize;
    m_selectedFontSize = selectedSize;
    m_fontDecreaseRate = decreaseRate;
    regenerateLabels();
}

void ScrollableMenu::modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    (void) roles;

    if (!isVisible() || animationState != ANIMATE_NONE) {
        // Skip redraw if we're currently animating, it'll redraw once the animation completes
        return;
    }

    int viewStretch = maxVisibleItems() / 2;

    // Make sure the data changed is applicable to what is currently visible
    if ((topLeft.parent() != m_currentIndex.parent() &&
         bottomRight.parent() != m_currentIndex.parent()) ||
            topLeft.row() > m_currentIndex.row() + viewStretch ||
            topLeft.column() > m_currentIndex.column() ||
            bottomRight.row() < m_currentIndex.row() - viewStretch ||
            bottomRight.column() < m_currentIndex.column()) {
        return;
    }

    recomputeLabelText();
}

void ScrollableMenu::regenerateLabels() {
    title->setFont(QFont(activeTheme->titleFont, m_titleFontSize));

    // Stop animation if in progress before we start destroying UI objects
    animationGroup->clear();
    animationState = ANIMATE_NONE;

    // Make sure we have enough menu items
    size_t requiredLabels = m_numVisibleItems + 1;
    if (requiredLabels != menuItemsAllocCount) {
        if (menuItemsAllocCount > 0) {
            // Already previously allocated, need to adjust allocation
            MenuItemWidget** newMenuItems = new MenuItemWidget*[requiredLabels];

            if (requiredLabels > menuItemsAllocCount) {
                // Need to grow array (alloc additional labels)
                memcpy(newMenuItems, menuItems, menuItemsAllocCount * sizeof(*menuItems));
                for (size_t i = menuItemsAllocCount; i < requiredLabels; i++) {
                    newMenuItems[i] = new MenuItemWidget(this);
                }
            }
            else {
                // Need to shrink array (delete unneeded labels)
                memcpy(newMenuItems, menuItems, requiredLabels * sizeof(*menuItems));
                for (size_t i = requiredLabels; i < menuItemsAllocCount; i++) {
                    delete menuItems[i];
                }
            }
            delete[] menuItems;
            menuItems = newMenuItems;
        }
        else {
            assert(menuItems == NULL);  // Not allocated yet, need to alloc
            menuItems = new MenuItemWidget*[requiredLabels];
            for (size_t i = 0; i < requiredLabels; i++) {
                menuItems[i] = new MenuItemWidget(this);
            }
        }
        menuItemsAllocCount = requiredLabels;
    }

    for (int i = 0; i < m_numVisibleItems + 1; i++) {
        menuItems[i]->setFontSize(getFontPointFromIdx(i));

        if (i == m_numVisibleItems) {
            // Last item is reserved for flying on/off during animations, hide by default
            menuItems[i]->setVisible(false);
        }
        else {
            menuItems[i]->setVisible(true);
        }
    }

    // Now that we've generated the labels, populate text
    recomputeLabelText();
}

void ScrollableMenu::recomputeLabelText() {
    bool set = false;
    if (m_model) {
        QVariant data = m_model->data(m_root);
        if (data.isValid() && data.type() == QVariant::String) {
            title->setText(data.toString());
            set = true;
        }
    }
    if (!set) {
        title->setText("No Name");
    }
    title->graphicsEffect()->setProperty("opacity", 1.0);

    for (int i = 0; i < m_numVisibleItems; i++) {
        int relIdx = i - (m_numVisibleItems/2);  // The current index is the center value

        if (m_fontDecreaseRate) {
            menuItems[i]->setFontSize(getFontPointFromIdx(i));
        }

        setItemFromRelIdx(menuItems[i], relIdx);
    }

    recomputeLabelPositioning();
}

void ScrollableMenu::setItemFromRelIdx(MenuItemWidget* item, int relIdx) {
    QModelIndex idx;

    if (m_currentIndex.isValid()) {
        if (relIdx == 0) {
            idx = m_currentIndex;
        }
        else {
            int rowIdx = m_currentIndex.row();
            rowIdx += relIdx;
            if (m_menuShouldWrap) {
                if (rowIdx < 0) {
                    rowIdx = (rowIdx % count()) + count();
                }
                else if (rowIdx >= count()) {
                    rowIdx = rowIdx % count();
                }
            }
            idx = m_currentIndex.sibling(rowIdx, m_modelColumn);
        }
    }

    QString itemText;
    QPixmap itemIcon;
    QString secondLineText;
    bool secondLineVisible = false;
    bool resetForegroundRole = true;
    if (m_model && idx.isValid()) {
        itemText = m_model->data(idx, Qt::DisplayRole).toString();

        QVariant checkState = m_model->data(idx, Qt::CheckStateRole);
        if (!checkState.isNull()) {
            itemIcon = (checkState.value<Qt::CheckState>() == Qt::Checked) ? checkedIcon : uncheckedIcon;
        }
        else {
            itemIcon = m_model->data(idx, Qt::DecorationRole).value<QPixmap>();
        }

        QVariant foregroundRole = m_model->data(idx, Qt::ForegroundRole);
        if (foregroundRole.canConvert<QPalette::ColorRole>()) {
            item->overrideForegroundRole(foregroundRole.value<QPalette::ColorRole>());
            resetForegroundRole = false;
        }

        QVariant editRole = m_model->data(idx, Qt::EditRole);
        if (!editRole.isNull()) {
            secondLineText = editRole.toString();
            if (relIdx == 0) {
                // Only show second line for center item
                secondLineVisible = true;
            }
        }
    }

    item->setData(itemText, itemIcon, secondLineText, secondLineVisible);
    if (resetForegroundRole) {
        item->resetForegroundRole();
    }
}

QPoint ScrollableMenu::getIndexLocation(MenuItemWidget* item, float idx, bool secondLineCanBeVisible, int fontSize, float radiusRatio) {
    const int screenRadius = height() / 2;
    const int labelCircleRadius = screenRadius * radiusRatio;
    const int labelCircleCenterY = height() / 2;
    const int labelCircleCenterX = width() * 0.65;
    const float labelMaxAngleRad = 0.3f * M_PI;
    const float labelAnchorRatio = 0.25f;  // How far into the label (from left to right) to anchor the label onto the virtual circle

    float labelAngle = labelMaxAngleRad - (idx * (2.0f * labelMaxAngleRad / (m_numVisibleItems - 1)));
    int labelAnchorX = labelCircleCenterX - (labelCircleRadius * cos(labelAngle));
    int labelAnchorY = labelCircleCenterY - (labelCircleRadius * sin(labelAngle));

    // Compute the size of the label for the requested fontSize
    QSize size = item->computeSize(secondLineCanBeVisible, fontSize);

    int labelX = labelAnchorX - (size.width() * labelAnchorRatio);
    int labelY = labelAnchorY - (size.height() / 2);

    return QPoint(labelX, labelY);
}

int ScrollableMenu::getFontPointFromIdx(float idx) {
    int fontSize = m_selectedFontSize;
    if (idx > m_numVisibleItems / 2) {
        fontSize -= (idx - (m_numVisibleItems/2)) * m_fontDecreaseRate;
    }
    else if (idx < m_numVisibleItems / 2) {
        fontSize -= ((m_numVisibleItems/2) - idx) * m_fontDecreaseRate;
    }

    return fontSize;
}

void ScrollableMenu::recomputeLabelPositioning() {
    const int titleCenterY = height() / 2;
    const int titleCenterX = width() * 0.65;

    for (int i = 0; i < m_numVisibleItems; i++) {
        menuItems[i]->move(getIndexLocation(menuItems[i], i, i == m_numVisibleItems / 2));
        menuItems[i]->resetOpacity();
    }
    menuItems[m_numVisibleItems]->setVisible(false);  // Last label is a utility label used during animations, hide after animations

    // Position the title/selection pointer
    moveCenter(title, titleCenterX, titleCenterY);
    moveCenter(selectionPtr, selectionPtrXPos, titleCenterY - 3);
}

} // namespace WingletUI
