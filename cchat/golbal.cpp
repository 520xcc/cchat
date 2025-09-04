#include "global.h"


QString gate_url_prefix = "";
//。h文件中声明的，这里用lambda表达式给它赋值
std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w); //将样式给卸掉
    w->style()->polish(w);   //将样式给装上
};
//然后在register.cpp中写一个刷新的逻辑

//简单加密的算法
std::function<QString(QString)> xorString = [](QString input){
    QString result = input; // 复制原始字符串，以便进行修改
    int length = input.length(); // 获取字符串的长度
    ushort xor_code = length % 255;
    for (int i = 0; i < length; ++i) {
        // 对每个字符进行异或操作
        // 注意：这里假设字符都是ASCII，因此直接转换为QChar
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ xor_code));
    }
    return result;
};
