#ifndef WINGLETUI_MENUITEMWIDGET_H
#define WINGLETUI_MENUITEMWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include "winglet-ui/widget/elidedlabel.h"

class QGraphicsOpacityEffect;

namespace WingletUI {

class ResizableLabel: public ElidedLabel {
    Q_OBJECT

    Q_PROPERTY(float fontSize READ fontSize WRITE setFontSize)

public:
    explicit ResizableLabel(QWidget *parent = nullptr): ElidedLabel(parent) {}
    virtual ~ResizableLabel() {}

    float fontSize() const { return font().pointSizeF(); }
    void setFontSize(float pointSizeF);

    int maxContentsWidth = 480;
};


class MenuItemWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(float fontSize READ fontSize WRITE setFontSize)
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(float secondLineOpacity READ secondLineOpacity WRITE setSecondLineOpacity)

public:
    explicit MenuItemWidget(QWidget *parent = nullptr);
    ~MenuItemWidget();

    bool blank() const { return textLabel->text().size() == 0; }
    float fontSize() const { return textLabel->fontSize(); }

    float opacity() const;
    void setOpacity(float val);
    void resetOpacity();

    float secondLineOpacity() const;
    void setSecondLineOpacity(float val);

    void setData(const QString &text, const QPixmap &icon = {}, const QString &secondLine = "", bool secondLineVisible = false);
    void setFontSize(float pointSizeF) { textLabel->setFontSize(pointSizeF); regenSize(); }
    QSize computeSize(bool showSecondLine, int fontPointSize = -1);  // Computes size, if fontPointSize == -1, it uses the current label font size
    QSize computeSize(QSize textLabelSize, QSize iconSize, QSize secondLineSize);
    QSize computeIconSize();
    QSize computeTextLabelSize(int maxWidth, int fontPointSize = -1);
    QSize computeSecondLineSize(bool canShowSecondLine, int maxWidth);

    void overrideForegroundRole(QPalette::ColorRole role);
    void resetForegroundRole();
    bool hasSecondLine() { return !secondLineLabel->text().isEmpty(); }

    void setMaxContentsWidth(int maxWidth) { maxContentsWidth = maxWidth; regenSize(); }

private:
    QGridLayout *layout;
    QLabel *iconLabel;
    QGraphicsOpacityEffect* iconOpacityEffect;
    ResizableLabel *textLabel;
    QGraphicsOpacityEffect* textOpacityEffect;
    ElidedLabel *secondLineLabel;
    QGraphicsOpacityEffect* secondLineOpacityEffect;

    void regenSize();

    int maxContentsWidth = 480;
    const int iconTextSpacing = 10;

signals:

};

} // namespace WingletUI

#endif // WINGLETUI_MENUITEMWIDGET_H
