#include "widget.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator qtTranslator;
    QTranslator myappTranslator;

    // i18n
    qtTranslator.load( "qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath) );
    myappTranslator.load( "mfs_" + QLocale::system().name() );

    a.installTranslator( &qtTranslator );
    a.installTranslator( &myappTranslator );

    Widget w;

    w.show();

    return a.exec();
}
