#ifndef PRINTMSG_H
#define PRINTMSG_H

#define QT

#include <iostream>
#ifdef QT
#include <QDebug>
#include <QString>
#endif
#define UNUSED(expr) do { (void)(expr); } while (0)

using namespace std;
void printMsg(string);
void printProgress(string);
#endif // PRINTMSG_H
