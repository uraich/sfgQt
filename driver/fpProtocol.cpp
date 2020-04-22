#include "fpProtocol.h"
#include "fpSerial.h"
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>
#include <string.h>

//#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define WAIT 3

FPProtocol::FPProtocol()
{
    serialLine = new(FPSerial);

}

void FPProtocol::fpSetDebug(bool onOff) {
    debug = onOff;
    serialLine->debug = onOff;
}

bool FPProtocol::fpDebugIsOn() {
    return debug;
}

void FPProtocol::fpEnableDangerous(bool onOff) {
    dangerous = onOff;
}

int FPProtocol::fpInit(string port, uint8_t br) {
    int errCode;
    int baudrate=br*9600;
    if (debug)
        printMsg("----------------- fpInit: -------------------");
    serialLine->open_serial(&handle,port,&baudrate,&errCode);
    return errCode;
}

int FPProtocol::fpClose() {
    int errCode;
    serialLine->close_serial(&handle,&errCode);
    return errCode;
}

int FPProtocol::fpCreatePacket(uint16_t header,
                               uint32_t address,
                               uint8_t  identifier,
                               uint16_t length,
                               uint8_t  *data,
                               uint8_t *packet) {

    if (debug)
        printMsg("fpCreatePacket: ");
    uint8_t *packetPtr,*dataPtr,*tmpDataPtr;
    uint16_t checksum;
    int i;

    packetPtr = packet;
    /* insert header */
    *packetPtr++ = header >> 8;
    *packetPtr++ = header & 0xff;
    /* insert address */
    *packetPtr++ = address >> 24;
    *packetPtr++ = (address >> 16) & 0xff;
    *packetPtr++ = (address >> 8) & 0xff;
    *packetPtr++ = address &0xff;
    /* packet identifier */
    *packetPtr++ = identifier;
    /* packet length */
    *packetPtr++ = length >>8 ;
    *packetPtr++ = length & 0xff;

    dataPtr = packetPtr;
    tmpDataPtr = data;
    for (i=0;i<length-2;i++)
        *packetPtr++ = *tmpDataPtr++;
    if (debug) {
        stringstream ss;
        ss << "Data Ptr content: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(*dataPtr);
        printMsg(ss.str());
    }

    checksum = calculateChecksum(identifier,length,dataPtr);
    if (debug) {
        stringstream ss;
        ss << "Checksum: 0x" << setfill('0') << setw(4) << hex << static_cast<int>(checksum);
        printMsg(ss.str());
    }
    *packetPtr++ = checksum >> 8;
    *packetPtr++ = checksum & 0xff;
    return FP_SUCCESS;
}

uint16_t FPProtocol::calculateChecksum(uint8_t packetIdentifier,
                                       uint16_t packetLength,
                                       uint8_t *data) {
    int i;
    uint8_t *dataPtr;
    uint16_t checksum = packetIdentifier;
    stringstream ss;
    if (debug) {
        printMsg(string("calculate Checksum:"));
        ss.str("");
        ss << "Checksum with identifier: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(checksum);
        printMsg(ss.str());
    }
    checksum += packetLength >> 8;
    checksum += packetLength & 0xff;
    if (debug) {
        ss.str("") ;
        ss << "Checksum with packetLength: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(checksum);
        printMsg(ss.str());
        ss.str("");
    }
    dataPtr = data;
    if (debug) {
        printMsg("Data in packet:");
    }
    for (i=0;i<packetLength-2;i++) {
        if (debug) {
            if ((i!=0) && (!(i%16))) {
                printMsg(ss.str());
                ss.str("");
            }
            ss <<  "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*dataPtr) << " ";
        }
        checksum += *dataPtr++;
    }
    if (debug) {
        printMsg(ss.str());
        ss.str("");
        ss << "Checksum: 0x" << setfill('0') << setw(4) << hex << checksum;
        printMsg(ss.str());
    }
    return checksum;
}

void FPProtocol::printPacket(uint8_t *packet) {
    uint8_t *packetPtr;
    uint16_t packetLength;
    uint8_t  packetIdentifier;
    uint16_t header;
    uint32_t address;
    uint16_t checksum;
    int i;

    packetPtr = packet;
    header   = static_cast<uint16_t>(*packetPtr++ << 8);
    header  |= static_cast<uint16_t>(*packetPtr++ & 0xff);
    address  = static_cast<uint32_t>(*packetPtr++ << 24);
    address |= static_cast<uint32_t>(*packetPtr++ << 16);
    address |= static_cast<uint32_t>(*packetPtr++ << 8);
    address |= *packetPtr++;
    if (debug) {
        stringstream ss;
        ss << "Header:   0x" << setfill('0') << setw(4) << hex << header;
        printMsg(ss.str());
        ss.str("");
        ss << "Address:  0x" << setfill('0') << setw(8) << hex << static_cast<uint32_t>(address);
        printMsg(ss.str());
    }
    packetIdentifier = *packetPtr++;
    switch ( packetIdentifier) {
    case FP_CMD_PACKET:
        printMsg(string("========== Packet is a command packet ========="));
        break;
    case FP_DATA_PACKET:
        printMsg(string("========== Packet is a data packet ========="));
        break;
    case FP_ACQ_PACKET:
        printMsg(string("========== Packet is an acknowledge packet ========="));
        break;
    case FP_END_PACKET:
        printMsg(string("========== Packet is an 'end of data' packet ========="));
        break;
    default:
        stringstream ss;
        ss << "I don't know this packet identifier: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(packetIdentifier);
        return;
    }
    packetLength = static_cast<uint16_t>(*packetPtr++ << 8);
    packetLength |= *packetPtr++;
    stringstream ss;
    ss << "Packet length: " << packetLength;
    printMsg(ss.str());

    if (packetIdentifier == FP_CMD_PACKET) {
        stringstream ss;
        ss << "Instruction code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(*packetPtr++);
        printMsg(ss.str());
        ss.str("");
        printMsg("Instruction parameters: ");
        for (i=0; i<packetLength-3;i++){
            if ((i!=0) && (!(i%16))) {
                printMsg(ss.str());
                ss.str("");
            }
            ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*packetPtr++) << " ";
        }
        printMsg(ss.str());
    }

    else {
        if (packetIdentifier == FP_ACQ_PACKET) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(*packetPtr);
            printMsg(ss.str());
            fpPrintError(static_cast<char>(*packetPtr++));
        }
        else {
            printMsg(string("Data:"));
            ss.str("");
            for (i=0; i<packetLength-2;i++) {
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*packetPtr++);
                if (!((i+1)%16)) {
                    printMsg(ss.str());
                    ss.str("");
                }
            }
            printMsg("ss.str()");
        }
    }
    checksum = static_cast<uint16_t>(*packetPtr++ << 8);
    checksum |= *packetPtr;
    if (debug) {
        stringstream ss;
        ss << "Checksum: 0x" << setfill('0') << setw(4) << hex << checksum;
        printMsg(ss.str());
    }
}

int FPProtocol::fpGetParameterSize(uint8_t instruction, uint16_t *cmdParLength, uint16_t *acqParLength) {
    /* the value for cmdParLength is 1 (for instruction) + size of pars */
    if (debug)
        printMsg("fpGetParameterSize:");
    switch (instruction) {
    case FP_CMD_HANDSHAKE:
    case FP_CMD_IMG2TZ:
    case FP_CMD_DOWN_CHAR:
    case FP_CMD_UP_CHAR:
        *cmdParLength  = 2;
        *acqParLength  = 1;
        break;
    case FP_CMD_SET_SYS_PARA:
        *cmdParLength  = 3;
        *acqParLength  = 1;
        break;
    case FP_CMD_SET_ADDR:
    case FP_CMD_DEL_CHAR:
        *cmdParLength  = 5;
        *acqParLength  = 1;
        break;
    case FP_CMD_STORE:
    case FP_CMD_LOAD:
        *cmdParLength = 4;
        *acqParLength = 1;
        break;
    case FP_CMD_VFY_PASSWORD:
        *cmdParLength=5;
        *acqParLength=1;
        break;
    case FP_CMD_READ_SYS_PARA:
        *cmdParLength=1;
        *acqParLength=17;
        break;
    case FP_CMD_GEN_IMAGE:
    case FP_CMD_UP_IMAGE:
    case FP_CMD_DOWN_IMAGE:
    case FP_CMD_REG_MODEL:
    case FP_CMD_EMPTY:
        *cmdParLength=1;
        *acqParLength=1;
        break;
    case FP_CMD_TEMPLATE_NUM:
        *cmdParLength=1;
        *acqParLength=3;
        break;
    case FP_CMD_READ_CON_LIST:
        *cmdParLength=2;
        *acqParLength=33;
        break;
    case FP_CMD_RANDOM:
        *cmdParLength=1;
        *acqParLength=5;
        break;
    case FP_CMD_WRITE_NOTEPAD:
        *cmdParLength=34;
        *acqParLength=1;
        break;
    case FP_CMD_READ_NOTEPAD:
        *cmdParLength=2;
        *acqParLength=33;
        break;
    case FP_CMD_MATCH:
        *cmdParLength=1;
        *acqParLength=3;
        break;
    case FP_CMD_SEARCH:
        *cmdParLength=6;
        *acqParLength=5;
        break;
    default:
        return FP_ILLEGAL_INSTRUCTION;
    }
    if(debug) {
        stringstream ss;
        ss << "Command parameter length: " << static_cast<int>(*cmdParLength) << ", acquisition parameter length: " << static_cast<int>(*acqParLength);
        printMsg(ss.str());
    }

    return FP_SUCCESS;
}

int FPProtocol::fpCreateDataPacket(uint8_t *data, uint16_t *dataPacketSize, uint8_t *packet) {
    return fpCreatePacket(FP_HEADER,
                          FP_ADDRESS,
                          FP_DATA_PACKET,
                          *dataPacketSize+2,
                          data,packet);
}

int FPProtocol::fpCreateEndPacket(uint8_t *data, uint16_t *dataPacketSize, uint8_t *packet) {
    return fpCreatePacket(FP_HEADER,
                          FP_ADDRESS,
                          FP_END_PACKET,
                          *dataPacketSize+2,
                          data,packet);
}

int FPProtocol::fpCreateCmdPacket(uint8_t instruction,
                                  uint8_t *pars,
                                  uint8_t *packet) {

    uint16_t cmdParSize,acqParSize;
    uint8_t *parBuf,*parBufPtr,*parsPtr;
    int i;
    if (debug)
        printMsg(string("fpCreatePacket:"));
    fpGetParameterSize(instruction,&cmdParSize,&acqParSize);
    /* parBuf contains the instruction code and its parameters */
    parBuf = new uint8_t[cmdParSize];
    parBufPtr = parBuf;
    *parBufPtr++ = instruction;
    if (debug) {
        stringstream ss;
        ss << "fpCreateCmdPacket: parameter size: " <<  cmdParSize;
        printMsg(ss.str());
    }

    parsPtr = pars;
    for (i=0;i<cmdParSize-1;i++)
        *parBufPtr++ = *parsPtr++;

    fpCreatePacket(FP_HEADER,FP_ADDRESS,FP_CMD_PACKET,cmdParSize+2,parBuf,packet);
    delete [] parBuf;
    return FP_SUCCESS;
}

int FPProtocol::fpSendCmd(uint8_t instruction,
                          uint8_t *pars,
                          uint8_t *acqBuf) {
    int i,errCode;
    uint8_t *packet;
    int cmdPacketLength,acqPacketLength;
    uint16_t cmdParSize,acqParSize;
    int retCode;
    if (debug)
        printMsg(string("fpSendCmd:"));

    /* get the size of the associated data in the command and acknowledge packet */
    fpGetParameterSize(instruction,&cmdParSize,&acqParSize);
    cmdPacketLength=FP_HEADER_SIZE+FP_ADDRESS_SIZE+FP_IDENTIFIER_SIZE+FP_LENGTH_SIZE;
    cmdPacketLength+=cmdParSize;
    cmdPacketLength+=FP_CHECKSUM_SIZE;
    packet = new uint8_t[cmdPacketLength];
    if (fpCreateCmdPacket(instruction,pars,packet) != FP_SUCCESS) {
        if (debug)
            printMsg(string("pSendCmd: Could not create command packet"));
        return -1;
    }

    if (debug)
        printPacket(packet);
    if (debug) {
        stringstream ss;
        ss << "Command packet size: " << cmdPacketLength;
        printMsg(ss.str());
    }
    /* send the packet to the serial line */
    serialLine->write_serial(&handle,reinterpret_cast<char *>(packet),&cmdPacketLength,&errCode);
    delete [] packet;

    /* read back the acknowledge packet */
    if (debug) {
        stringstream ss;
        ss << "Instruction: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(instruction) << ", acqParSize: " << acqParSize;
        printMsg(ss.str());

    }
    acqPacketLength=FP_BASE_PKT_SIZE+acqParSize;
    if (debug) {
        stringstream ss;
        ss << "Acknowledge packet size: " << acqPacketLength;
        printMsg(ss.str());
    }
    serialLine->read_serial(&handle,reinterpret_cast<char *>(acqBuf),&acqPacketLength,&errCode);
    if (errCode != FP_SUCCESS) {
        stringstream ss;
        ss << "Error from read_serial: " << dec << static_cast<int>(errCode);
        printMsg(ss.str());
    }

    if ((!fpIsAcknowledgePacket(acqBuf) || (debug))) {
        printMsg("Acquisition buffer: ");
        stringstream ss;
        for (i=0;i<12;i++)
            ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(acqBuf[i]) << " ";
        printMsg(ss.str());
        printPacket(acqBuf);
        if (!fpIsAcknowledgePacket(acqBuf)) {
            printMsg(string("Did not find an acknowledge packet as expected"));
            return FP_INVALID_PACKET;
        }
    }

    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg(string("Acknowledge packet is bad"));
        return FP_INVALID_PACKET;
    }

    if ((retCode = fpGetErrorCode(acqBuf)) != FP_SUCCESS) {
        switch (retCode) {
        case ERR_NO_FINGER:
        case ERR_NO_MATCH:
        case ERR_PRINTS_DONT_MATCH:
            break;
        default:
            stringstream ss;
            ss << "Error when executing command. Error code is: " << dec << retCode;
            printMsg(ss.str());
            fpPrintError(static_cast<char>(retCode));
            return retCode;
        }
    }
    return retCode;
}

int FPProtocol::fpSendData(uint8_t type, uint16_t *dataPacketSize,uint8_t *data) {            // data or end packet
    int errCode;
    uint8_t *packet;

    int dataPacketLength=FP_BASE_PKT_SIZE + *dataPacketSize;
    if (debug) {
        stringstream ss;
        ss << "Creating data packet of size " << dec << dataPacketLength;
        printMsg(ss.str());
    }

    packet = new uint8_t[dataPacketLength];
    if (debug) {
        stringstream ss;
        ss << "fpSendData for data packet size: " << *dataPacketSize;
        printMsg(ss.str());
    }
    if (type == FP_DATA_PACKET) {
        if (fpCreateDataPacket(data,dataPacketSize,packet) != FP_SUCCESS) {
            if (debug)
                printMsg(string("fpSendData: Could not create data packet"));
            delete [] packet;
//            free(packet);
            return -1;
        }
    }
    else {
        if (fpCreateEndPacket(data,dataPacketSize,packet) != FP_SUCCESS) {
            if (debug)
                printMsg(string("fpSendData: Could not create end packet"));
            delete []  packet;
//            free(packet);
            return -1;
        }
    }

    if (debug) {
        printPacket(packet);
        stringstream ss;
        ss << "Data packet size: " << static_cast<int>(dataPacketLength);
        printMsg(ss.str());
    }
    /* send the packet to the serial line */
    serialLine->write_serial(&handle,reinterpret_cast<char *>(packet),
                             &dataPacketLength,&errCode);
    delete [] packet;
//    free(packet);
    return FP_SUCCESS;
}

bool FPProtocol::fpIsAcknowledgePacket(uint8_t *packet) {
    if (packet[FP_IDENTIFIER_POS] == FP_ACQ_PACKET)
        return true;
    else {
        if (debug) {
            stringstream ss;
            ss << "Found identifier: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(packet[FP_IDENTIFIER_POS]);
            printMsg(ss.str());
        }

        return false;
    }
}

uint8_t FPProtocol::fpGetErrorCode(uint8_t *packet) {
    return packet[FP_CONF_CODE_POS];
}

int FPProtocol::fpCheckPacket(uint8_t *packet) {
    uint16_t header;
    uint8_t  *packetPtr;
    uint8_t  identifier;
    uint16_t checksumCalc,checksumRec;
    uint16_t dataLength;
    int i;

    if (debug)
        printMsg(string("fpCheckPacket"));
    packetPtr = packet;
    header = static_cast<uint16_t>(*packetPtr++ << 8);
    header |= *packetPtr++;
    /* check if the package header is correct */
    if (header != FP_HEADER)
        return FP_INVALID_PACKET;
    packetPtr += 4;
    /* calculate checksum and compare */
    identifier = *packetPtr;
    if (debug) {
        stringstream ss;
        ss << "Identifier: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(identifier);
        printMsg(ss.str());
    }
    checksumCalc = *packetPtr++;      // package identifier
    dataLength = static_cast<uint16_t>(*packetPtr << 8);   // package length
    checksumCalc += *packetPtr++;
    dataLength |= *packetPtr;
    if (debug) {
        stringstream ss;
        ss << "Data length: 0x" << setfill('0') << setw(4) << hex << dataLength;
        printMsg(ss.str());

    }
    checksumCalc += *packetPtr++;

    for (i=0; i<dataLength-static_cast<uint16_t>(sizeof(uint16_t));i++)
        checksumCalc += *packetPtr++;

    checksumRec=static_cast<uint16_t>(*packetPtr++ << 8);
    checksumRec |= *packetPtr;
    if (checksumRec == checksumCalc)
        return FP_SUCCESS;
    else {
        if (debug) {
            stringstream ss;
            ss << "Checksum calculated: 0x" << setfill('0') << setw(4) << hex << checksumCalc << ", checksum read:"  << checksumRec;
            printMsg(ss.str());
        }
        exit(-1);
        return FP_BAD_CHECKSUM;
    }
}

int FPProtocol::fpHandshake() {
    uint8_t *acqBuf;
    uint8_t cmd=0;
    uint8_t confCode;
    uint16_t cmdParLength,acqParLength;
    if (debug)
        printMsg(string("---------------- fpHandshake: --------------------"));
    fpGetParameterSize(FP_CMD_HANDSHAKE,&cmdParLength,&acqParLength);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParLength];
    fpSendCmd(FP_CMD_HANDSHAKE,&cmd,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        if (debug)
            printMsg(string("Invalid acknowledge packet"));
        delete [] acqBuf;
        return FP_INVALID_PACKET;
    }
    else {
        confCode = acqBuf[FP_CONFIRMATION_CODE_POS];
        delete[] acqBuf;
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpReadSysPara(
        uint16_t  *statusReg,
        uint16_t  *systemIdentifier,
        uint16_t  *fingerLibSize,
        uint16_t  *securityLevel,
        uint32_t  *deviceAddress,
        uint16_t  *dataPacketSize,
        uint16_t  *baudrate) {

    uint8_t *acqBuf,*acqBufPtr;
    uint8_t cmd;
    uint8_t confCode;
    uint16_t cmdParLength,acqParLength;
    uint16_t statReg,sysIdent,fingLibSze,secuLvl,dataPckSze,br;
    uint32_t devAddr;
    if (debug)
        printMsg(string("----------------- fpReadSysPara: -----------------"));
    fpGetParameterSize(FP_CMD_READ_SYS_PARA,&cmdParLength,&acqParLength);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParLength];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+cmdParLength));
    fpSendCmd(FP_CMD_READ_SYS_PARA,&cmd,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        if (debug)
            printMsg(string("Invalid acknowledge packet"));
        if (acqBuf != nullptr)
            delete [] acqBuf;
//            free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr = acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x"<< setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }

        statReg = static_cast<uint16_t>((*acqBufPtr++) << 8);
        statReg |= *acqBufPtr++;
        if (debug) {
            stringstream ss;
            ss << "Status register: 0x"<< setfill('0') << setw(4) << hex << statReg;
            printMsg(ss.str());
        }

        *statusReg=statReg;

        sysIdent = static_cast<uint16_t>((*acqBufPtr++) << 8);
        sysIdent |= *acqBufPtr++;
        *systemIdentifier=sysIdent;
        if (debug) {
            stringstream ss;
            ss << "System Identification Code: 0x"<< setfill('0') << setw(4) << hex << sysIdent;
            printMsg(ss.str());
        }

        fingLibSze =static_cast<uint16_t>((*acqBufPtr++) << 8);
        fingLibSze |= *acqBufPtr++;
        *fingerLibSize=fingLibSze;
        if (debug) {
            stringstream ss;
            ss << "Finger Lib Size: "<< setfill('0') << dec << fingLibSze;
            printMsg(ss.str());
        }

        secuLvl = static_cast<uint16_t>((*acqBufPtr++) << 8);
        secuLvl |= *acqBufPtr++;
        *securityLevel = secuLvl;
        if (debug) {
            stringstream ss;
            ss << "Security Level: "<< setfill('0') << dec << secuLvl;
            printMsg(ss.str());
        }

        devAddr =static_cast<uint32_t>(*acqBufPtr++ << 24);
        devAddr|=static_cast<uint32_t>(*acqBufPtr++ << 16);
        devAddr|=static_cast<uint32_t>(*acqBufPtr++ <<  8);
        devAddr|=*acqBufPtr++;
        *deviceAddress = devAddr;
        if (debug) {
            stringstream ss;
            ss << "Device address: 0x" << setfill('0') << setw(8) << hex << devAddr;
            printMsg(ss.str());
        }


        dataPckSze = static_cast<uint16_t>((*acqBufPtr++) << 8);
        dataPckSze |= *acqBufPtr++;
        *dataPacketSize = dataPckSze;
        if (debug) {
            stringstream ss;
            ss << "Data packet size: "<< setfill('0') << dec << dataPckSze;
            printMsg(ss.str());
        }

        br = static_cast<uint16_t>(*acqBufPtr++ << 8);
        br |= *acqBufPtr++;
        *baudrate = br;
        if (debug) {
            stringstream ss;
            ss << "Baud rate: "<< setfill('0') << dec << br*9600;
            printMsg(ss.str());
        }
        delete [] acqBuf;
//        free(acqBuf);
        return confCode;

    }
}


void FPProtocol::printDangerousWarning() {
    printMsg(string("This call would change the device address."));
    printMsg("Even though the code is implemented we do not recommend to execute it");
    printMsg("if it is not absolutely necessary");
    printMsg("Changing the device address and/or its baud rate may make the modules unaccessible");
    printMsg("and I do not know how to perform a factory reset");
}

int FPProtocol::fpSetAddr(uint16_t /* address */) {
    if (!dangerous) {
        printDangerousWarning();
        return FP_DANGEROUS;
    }

    printMsg("Not implemented yet");
    return FP_SUCCESS;
}

int FPProtocol::fpSetSysPara(uint8_t type, uint8_t value) {

    int retCode;
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t pars[2];
    uint16_t cmdParSize,acqParSize;
    pars[0] = type;

    if (debug) {
        printMsg("---------------- fpSetSysPara: ------------------");
        stringstream ss;
        ss << "type:" << type << ", value: " << value;
        printMsg(ss.str());
    }

    if ((type < 4) || (type > 6)) {
        stringstream ss;
        ss << "fpSetSysPara: type " << type << "is illegal";
        printMsg(ss.str());
        return FP_ILLEGAL_PARAMETER;
    }

    switch (type) {
    /* We only use "standard" baudrates available in the Linux serial driver */
    /* non-standard baud rates may be implemented later                      */
    case FP_BAUDRATE:
        switch (value) {
        case 1:    // 9600 baud
        case 2:    // 19200 baud
        case 4:    // 38400 baud
        case 6:    // 57600 baud
        case 12:   // 115200 baud
            pars[1] = value;
            break;
        default:
            return FP_ILLEGAL_PARAMETER;
        }
        break;
    case FP_SECURITY:
        if ((value < 1) || (value > 5))
            return FP_ILLEGAL_PARAMETER;
        break;
    case FP_PACKET_SIZE:
        if (value > 4)
            return FP_ILLEGAL_PARAMETER;
        break;
    default:
        return FP_ILLEGAL_PARAMETER;
    }
    pars[0] = type;
    pars[1] = value;
    if (debug) {
        stringstream ss;
        switch (value) {
        case FP_BAUDRATE:
            ss << "Setting baudrate to " << dec << value*9600;
            printMsg(ss.str());
            break;
        case FP_SECURITY:
            ss << "Set security level to " << dec << value;
            printMsg(ss.str());
            break;
        case FP_PACKET_SIZE:
            ss << "Set packet size to " << dec << 32 <<value;
            printMsg(ss.str());
            break;
        }
    }
    fpGetParameterSize(FP_CMD_SET_SYS_PARA,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
    acqBufPtr = acqBuf;

//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+acqParameterSize));
    if ((retCode=fpSendCmd(FP_CMD_SET_SYS_PARA,pars,acqBuf)) != FP_SUCCESS) {
        delete [] acqBuf;
//        free(acqBuf);
        return retCode;
    }
    else {
        /* get the error code */
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        retCode =*acqBufPtr;
        acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
    }
    if (debug) {
        stringstream ss;
        ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << retCode;
        printMsg(ss.str());
    }
    return retCode;
}

int FPProtocol::fpTemplateNumber(uint16_t *templateNumber) {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode,templNum;
    uint16_t cmdParSize,acqParSize;
    if (debug)
        printMsg("------------------ fpTemplateNumber: ------------------");

    fpGetParameterSize(FP_CMD_TEMPLATE_NUM,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+3));

    fpSendCmd(FP_CMD_TEMPLATE_NUM,nullptr,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
        }
        templNum =static_cast<uint8_t>(*acqBufPtr++ << 8);
        templNum|=*acqBufPtr;
        if (debug) {
            stringstream ss;
            ss << "Number of valid templates: " << dec <<templNum;
            printMsg(ss.str());
        }

        *templateNumber=templNum;
        delete [] acqBuf;
//        free(acqBuf);
        return confCode;
    }
}

int FPProtocol::fpGenImg() {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    if (debug)
        printMsg("---------------- fpGenImg: ---------------------");
    fpGetParameterSize(FP_CMD_GEN_IMAGE,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_GEN_IMAGE,nullptr,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpVerifyPassword(uint32_t passwd) {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    uint32_t pw = passwd;

    if (debug)
        printMsg("------------------- fpVerifyPassword: ----------------------");
    fpGetParameterSize(FP_CMD_VFY_PASSWORD,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+5));
    fpSendCmd(FP_CMD_VFY_PASSWORD,reinterpret_cast<uint8_t *>(&pw),acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {

        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            if (confCode == FP_SUCCESS)
                printMsg("Password is correct");
            else
                printMsg("Password is wrong");
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
    }
    return confCode;
}


int FPProtocol::fpGetRandomNumber(uint32_t * randomNumber) {
    uint8_t *acqBuf,*acqBufPtr;
    uint16_t cmdParSize,acqParSize;
    uint32_t randNum;
    uint8_t confCode;
    if (debug)
        printMsg("---------------- fpGetRandomNumber: ---------------------");
    fpGetParameterSize(FP_CMD_RANDOM,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+5));
    fpSendCmd(FP_CMD_RANDOM,nullptr,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        randNum  = static_cast<uint32_t>(*acqBufPtr++ << 24);
        randNum |= static_cast<uint32_t>(*acqBufPtr++ << 16);
        randNum |= static_cast<uint32_t>(*acqBufPtr++ << 8);
        randNum |= *acqBufPtr++;;
        *randomNumber=randNum;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpUpImage() {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    if (debug)
        printMsg("----------------- fpUpImage: ---------------------");
    fpGetParameterSize(FP_CMD_UP_IMAGE,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_UP_IMAGE,nullptr,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpReadConList(uint8_t idxPage,uint8_t *templateList) {
    int retCode;
    uint8_t *acqBuf,*acqBufPtr;
    uint16_t cmdParLength,acqParLength;
    uint8_t confCode;
    uint8_t *templateListPtr;
    int i;
    if (debug)
        printMsg("------------------- fpReadConList: -------------------------");
    retCode = fpGetParameterSize(FP_CMD_READ_CON_LIST,&cmdParLength,&acqParLength);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParLength];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+acqParLength));
    retCode = fpSendCmd(FP_CMD_READ_CON_LIST,&idxPage,acqBuf);
    if ((retCode = fpCheckPacket(acqBuf)) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        templateListPtr = templateList;
        for (i=0;i<acqParLength-1;i++)
            *templateListPtr++ = *acqBufPtr++;
        /* print the values */
        templateListPtr = templateList;
        if (debug) {
            printMsg("Valid templates: ");
            stringstream ss;
            for (i=0; i<acqParLength;i++) {
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*templateListPtr++) << " ";
                if (!((i+1)%16)){
                    printMsg(ss.str());
                    ss.str("");
                }
            }
        }
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
    }
    return retCode;
}

int FPProtocol::fpReadDataBuffer(uint8_t *packetType, uint16_t *dataPacketSize, uint8_t *dataBuffer) {
    int errCode;
    uint8_t *acqBuf,*acqBufPtr;
    if (debug)
        printMsg("fpReadDataBuffer:");
    int size = FP_BASE_PKT_SIZE + *dataPacketSize;
    if (debug) {
        stringstream ss;
        ss << "reading data packet of size " << dec << size;
        printMsg(ss.str());
    }
    acqBuf = new uint8_t[size];
    serialLine->read_serial(&handle,reinterpret_cast<char *>(acqBuf),&size, &errCode);

    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Some problem with the data packet");
        delete [] acqBuf;
        return FP_INVALID_PACKET;
    }
    else {
        if (debug)
            printMsg("Packet is ok");
        *packetType=acqBuf[FP_IDENTIFIER_POS];
        acqBufPtr = acqBuf+FP_PKT_HEADER_LENGTH;
        bcopy(acqBufPtr,dataBuffer,static_cast<size_t>(*dataPacketSize));
        delete [] acqBuf;
        return FP_SUCCESS;
    }

}

int FPProtocol::fpImg2Tz(uint8_t charBuf) {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint8_t chrBuf;
    uint16_t cmdParSize,acqParSize;
    if (debug)
        printMsg("------------------ fpImg2Tz: ----------------------");
    if ((charBuf != 1) && (charBuf != 2)) {
        printMsg("Illegal char buf number, must be 1 or 2");
        return FP_ILLEGAL_CHARBUF;
    }

    chrBuf = charBuf;
    fpGetParameterSize(FP_CMD_IMG2TZ,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_IMG2TZ,&chrBuf,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpStore(uint8_t charBuf,uint16_t libIndex) {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    uint16_t libIdx;
    uint8_t pars[3];
    if (debug)
        printMsg("-------------------- fpStore: ---------------------");
    if ((charBuf != 1) && (charBuf != 2)) {
        printMsg("Illegal char buf number, must be 1 or 2");
        return FP_ILLEGAL_CHARBUF;
    }
    pars[0] = charBuf;
    libIdx = libIndex;
    pars[1] = libIdx >> 8;
    pars[2] = libIdx & 0xff;
    stringstream ss;
    ss << "Saving character file from buffer " << static_cast<int>(charBuf) << " to library at position " << dec << libIndex;
    printMsg(ss.str());

    fpGetParameterSize(FP_CMD_STORE,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_STORE,pars,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}
int FPProtocol::fpLoad(uint8_t charBuf,uint16_t libIndex) {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    uint16_t libIdx;
    uint8_t pars[3];
    if (debug)
        printMsg("--------------------- fpLoad: -----------------------");
    if ((charBuf != 1) && (charBuf != 2)) {
        printMsg("Illegal char buf number, must be 1 or 2");
        return FP_ILLEGAL_CHARBUF;
    }
    pars[0] = charBuf;
    libIdx = libIndex;
    pars[1] = libIdx >> 8;
    pars[2] = libIdx & 0xff;
    if (debug) {
        stringstream ss;
        ss << "Restoring character file from library position " << libIndex << " to character buffer " << dec << static_cast<int>(charBuf);
        printMsg(ss.str());
    }
    fpGetParameterSize(FP_CMD_LOAD,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_LOAD,pars,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpRegModel() {
    uint8_t *acqBuf,*acqBufPtr;
    uint8_t confCode;
    uint16_t cmdParSize,acqParSize;
    if (debug)
        printMsg("------------------ fpRegModel: ------------------");
    fpGetParameterSize(FP_CMD_REG_MODEL,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_REG_MODEL,nullptr,acqBuf);
    if (fpCheckPacket(acqBuf) != FP_SUCCESS) {
        printMsg("Invalid acknowledge packet");
        delete [] acqBuf;
//        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        confCode=*acqBufPtr++;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation code: 0x" << setfill('0') << setw(2) << hex << static_cast<int>(confCode);
            printMsg(ss.str());
        }
        return confCode;
    }
}

int FPProtocol::fpSendImageData(uint16_t *dataPacketSize, uint8_t *image) {
    int i;
    uint8_t *imagePtr;

    int retCode;
    imagePtr = image;
    for (i=0;i<FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH/(*dataPacketSize);i++) {
        /* send the first 128 bytes */
        if ((retCode=fpSendData(FP_DATA_PACKET,dataPacketSize, imagePtr)) != FP_SUCCESS) {
            return(retCode);
        }
        if (debug) {
            stringstream ss;
            ss << "Sending 0x" << setfill('0') << setw(4) << hex << static_cast<int>(imagePtr-image);
            printMsg(ss.str());
        }

        imagePtr += *dataPacketSize;
        fflush(stdout);

        /* send the second 128 bytes */
        if ((retCode=fpSendData(FP_DATA_PACKET,dataPacketSize,imagePtr)) != FP_SUCCESS) {
            return(retCode);
        }

        if (debug) {
            stringstream ss;
            ss << "Sending 0x" << setfill('0') << setw(4) << hex << static_cast<int>(imagePtr-image);
            printMsg(ss.str());
        }
        imagePtr += *dataPacketSize;
        fflush(stdout);
    }

    /* send the first 128 bytes */
    if ((retCode=fpSendData(FP_DATA_PACKET,dataPacketSize,imagePtr)) != FP_SUCCESS) {
        return(retCode);
    }
    i++;
    imagePtr += FP_DEFAULT_DATA_PKT_LENGTH;
    if (debug) {
        stringstream ss;
        ss << "Sending 0x" << setfill('0') << setw(4) << hex << static_cast<int>(imagePtr-image) << '\r';
        printProgress(ss.str());
        ss.str("");
    }
    if (debug) {
        stringstream ss;
        ss << "Sending 0x" << setfill('0') << setw(4) << hex << static_cast<int>(imagePtr-image) << '\r';
        printMsg(ss.str());
        ss.str("");
    }

    /* end the end packet */
    if ((retCode=fpSendData(FP_END_PACKET,dataPacketSize,imagePtr)) != FP_SUCCESS) {
        return(retCode);
    }
    stringstream ss;
    ss << dec << i << "packages have been sent";
    printMsg(ss.str());
    return FP_SUCCESS;
}

int FPProtocol::fpDownImage(uint16_t *dataPacketSize,uint8_t *image) {

    uint8_t *acqBuf;
    uint16_t cmdParSize,acqParSize;
    int retCode;
    if (debug)
        printMsg("---------------- fpDownImage: --------------------");
    fpGetParameterSize(FP_CMD_DOWN_IMAGE,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    if ((retCode=fpSendCmd(FP_CMD_DOWN_IMAGE,nullptr,acqBuf)) != FP_SUCCESS) {
        delete [] acqBuf;
        return retCode;
    }
    if (debug)
        printMsg("Download command accepted, sending the data");

    if ((retCode=fpSendImageData(dataPacketSize,image)) != FP_SUCCESS) {
        delete [] acqBuf;
        return retCode;
    }
    //  fpSetDebug(false);
    return FP_SUCCESS;
}

int FPProtocol::fpEmpty() {
    uint8_t *acqBuf;
    uint16_t cmdParSize,acqParSize;
    int retCode;
    if (debug)
        printMsg("---------------- fpEmpty: --------------------");
    fpGetParameterSize(FP_CMD_EMPTY,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];

    if ((retCode=fpSendCmd(FP_CMD_EMPTY,nullptr,acqBuf)) != FP_SUCCESS) {
        printMsg("Could not clear the fingerprint library");
        delete [] acqBuf;

    }
    delete [] acqBuf;
    return retCode;
}

int FPProtocol::fpDeleteChar(uint16_t firstTemplate, uint16_t noOfTemplates) {
    uint8_t *acqBuf;
    uint8_t pars[4];
    uint16_t cmdParSize,acqParSize;
    int retCode;
    if (debug)
        printMsg("------------------ fpDeleteChar: ---------------------");
    pars[0]=firstTemplate>>8;
    pars[1]=firstTemplate&0xff;
    pars[2]=noOfTemplates>>8;
    pars[3]=noOfTemplates&0xff;

    fpGetParameterSize(FP_CMD_DEL_CHAR,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
    if ((retCode=fpSendCmd(FP_CMD_DEL_CHAR,pars,acqBuf)) != FP_SUCCESS) {
        printMsg("Could not delete templates");
        delete [] acqBuf;
    }
    return retCode;
}

int FPProtocol::fpCreateBitmap(uint8_t *pixels, const char *filename) {
    typedef struct __attribute__((packed)) {
        unsigned short type;                     /* Magic identifier            */
        unsigned int size;                       /* File size in bytes          */
        unsigned int reserved;
        unsigned int offset;                     /* Offset to image data, bytes */
    } HEADER;

    typedef struct __attribute__((packed)) {
        unsigned int size;               /* Header size in bytes      */
        int width,height;                /* Width and height of image */
        unsigned short planes;           /* Number of colour planes   */
        unsigned short bits;             /* Bits per pixel            */
        unsigned int compression;        /* Compression type          */
        unsigned int imagesize;          /* Image size in bytes       */
        int xresolution,yresolution;     /* Pixels per meter          */
        unsigned int ncolours;           /* Number of colours         */
        unsigned int importantcolours;   /* Important colours         */
        unsigned int dummy;
    } INFOHEADER;

    HEADER header;
    INFOHEADER infoheader;

//    FILE *fd;
    int i,j,k;
    uint8_t tmp,*pixelBufPtr, lineBuf[FP_IMAGE_WIDTH*3],*lineBufPtr;

    ofstream bitmapFileStream;
    bitmapFileStream.open(filename,ios::out | ios::binary);
    if (!bitmapFileStream.is_open()) {
        stringstream ss;
        ss << "Could not open file: " << filename << " for writing";
        printMsg(ss.str());
        return FP_BMP_OPEN_ERROR;
    }
/*
    if ((fd=fopen(filename,"w"))== nullptr) {
        stringstream ss;
        ss << "Could not open file: " << filename << " for writing";
        printMsg(ss.str());but when writing the value 27 to a random byte, there are formally no guarantees of what the result will be. And you might be writing to a padding byte.)
        return FP_BMP_OPEN_ERROR;
    }
    */
    header.type=0x4d42;                /* corresponds to "BM" */
    header.size=sizeof(HEADER)+sizeof(INFOHEADER)+FP_IMAGE_WIDTH*FP_IMAGE_HEIGHT*3;
    //  header.size=0x30876;
    header.reserved=0;
    header.offset=sizeof(HEADER) + sizeof(INFOHEADER);
    stringstream ss;
    ss << "pixel offset is: " << dec << header.offset;
    printMsg(ss.str());

    bitmapFileStream.write(reinterpret_cast<char *>(&header),sizeof(HEADER));
    if(bitmapFileStream.good()) {
        if (debug)
            printMsg("BMP header was successfull written");
    }
    else {
        printMsg("BMP header could not be written!");
        bitmapFileStream.close();
        return FP_WRITE_ERROR;
    }

    /*
    if (fwrite(&header,sizeof(HEADER),1,fd) != 1)
        printMsg("header was not written");
    else
        printMsg("BMP header was successfull written");
*/
    infoheader.size=40;
    infoheader.width=FP_IMAGE_WIDTH;
    infoheader.height=FP_IMAGE_HEIGHT;
    infoheader.planes=1;
    infoheader.bits=24;
    infoheader.compression=0;
    infoheader.imagesize=FP_IMAGE_WIDTH*FP_IMAGE_HEIGHT;
    infoheader.xresolution=2834;
    infoheader.yresolution=2834;
    infoheader.ncolours=0;
    infoheader.importantcolours=0;

    ss.str("");
    ss << "resolution: x: " << setfill('0') << setw(8) << hex << infoheader.xresolution << ", y:" <<infoheader.yresolution;
    printMsg(ss.str());

    bitmapFileStream.write(reinterpret_cast<char *>(&infoheader),sizeof(INFOHEADER));
    if(bitmapFileStream.good()) {
        if (debug)
            printMsg("BMP info header was successfull written");
    }
    else {
        printMsg("BMP into header could not be written!");
        bitmapFileStream.close();
        return FP_WRITE_ERROR;
    }
    /*
    if (fwrite(&infoheader,sizeof(INFOHEADER),1,fd) != 1) {
        printMsg("BMP infoheader was not written");
        return FP_BMP_WRITE_ERROR;
    }
    else
        printMsg("BMP infoheader was successfully written");
*/
    pixelBufPtr = pixels;

    if (debug) {
        printMsg("Incoming data:");
        stringstream ss;
        for (i=0;i<FP_IMAGE_WIDTH/2;i++) {
            ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*pixelBufPtr++) << " ";
            if (!((i+1)%16)) {
                printMsg(ss.str());
                ss.str("");
            }
        }
        printMsg(ss.str());
        pixelBufPtr = pixels;
    }

    lineBufPtr = lineBuf;
    for (j=0;j<FP_IMAGE_WIDTH/2;j++) {
        tmp = *pixelBufPtr++;
        /* for 24 bit real color we create 3 rgb colors of the same value */
        /* this corresponds ro a grey level                               */
        for (k=0;k<3;k++) {
            *lineBufPtr++ = tmp & 0xf0;
            *lineBufPtr++ = static_cast<uint8_t>((tmp & 0x0f) << 4);
        }
    }

    if (debug) {
        printMsg("Outgoing data:");
        lineBufPtr = lineBuf;
        stringstream ss;
        for (i=0;i<FP_IMAGE_WIDTH*3;i++) {
            if (!(i%16))
                ss << "0x" << setfill('0') << setw(4) << hex << i <<" ";
            ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(*lineBufPtr++) << " ";
            if (!((i+1)%16)) {
                printMsg(ss.str());
                ss.str("");
            }
        }
        printMsg(ss.str());
    }

    pixelBufPtr = pixels;
    /* fill the pixels in the bitmap file */
    for (i=0;i<FP_IMAGE_HEIGHT;i++) {
        /* fill one line of pixels */
        lineBufPtr = lineBuf;
        for (j=0;j<FP_IMAGE_WIDTH/2;j++) {
            tmp = *pixelBufPtr++;
            /* for 24 bit real color we create 3 rgb colors of the same value */
            /* this corresponds ro a grey level                               */
            for (k=0;k<3;k++) {
                *lineBufPtr++ = tmp & 0xf0;
                *lineBufPtr++ = static_cast<uint8_t>((tmp & 0x0f) << 4);
            }
        }
        bitmapFileStream.write(reinterpret_cast <char *>(lineBuf),FP_IMAGE_WIDTH*3);
        if (!bitmapFileStream.good()) {
             stringstream ss;
             ss << "Write error when trying to write " << filename;
             printMsg(ss.str());
             return FP_BMP_WRITE_ERROR;
        }
/*
        size_t noOfCharsWritten;
        if ((noOfCharsWritten = fwrite(lineBuf,FP_IMAGE_WIDTH*3,1,fd)) != 1) {
            stringstream ss;
            ss << "No of chars written: " << noOfCharsWritten;
            printMsg(ss.str());
            ss.str("");
            ss << "Write error when trying to write " << filename;
            printMsg(ss.str());
            return FP_BMP_WRITE_ERROR;
        }
*/
}

    if (debug)
        printMsg("fpCreateBitmap success");
    bitmapFileStream.close();
//    fclose(fd)
    return FP_SUCCESS;
}

int FPProtocol::fpDownChar(uint8_t charBufNo) {
    uint8_t chrBufNo;
    uint16_t cmdParSize,acqParSize;
    int retCode;
    uint8_t *acqBuf;

    if ((charBufNo != 1) && (charBufNo != 2)) {
        printMsg("Illegal character buffer number, must be 1 or 2");
        return FP_ILLEGAL_CHARBUF;
    }
    chrBufNo=charBufNo;
    fpGetParameterSize(FP_CMD_DOWN_CHAR,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf=static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+2));
    if ((retCode=fpSendCmd(FP_CMD_DOWN_CHAR,&chrBufNo,acqBuf)) != FP_SUCCESS) {
        stringstream ss;
        ss << "DownChar command failed with error " << dec <<retCode;
        printMsg(ss.str());
        fpPrintError(static_cast<char>(retCode));
        delete [] acqBuf;
//        free(acqBuf);
        return retCode;
    }
    delete [] acqBuf;
//    free(acqBuf);
    return FP_SUCCESS;
}

int FPProtocol::fpSendCharBuf(uint16_t *dataPacketSize,uint8_t *charBuf) {
    int i;
    int retCode;
    uint8_t *charBufPtr;

    charBufPtr = charBuf;
    for (i=0;i<FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS/(*dataPacketSize)-1;i++) {
        /* send the first blocks of data */
        if ((retCode=fpSendData(FP_DATA_PACKET,dataPacketSize,charBufPtr)) != FP_SUCCESS) {
            return(retCode);
        }
        if (debug) {
            stringstream ss;
            ss << "Block no: " << dec << i+1;
            printMsg(ss.str());
            ss.str("");
            ss << "Sending 0x" << setfill('0') << setw(4) << hex << static_cast<int>(charBufPtr-charBuf);
            printMsg(ss.str());
        }

        charBufPtr += *dataPacketSize;
        fflush(stdout);
    }

    if ((retCode=fpSendData(FP_END_PACKET,dataPacketSize,charBufPtr)) != FP_SUCCESS) {
        return(retCode);
    }
    return FP_SUCCESS;
}

int FPProtocol::fpWriteNotepad(uint8_t pageNo, uint8_t *page) {
    int retCode;
    uint8_t *acqBuf;
    uint8_t *pars;
    uint16_t cmdParSize,acqParSize;

    if (debug)
        printMsg("fpWriteNotepad:");
    if (pageNo > 15)
        return FP_ILLEGAL_PAGE_NO;

    fpGetParameterSize(FP_CMD_WRITE_NOTEPAD,&cmdParSize,&acqParSize);
    pars = new uint8_t [cmdParSize-1];
    pars[0] = pageNo;
    bcopy(page,&pars[1],FP_PAGE_SIZE);
    if (debug) {
        stringstream ss;
        ss << "Notepad page data for page no: " << dec << static_cast<int>(pars[0]);
        printMsg(ss.str());
        ss.str("");
        for (int i=0; i<2;i++) {
            for (int j=0;j<16;j++)
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(pars[2*i+j+1]) << " ";
            printMsg(ss.str());
            ss.str("");
        }

        ss.str("");
        for (int i=0; i<32;i++) {
            ss << pars[i+1];
        }
        printMsg(ss.str());
        ss.str("");
    }
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf=static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+32));
    if ((retCode=fpSendCmd(FP_CMD_WRITE_NOTEPAD,pars,acqBuf)) != FP_SUCCESS) {
        printMsg("DownChar command failed");
        delete [] pars;
        delete [] acqBuf;
//       free(acqBuf);
        return retCode;
    }
    delete [] pars;
    delete [] acqBuf;
//    free(acqBuf);
    return FP_SUCCESS;
}

int FPProtocol::fpReadNotepad(uint8_t pageNo, uint8_t *page) {
    int retCode;
    uint8_t *acqBuf;
    uint8_t pgNo;
    uint8_t *acqBufPtr;
    uint16_t cmdParSize,acqParSize;
    pgNo = pageNo;

    fpGetParameterSize(FP_CMD_READ_NOTEPAD,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf=static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+32));
    if ((retCode=fpSendCmd(FP_CMD_READ_NOTEPAD,&pgNo,acqBuf)) != FP_SUCCESS) {
        printMsg("DownChar command failed");
        delete[] acqBuf;
//        free(acqBuf);
    }
    acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS+1;
    bcopy(acqBufPtr,page,FP_PAGE_SIZE);
    delete [] acqBuf;
//    free(acqBuf);
    return FP_SUCCESS;
}

int FPProtocol::fpUpChar(uint8_t charBufNo) {
    uint8_t chrBufNo;
    uint16_t cmdParSize,acqParSize;
    int retCode;
    uint8_t *acqBuf;

    if ((charBufNo != 1) && (charBufNo != 2)) {
        printMsg("Illegal character buffer number, must be 1 or 2");
        return FP_ILLEGAL_CHARBUF;
    }
    chrBufNo=charBufNo;
    if (debug) {
        stringstream ss;
        ss << "Reading character buffer " << dec << chrBufNo;
        printMsg(ss.str());
//        printMsg("Reading character buffer %d\n",chrBufNo);
    }
    fpGetParameterSize(FP_CMD_UP_CHAR,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf=static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+2));
    if ((retCode=fpSendCmd(FP_CMD_UP_CHAR,&chrBufNo,acqBuf)) != FP_SUCCESS) {
        printMsg("DownUp command failed");
        delete [] acqBuf;
        free(acqBuf);
        return retCode;
    }
    delete [] acqBuf;
//    free(acqBuf);
    return FP_SUCCESS;
}

int FPProtocol::fpMatch(uint16_t *score) {
    uint8_t *acqBuf,*acqBufPtr;
    uint16_t scr;
    uint16_t cmdParSize,acqParSize;
    int retCode;

    fpGetParameterSize(FP_CMD_MATCH,&cmdParSize,&acqParSize);
    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+1));
    fpSendCmd(FP_CMD_MATCH,nullptr,acqBuf);
    if ((retCode=fpCheckPacket(acqBuf)) != FP_SUCCESS) {
        fpPrintError(static_cast<char>(retCode));
        delete [] acqBuf;
        free(acqBuf);
        return FP_INVALID_PACKET;
    }
    else
    {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS;
        retCode = *acqBufPtr;
        acqBufPtr++;
        scr=static_cast<uint16_t>(*acqBufPtr++ << 8);
        scr |=*acqBufPtr;
        *score=scr;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation Code: " << dec << retCode;
            printMsg(ss.str());
        }

        return retCode;
    }
}
int FPProtocol::fpSearch(uint8_t charBufId, uint16_t startPage, uint16_t noOfPages,
                         uint16_t *pageId, uint16_t *score) {
    uint8_t *acqBuf,*acqBufPtr;
    uint16_t cmdParSize,acqParSize;
    uint8_t *pars;
    uint16_t pgId,scr;
    int retCode;

    fpGetParameterSize(FP_CMD_SEARCH,&cmdParSize,&acqParSize);
    pars = new uint8_t[cmdParSize-1];
    pars[0] = charBufId;
    pars[1] = static_cast<uint8_t>(startPage << 8);
    pars[2] = startPage & 0xff;
    pars[3] = static_cast<uint8_t>(noOfPages << 8);
    pars[4] = noOfPages &0xff;

    acqBuf = new uint8_t[FP_BASE_PKT_SIZE+acqParSize];
//    acqBuf = static_cast<uint8_t *>(malloc(FP_BASE_PKT_SIZE+5));
    if ((retCode=fpSendCmd(FP_CMD_SEARCH,pars,acqBuf)) != FP_SUCCESS) {
        delete [] pars;
        delete [] acqBuf;
//        free(acqBuf);
        return retCode;
    }
    else {
        acqBufPtr=acqBuf+FP_CONFIRMATION_CODE_POS+1;
        pgId=static_cast<u_int16_t>(*acqBufPtr++ << 8);
        pgId |=*acqBufPtr++;
        scr=static_cast<u_int16_t>(*acqBufPtr++ << 8);
        scr |=*acqBufPtr++;
        *pageId=pgId;
        *score=scr;
        delete [] pars;
        delete [] acqBuf;
//        free(acqBuf);
        if (debug) {
            stringstream ss;
            ss << "Confirmation Code: " << dec << retCode;
            printMsg(ss.str());
        }
        return retCode;
    }
}

string FPProtocol::fpGetErrorString(char errCode) {
    string msg;
    switch (errCode) {
    case 0x00:
        msg= "Success, Command completed\n";
        break;
    case 0x01:
        msg="error when receiving the data package\n";
        break;
    case 0x02:
        msg="No finger in the sensor\n";
        break;
    case 0x03:
        msg="Fail to enroll the finger\n";
        break;
    case 0x06:
        msg="Fail to generate character file due to over-disorderly fingerprint image\n";
        break;
    case 0x07:
        msg="Fail to generate character file due to lack of character point\nor too small fingerprint\n";
        break;
    case 0x08:
        msg="Fingerprints do not match\n";
        break;
    case 0x09:
        msg="Fail to find matching fingerprint\n";
        break;
    case 0x0a:
        msg="Fail to combine character files\n";
        break;
    case 0x0b:
        msg="Addressing page id beyond the fingerprint library\n";
        break;
    case 0x0c:
        msg="Error when reading template from the library or template is invalid\n";
        break;
    case 0x0d:
        msg="Error when uploading template\n";
        break;
    case 0x0e:
        msg="Module cannot receive the following data packages\n";
        break;
    case 0x0f:
        msg="Error when uploading image\n";
        break;
    case 0x10:
        msg="Fail to delete template\n";
        break;
    case 0x11:
        msg="Fail to clear finger library\n";
        break;
    case 0x15:
        msg="Fail to delete the template\n";
        break;
    case 0x18:
        msg="Error when writing flash\n";
        break;
    case 0x19:
        msg="No definition error\n";
        break;
    case 0x1a:
        msg="Invalid register number\n";
        break;
    case 0x1b:
        msg="Incorrect configuration register\n";
        break;
    case 0x1c:
        msg="Wrong notepad page\n";
        break;
    case 0x1d:
        msg="Fail to operate communication port\n";
        break;
    case FP_ILLEGAL_INSTRUCTION:
        msg="Illegal instruction code\n";
        break;
    case FP_INVALID_PACKET:
        msg="Invalid packet\n";
        break;
    case FP_BAD_CHECKSUM:
        msg="Wrong checksum\n";
        break;
    case FP_DANGEROUS:
        msg="Changing this setting may result in difficulty to commnunicate with the fingerprint module\n";
        break;
    case FP_ILLEGAL_CHARBUF:
        msg="Bad character buffer\n";
        break;
    case FP_ILLEGAL_PAGE_NO:
        msg="Illegal page no for notepad\n";
        break;
    default:
        stringstream ss;
        ss << "Unknown error code: " << dec << static_cast<int>(errCode);
        msg=ss.str();
        break;
    }
    return msg;
}

void FPProtocol::fpPrintError(char errCode) {
    switch (errCode) {
    case 0x00:
        printMsg("Success, Command completed");
        break;
    case 0x01:
        printMsg("error when receiving the data package");
        break;
    case 0x02:
        printMsg("No finger in the sensor");
        break;
    case 0x03:
        printMsg("Fail to enroll the finger");
        break;
    case 0x06:
        printMsg("Fail to generate character file due to over-disorderly fingerprint image");
        break;
    case 0x07:
        printMsg("Fail to generate character file due to lack of character point");
        printMsg("or too small fingerprint");
        break;
    case 0x08:
        printMsg("Fingerprints do not match");
        break;
    case 0x09:
        printMsg("Fail to find matching fingerprint");
        break;
    case 0x0a:
        printMsg("Fail to combine character files");
        break;
    case 0x0b:
        printMsg("Addressing page id beyond the fingerprint library");
        break;
    case 0x0c:
        printMsg("Error when reading template from the library or template is invalid");
        break;
    case 0x0d:
        printMsg("Error when uploading template");
        break;
    case 0x0e:
        printMsg("Module cannot receive the following data packages");
        break;
    case 0x0f:
        printMsg("Error when uploading image");
        break;
    case 0x10:
        printMsg("Fail to delete template");
        break;
    case 0x11:
        printMsg("Fail to clear finger library");
        break;
    case 0x15:
        printMsg("Fail to delete the template");
        break;
    case 0x18:
        printMsg("Error when writing flash");
        break;
    case 0x19:
        printMsg("No definition error");
        break;
    case 0x1a:
        printMsg("Invalid register number");
        break;
    case 0x1b:
        printMsg("Incorrect configuration register");
        break;
    case 0x1c:
        printMsg("Wrong notepad page");
        break;
    case 0x1d:
        printMsg("Fail to operate communication port");
        break;
    case FP_ILLEGAL_INSTRUCTION:
        printMsg("Illegal insgtruction code");
        break;
    case FP_INVALID_PACKET:
        printMsg("Invalid packet");
        break;
    case FP_BAD_CHECKSUM:
        printMsg("Wrong checksum");
        break;
    case FP_DANGEROUS:
        printMsg("Changing this setting may result in difficulty to commnunicate with the fingerprint module");
        break;
    case FP_ILLEGAL_CHARBUF:
        printMsg("Bad character buffer");
        break;
    case FP_ILLEGAL_PAGE_NO:
        printMsg("Illegal page no for notepad");
        break;

    default:
        stringstream ss;
        ss << "Unknown error code: " << dec << static_cast<int>(errCode);
        printMsg(ss.str());
        break;
    }
}
