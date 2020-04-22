#include "sfgQt.h"
#include <QStatusBar>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SfgQt sfgQt;
    sfgQt.show();
    sfgQt.statusBar()->showMessage("Please open serial line before starting");
    return a.exec();
}
