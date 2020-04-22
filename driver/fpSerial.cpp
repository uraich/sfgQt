/*
 * serial driver for fingerprint module
 * copyright U. Raich, Dec 2018
 */

#include "fpSerial.h"
#include "fpProtocol.h"
#include "printmsg.h"

void FPSerial::open_serial(int *handle, string port, int *baudrate, int *errCode){
    serialPort = new QSerialPort();
    QString portname = QString::fromStdString(port);
    if (debug)
        qDebug() << "Trying to open port " << portname;
    serialPort->setPortName(portname);
    serialPort->setBaudRate(static_cast<qint32>(*baudrate));

    if (serialPort->open(QIODevice::ReadWrite)) {
        *errCode = FP_SUCCESS;
        *handle = serialPort->handle();
        correctHandle = *handle;
        if (debug)
            qDebug() << "open_serial: Handle: " << *handle;
    }

}

void FPSerial::close_serial(int *handle, int *errCode)  {
    if (*handle == correctHandle) {
        serialPort->close();
        *errCode = FP_SUCCESS;
        *handle = 0;
    }
    else
        *errCode = FP_BAD_HANDLE;
    return ;
}

void FPSerial::write_serial(int *handle, char *buf, int *size, int *errCode) {
    qint64 retCode;
    QString hexString=QString("");
    if (*handle != correctHandle) {
        if (debug)
            qDebug() << "write_serial: Handle: " << *handle;
        *errCode = FP_BAD_HANDLE;
        return;
    }
    if (debug) {
        char *bufPtr=buf;
        qDebug() << "write_serial: The incoming data buffer:";
        for (int i=0; i<*size;i++) {
            if ((i!=0) && (!(i%16))) {
                qDebug().noquote() << hexString;
                hexString=QString("");
            }
            hexString.append(QString("0x%1 ").arg(static_cast<uint8_t>(*bufPtr++),2,16,QLatin1Char('0')));
        }
        qDebug().noquote() << hexString;
    }

    if ((retCode =serialPort->write(buf,static_cast<qint64>(*size))) == static_cast<qint64>(*size))
        *errCode= FP_SUCCESS;
    else
        *errCode = FP_READ_ERROR;
    return;
}

void FPSerial::read_serial(int *handle, char *buf, int *size, int *errCode) {
    QString hexString;
    char *bufPtr = buf;
    if (*handle != correctHandle) {
        if (debug)
            qDebug() << "read_serial: Handle: " << *handle;
        *errCode = FP_BAD_HANDLE;
        return;
    }
    int totalNoOfCharsRead=0;

    while (noOfCharsRead != *size) {
        if (!serialPort->waitForReadyRead(5000)) {
            if (debug) {
                if (totalNoOfCharsRead==0) {
                    qDebug() << "read_serial: No characters read";
                }
                char *bufPtr=buf;
                hexString=QString("");
                for (int i=0; i<totalNoOfCharsRead;i++) {
                    if ((i!=0) && (!(i%16))) {
                        qDebug().noquote() << hexString;
                        hexString=QString("");
                    }
                    hexString.append(QString("0x%1 ").arg(static_cast<uint8_t>(*bufPtr++),2,16,QLatin1Char('0')));
                }
                qDebug().noquote()  << hexString;
            }
            *errCode = FP_READ_TIMEOUT;
            return;
        }
        int noOfCharsRead = static_cast<int>(serialPort->read(bufPtr,*size-totalNoOfCharsRead));
        totalNoOfCharsRead += noOfCharsRead;
        bufPtr += noOfCharsRead;

        if (totalNoOfCharsRead == *size) {
            if (debug)
                qDebug() << "read_serial: data buffer correctly filled with " << totalNoOfCharsRead << " chars";
            *errCode = FP_SUCCESS;
            if (debug) {
                char *bufPtr=buf;
                hexString="";
                for (int i=0; i<totalNoOfCharsRead;i++) {
                    if ((i!=0) && (!(i%16))) {
                        qDebug().noquote() << hexString;
                        hexString=QString("");
                    }
                    hexString.append(QString("0x%1 ").arg(static_cast<uint8_t>(*bufPtr++),2,16,QLatin1Char('0')));
                }
                qDebug().noquote() << hexString;
            }
            return;
        }
    }
}
