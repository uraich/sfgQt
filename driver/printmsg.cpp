#include "printmsg.h"

void printMsg(string msg) {
#ifdef QT
    qDebug().noquote() << QString::fromStdString(msg);
#else
    cout << msg << endl;
#endif
}
void printProgress(string msg) {

#ifndef QT  /* in Qt we have the progress bar and don't need this */
    cout << msg;
    cout.flush();
#else
    UNUSED(msg);
#endif
}
