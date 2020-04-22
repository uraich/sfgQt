/* ====================================================================== */
/* Serial communication include file for fingerprint program              */
/* Copyright U. Raich March 2019                                          */
/* This code is released under GPL                                        */
/* It is part of a project for the course on embedded systems at the      */
/* University of Cape Coast, Ghana                                        */
/* ====================================================================== */
#ifndef FP_SERIAL_H
#define FP_SERIAL_H

#define FP_SUCCESS                     0
#define FP_BAD_OPEN                   -1
#define FP_BAD_HANDLE                 -2
#define FP_ILLEGAL_BAUDRATE           -3
#define FP_ILLEGAL_PORT               -4
#define FP_WRITE_ERROR                -5
#define FP_READ_ERROR                 -6
#define FP_READ_TIMEOUT               -7

#define BAUDRATE                   57600

#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <QtSerialPort/QSerialPort>
#include <QDebug>

using namespace std;

class FPSerial{
public:
    struct termios oldtio,newtio;
    int noOfCharsRead,noOfCharsWritten;
    int correctHandle;
    bool debug=false;
    /**
    * open_serial function
    * @param[out] file descriptor
    * @param[in] port number (0..3)
    * @param[in] baud rate
    * @param[out] error code
    */
    void open_serial(int *handle, string port, int *baudrate, int *errCode);
    /**
    * close_serial function
    * @param[in] file descriptor
    * @param[out] error code
    */
    void close_serial(int *handle, int *errCode);
    void write_serial(int *handle, char *str, int *size, int *errCode);
    void read_serial(int *handle,char *buf, int *size, int *errCode);
private:
    QSerialPort *serialPort;
};
#endif /* FP_SERIAL_H */

