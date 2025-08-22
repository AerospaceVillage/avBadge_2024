#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QFrame>

class ElidedLabel : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool multiline READ multiline WRITE setMultiline)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode)

public:
    explicit ElidedLabel(QWidget *parent = nullptr);

    void setText(const QString &text);
    const QString& text() const { return m_text; }

    void setMultiline(bool on);
    bool multiline() const { return m_multiline; }

    void setAlignment(Qt::Alignment align);
    Qt::Alignment alignment() const { return m_alignment; }

    void setElideMode(Qt::TextElideMode mode);
    Qt::TextElideMode elideMode() const { return m_elideMode; }

    int calcSingleLineHorizAdvance(int maxWidth = 0);  // This is sort of a hacky function and only for single line mode
    QSize calcSingleLineSize(int maxWidth = 0, int overrideFontPointSize = -1);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_multiline;
    Qt::Alignment m_alignment;
    Qt::TextElideMode m_elideMode;
    QString m_text;

    int calcLineStart(int textWidth);
};

#endif // ELIDEDLABEL_H
