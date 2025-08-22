#ifndef CIRCULARKEYBOARD_H
#define CIRCULARKEYBOARD_H

#include <QLabel>
#include <QValidator>
#include <QTimer>
#include "winglet-ui/widget/elidedlabel.h"

class CircularKeyboard: public QWidget {
    Q_OBJECT

    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY entryComplete)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(bool allowEmptyInput READ allowEmptyInput WRITE setAllowEmptyInput)
    Q_PROPERTY(bool passwordMaskEnable READ passwordMaskEnable WRITE setPasswordMaskEnable)
    Q_PROPERTY(QString validatorFailedMsg READ validatorFailedMsg WRITE setValidatorFailedMsg)
    Q_PROPERTY(QString prompt READ prompt WRITE setPrompt)
    Q_PROPERTY(QString title READ title WRITE setTitle)

public:
    explicit CircularKeyboard(
        QList<QString> keyboardChars,
        QWidget* parent = nullptr);

    bool entrySuccessful() { return m_entrySuccessful; }
    QString value() { return userInputString; }
    void setValue(const QString& val);

    int maxLength() const { return m_maxLength; }
    void setMaxLength(int len);

    int allowEmptyInput() const { return m_allowEmptyInput; }
    void setAllowEmptyInput(bool allow) { m_allowEmptyInput = allow; }

    bool passwordMaskEnable() const { return m_passwordMaskEnable; }
    void setPasswordMaskEnable(bool en);

    const QValidator* validator() const { return m_validator; }
    void setValidator(const QValidator* validator) { m_validator = validator; }

    QString validatorFailedMsg() const { return m_validatorFailedMsg; }
    void setValidatorFailedMsg(const QString& msg) { m_validatorFailedMsg = msg; }

    QString prompt() const { return m_prompt; }
    void setPrompt(const QString &prompt);

    QString title() const { return titleBox->text(); }
    void setTitle(const QString &title) { titleBox->setText(title); }

    static const QList<QString> fullKeyboard;

signals:
    void entryComplete(QString data);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *ev) override;

private slots:
    void onSetKeyboardNext();
    void onSetKeyboardPrev();
    void onEntrySelectionNext();
    void onEntrySelectionPrev();
    void onEntrySelectionConfirm();
    void onEntrySelectionBackspace();
    void onPasswordHideCharTimeout();

private:
    QList<QString> keyboardChars;
    int selectedKbdIdx;
    double keysAngleIncrement;
    double keysStartAngle;

    bool m_allowEmptyInput = false;
    bool m_passwordMaskEnable = false;
    int m_maxLength = -1;
    const QValidator* m_validator = NULL;
    QString m_validatorFailedMsg = "Input does not match required criteria";

    bool m_entrySuccessful = false;
    bool passwordShowLastChar = false;
    QTimer passwordHideCharTimeout;

    QList<QWidget*> character_widgets;
    QWidget* selectedEntry;
    int selectedEntryIdx;
    QWidget* lastSelectedEntry;
    int lastSelectedEntryIdx;

    ElidedLabel* titleBox;
    ElidedLabel* textBox;
    QLabel* instructionsBox;
    QString userInputString;
    QString m_prompt = "Enter Data:\n";

    QLabel *avLogoLabel;

    QFont standardFont;
    QFont highlightedFont;

    void redrawKeyboard(double targetAngle = 0);
    void refreshSelectedIndex();
    void refreshTextBox();
    void fixIndexCenter(int idx);
};



#endif //CIRCULARKEYBOARD_H
