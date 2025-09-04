#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include "global.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //主函数中需要调用qss文件（设置了相应的格式）
    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){ //如果打开了qss文件（并且为只读的方式）
        qDebug("open success!");
        QString style = QLatin1String(qss.readAll());  //读的时候返回的类型是QByteArray readAll()->转换成String的形式(便于处理)
        a.setStyleSheet(style);
        qss.close();
    }else{
        qDebug("open failed!");
    }

    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    qDebug() << "Application path:" << app_path;
    // 拼接文件名
    QString fileName = "config.ini";

    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);
    qDebug() << "Config path:" << config_path;

    //检查文件权限
    QFile file(config_path);
    if(file.open(QIODevice::ReadOnly)) {
        qDebug() << "文件可读，内容前50字节:" << file.read(50);
        file.close();
    } else {
        qCritical() << "无法打开文件，错误:" << file.errorString();
    }
    //
    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();

    gate_url_prefix = "http://"+gate_host+":"+gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
