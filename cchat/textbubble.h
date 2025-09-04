#ifndef TEXTBUBBLE_H
#define TEXTBUBBLE_H
#include <QTextEdit>//文本的管理类
#include <QHBoxLayout>
#include "bubbleframe.h"

class TextBubble: public BubbleFrame//气泡----文本气泡
{
    Q_OBJECT
public:
    TextBubble(ChatRole role,const QString &text,QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *o,QEvent *e);
private:
    void adjustTextHeight();
    void setPlainText(const QString &text);
    void initStyleSheet();
private:
    QTextEdit *m_pTextEdit;
};

#endif // TEXTBUBBLE_H
