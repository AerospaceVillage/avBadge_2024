#include "menuitemwidget.h"
#include "winglet-ui/theme.h"

#include <QGraphicsOpacityEffect>

namespace WingletUI {

MenuItemWidget::MenuItemWidget(QWidget *parent)
    : QWidget{parent}
{
    layout = new QGridLayout(this);
    iconLabel = new QLabel(this);
    textLabel = new ResizableLabel(this);
    secondLineLabel = new ResizableLabel(this);

    iconOpacityEffect = new QGraphicsOpacityEffect(iconLabel);
    iconLabel->setGraphicsEffect(iconOpacityEffect);
    textOpacityEffect = new QGraphicsOpacityEffect(textLabel);
    textLabel->setGraphicsEffect(textOpacityEffect);
    secondLineOpacityEffect = new QGraphicsOpacityEffect(secondLineLabel);
    secondLineLabel->setGraphicsEffect(secondLineOpacityEffect);

    layout->addWidget(iconLabel, 0, 0, Qt::AlignCenter);
    layout->addWidget(textLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(secondLineLabel, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    layout->setHorizontalSpacing(iconTextSpacing);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);

    iconLabel->setAlignment(Qt::AlignCenter);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    textLabel->setFont(QFont(activeTheme->standardFont));
    resetForegroundRole();

    setLayout(layout);
}

MenuItemWidget::~MenuItemWidget()
{
    delete layout;
    delete iconLabel;
    delete textLabel;
    delete secondLineLabel;
}

void MenuItemWidget::regenSize()
{
    if (blank())
        return;

    int maxWidth = maxContentsWidth;

    QSize iconSize = computeIconSize();
    if (!iconSize.isEmpty())
    {
        iconLabel->resize(iconSize);
        iconLabel->setFixedSize(iconSize);
        iconLabel->setVisible(true);
        layout->setHorizontalSpacing(iconTextSpacing);
        maxWidth -= iconSize.width() + iconTextSpacing;
    }
    else {
        iconLabel->setVisible(false);
        layout->setHorizontalSpacing(0);
    }

    QSize labelSize = computeTextLabelSize(maxWidth);
    textLabel->resize(labelSize);
    textLabel->setFixedSize(labelSize);
    if (maxWidth > 0)
        textLabel->maxContentsWidth = maxWidth;

    QSize secondLineSize = computeSecondLineSize(secondLineOpacityEffect->opacity() > 0, maxWidth);
    if (!secondLineSize.isEmpty()) {
        secondLineLabel->resize(secondLineSize);
        secondLineLabel->setFixedSize(secondLineSize);
        secondLineLabel->setVisible(true);
    }
    else {
        secondLineLabel->setVisible(false);
    }
    QSize fullSize = computeSize(labelSize, iconSize, secondLineSize);
    resize(fullSize);
    setFixedSize(fullSize);
}

QSize MenuItemWidget::computeSize(bool showSecondLine, int fontPointSize)
{
    int maxWidth = maxContentsWidth;
    QSize iconSize = computeIconSize();
    if (!iconSize.isEmpty())
        maxWidth -= iconSize.width() + iconTextSpacing;

    QSize secondLineSize = computeSecondLineSize(showSecondLine, maxWidth);
    return computeSize(computeTextLabelSize(maxWidth, fontPointSize), iconSize, secondLineSize);
}

QSize MenuItemWidget::computeSize(QSize textLabelSize, QSize iconSize, QSize secondLineSize)
{
    QSize fullSize = textLabelSize;
    if (!secondLineSize.isEmpty() && secondLineSize.width() > fullSize.width()) {
        fullSize.setWidth(secondLineSize.width());
    }

    if (!iconSize.isEmpty())
    {
        if (iconSize.height() > fullSize.height()){
            fullSize.setHeight(iconSize.height());
        }
        fullSize.setWidth(iconTextSpacing + iconSize.width() + fullSize.width());
    }

    if (!secondLineSize.isEmpty()) {
        fullSize.setHeight(fullSize.height() + secondLineSize.height());
    }

    return fullSize;
}

void MenuItemWidget::setData(const QString &text, const QPixmap &icon, const QString &secondLine, bool secondLineVisible)
{
    textLabel->setText(text);
    iconLabel->setPixmap(icon);
    secondLineLabel->setText(secondLine);
    secondLineOpacityEffect->setOpacity(secondLineVisible ? 1 : 0);
    regenSize();
}

QSize MenuItemWidget::computeTextLabelSize(int maxWidth, int fontPointSize)
{
    QSize size = textLabel->calcSingleLineSize(maxWidth, fontPointSize);
    size.rwidth()++;  // Add single pixel to size so we don't get weird rendering bugs
    return size;
}

QSize MenuItemWidget::computeIconSize()
{
    QPixmap pixmap = iconLabel->pixmap(Qt::ReturnByValue);
    return pixmap.isNull() ? QSize() : pixmap.size();
}

QSize MenuItemWidget::computeSecondLineSize(bool canShowSecondLine, int maxWidth)
{
    if (secondLineLabel->text().isEmpty())
        return {};
    if (!canShowSecondLine)
        return {};
    QSize size = secondLineLabel->calcSingleLineSize(maxWidth);
    size.rwidth()++;  // Add single pixel to size so we don't get weird rendering bugs
    return size;
}

float MenuItemWidget::opacity() const
{
    return textOpacityEffect->opacity();
}

void MenuItemWidget::setOpacity(float val) {
    textOpacityEffect->setOpacity(val);
    iconOpacityEffect->setOpacity(val);
}

void MenuItemWidget::resetOpacity()
{
    textOpacityEffect->setOpacity(1.0);
    iconOpacityEffect->setOpacity(1.0);
}

float MenuItemWidget::secondLineOpacity() const
{
    return secondLineOpacityEffect->opacity();
}

void MenuItemWidget::setSecondLineOpacity(float val) {
    secondLineOpacityEffect->setOpacity(val);
    regenSize();
}

void MenuItemWidget::overrideForegroundRole(QPalette::ColorRole role)
{
    textLabel->setForegroundRole(role);
}

void MenuItemWidget::resetForegroundRole()
{
    textLabel->setForegroundRole(QPalette::HighlightedText);
}

void ResizableLabel::setFontSize(float pointSizeF) {
    if (fontSize() == pointSizeF)
        return;
    else if (pointSizeF < 1)
        pointSizeF = 1;

    QFont newFont = font();
    newFont.setPointSizeF(pointSizeF);
    setFont(newFont);

    QSize targetSize = calcSingleLineSize(maxContentsWidth);
    targetSize.rwidth()++;  // Add single pixel to size so we don't get weird rendering bugs
    resize(targetSize);
    setFixedSize(targetSize);
}

} // namespace WingletUI
