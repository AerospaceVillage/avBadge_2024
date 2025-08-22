#include "elidedlabel.h"
#include <QTextLine>
#include <QTextLayout>
#include <QPainter>
#include <QVector>
#include <QStyle>

ElidedLabel::ElidedLabel(QWidget *parent)
    : QFrame(parent), m_multiline(false), m_alignment(Qt::AlignLeft | Qt::AlignTop),
      m_elideMode(Qt::ElideRight), m_text("")
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ElidedLabel::setText(const QString &newText)
{
    m_text = newText;
    update();
}

void ElidedLabel::setMultiline(bool on) {
    if (on != m_multiline) {
        m_multiline = on;
        update();
    }
}

void ElidedLabel::setAlignment(Qt::Alignment align) {
    if (align != m_alignment) {
        m_alignment = align;
        update();
    }
}

void ElidedLabel::setElideMode(Qt::TextElideMode mode) {
    if (mode != m_elideMode) {
        m_elideMode = mode;
        update();
    }
}

int ElidedLabel::calcLineStart(int textWidth) {
    if (m_alignment & Qt::AlignHCenter) {
        int start = (width() - textWidth) / 2;
        if (start < 0)
            start = 0;
        return start;
    }
    else if (m_alignment & Qt::AlignRight) {
        int start = width() - textWidth;
        if (start < 0)
            start = 0;
        return start;
    }
    else {
        return 0;
    }
}

int ElidedLabel::calcSingleLineHorizAdvance(int maxWidth) {
    if (m_multiline) {
        qWarning("ElidedLabel::calcSingleLineHorizAdvance: Called in multiline mode. This may result in invalid measurements");
    }

    if (maxWidth < 1) {
        maxWidth = width();
    }

    QFontMetrics fontMetrics(font());
    QString elidedLastLine = fontMetrics.elidedText(m_text, m_elideMode, maxWidth);
    return fontMetrics.horizontalAdvance(elidedLastLine);
}

QSize ElidedLabel::calcSingleLineSize(int maxWidth, int overrideFontPointSize) {
    if (m_multiline) {
        qWarning("ElidedLabel::calcSingleLineHorizAdvance: Called in multiline mode. This may result in invalid measurements");
    }

    if (maxWidth < 1) {
        maxWidth = width();
    }

    QFont curFont = font();
    if (overrideFontPointSize > 0)
        curFont.setPointSize(overrideFontPointSize);

    QFontMetrics fontMetrics(curFont);
    QString elidedLastLine = fontMetrics.elidedText(m_text, m_elideMode, maxWidth);
    return {fontMetrics.horizontalAdvance(elidedLastLine), fontMetrics.height()};
}

void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    // If no text, don't draw anything
    if (m_text.size() == 0) {
        return;
    }

    QPainter painter(this);
    QFontMetrics fontMetrics = painter.fontMetrics();

    if (m_multiline) {
        int lineSpacing = fontMetrics.lineSpacing();
        int renderedHeight = 0;

        QString dat = m_text;
        if (dat.endsWith('\n')) {
            dat += "\u00A0";
        }

        QTextLayout textLayout(dat, painter.font());
        textLayout.beginLayout();
        QVector<QTextLine> lines;
        forever {
            QTextLine line = textLayout.createLine();

            if (!line.isValid())
                break;

            line.setLineWidth(width());
            int newlineIdx = dat.midRef(line.textStart(), line.textLength()).indexOf('\n');
            if (newlineIdx >= 0) {
                line.setNumColumns(newlineIdx);
            }
            renderedHeight += lineSpacing;

            lines.push_back(line);
            if (height() < renderedHeight + lineSpacing) {
                break;
            }
        }

        int y = 0;
        if (m_alignment & Qt::AlignVCenter) {
            y = (height() - renderedHeight) / 2;
        }
        else if (m_alignment & Qt::AlignBottom) {
            y = (height() - renderedHeight);
        }

        for (int i = 0; i < lines.size(); i++) {
            QTextLine &line = lines[i];

            int nextLineY = y + lineSpacing;
            if (i < lines.size() - 1) {
                int x = 0;
                if (m_alignment & (Qt::AlignHCenter | Qt::AlignRight)) {
                    // Saves calculating font metrics unless we need to
                    x = calcLineStart(line.naturalTextWidth());
                }
                line.draw(&painter, QPoint(x, y));
                y = nextLineY;
            } else {
                QString lastLine = m_text.mid(line.textStart());
                QString elidedLastLine = fontMetrics.elidedText(lastLine, m_elideMode, width());
                int x = 0;
                if (m_alignment & (Qt::AlignHCenter | Qt::AlignRight)) {
                    // Saves calculating font metrics unless we need to
                    x = calcLineStart(fontMetrics.horizontalAdvance(elidedLastLine));
                }
                painter.drawText(QPoint(x, y + fontMetrics.ascent()), elidedLastLine);
                line = textLayout.createLine();
                break;
            }
        }
        textLayout.endLayout();
    }
    else {
        int y = 0;
        if (m_alignment & Qt::AlignVCenter) {
            y = (height() - fontMetrics.lineSpacing()) / 2;
        }
        else if (m_alignment & Qt::AlignBottom) {
            y = (height() - fontMetrics.lineSpacing());
        }

        QString elidedLastLine = fontMetrics.elidedText(m_text, m_elideMode, width());
        int x = 0;
        if (m_alignment & (Qt::AlignHCenter | Qt::AlignRight)) {
            // Saves calculating font metrics unless we need to
            x = calcLineStart(fontMetrics.horizontalAdvance(elidedLastLine));
        }
        painter.drawText(QPoint(x, y + fontMetrics.ascent()), elidedLastLine);
    }
}
