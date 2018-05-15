#include "monitor.h"
#include <QApplication>
#include <QTranslator>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QTextCodec* c = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(c);
    QApplication a(argc, argv);
    Monitor w;
    w.show();

    return a.exec();
}
