#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // adding qdarkstyle theme from directory
    QFile f("qdarkstyle/dark/darkstyle.qss");

    if (!f.exists())   {
        printf("Unable to set stylesheet, file not found\n");
    }
    else   {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        a.setStyleSheet(ts.readAll());
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "harrington-ide_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return QApplication::exec();
}
