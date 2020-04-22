#ifndef SFGQT_H
#define SFGQT_H

#include <QMainWindow>
#include <QFileDialog>
#include <QImage>
#include <QByteArray>
#include <QThread>
#include <QSettings>
#include <QVBoxLayout>
#include "enrolldialog.h"
#include "verifyTemplateDialog.h"
#include "writenotepaddialog.h"
#include "fpProtocol.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdint.h>

#include <string.h>

#define BHEX( x ) setw(2) << setfill('0') << hex << int( x )
#define SHEX( x ) setw(4) << setfill('0') << hex << int( x )

#define FILENAME "fingerprint.bmp"

using namespace std;

namespace Ui {
class SfgQt;
}

struct fpListEntry{
  int index;
  uint32_t pad;     // needed for alignment
  QString itemText;
};

class Assistant;

class SfgQt : public QMainWindow
{
    Q_OBJECT

public:
    SfgQt(QWidget *parent = nullptr);
    ~SfgQt();
  
    FPProtocol *protocol;
    uint16_t statusReg;
    uint16_t systemIdentifier;
    uint16_t fingerLibSize;
    uint16_t securityLevel;
    uint32_t deviceAddress;
    uint16_t dataPacketSize;
    string   port = "/dev/ttyUSB0";
    uint16_t baudrate;
    QString  templateDir=QString("");
    QString  imageDir=QString("");

private:
    EnrollDialog         *enrollDialog;
    VerifyTemplateDialog *verifyTemplateDialog;
    WriteNotepadDialog   *writeNotepadDialog;
    QFileDialog          *uploadImageDialog;
    QFileDialog          *uploadTemplateDialog;
    QSpinBox             *templateSelector;
    QSpinBox             *charFileSelector;
    uint8_t fingerPrintCompressedImage[FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH/2],*fingerPrintCompressedImagePtr;

    QString   fingerName;
    bool      serialIsOpen = false;
    bool      cancel=false;
    int       settingsSize;
    int       fpMaxTemplates=0;
    Ui::SfgQt *ui_sfgQt;


    void printSystemInfo();
    int  takeFingerprint();
    void saveSettings();
    void readSettings();
    int  writeCharFile(ostream&,int);
    int  readAndDownloadCharfile(istream&,uint16_t,int);
    void parse(string, uint16_t *, uint8_t *);
    int  isValidTemplate(int,bool *);
    int  getFpMaxTemplates();
    void printData(uint8_t *data, uint16_t size);
    int  waitForFingerprint();
    QList<int> getTemplateTable();

private slots:
    int  slotOpenSerial();
    int  createEnrollDialog();
    int  doEnroll();
    int  continuousEnroll();
    int  doWriteNotepad();
    int  createNotePadDialog();
    int  createWriteNotePadDialog();
    int  createUploadTemplateDialog();
    int  createUploadCharFileDialog();
    int  clearNotepad();
    int  captureFPImage();
    int  continuousCaptureFPImage();
    int  uploadFPImage();
    int  saveFPImage(QString);
    void cancelOperation();
    int  emptyDB();
    int  deleteDB_Entry();
    void close();
    bool checkConsistency();
    int  match();
    int  getRandomNumber();
    void protocolError(int,QString);
    int  uploadTemplateDB(QString);
    int  downloadTemplateDB(QString);
    int  search();
    int  continuousSearch();
    int  quickSearch();
    void getDirectoryPath();
    void createSaveImageFileDialog();
    void createVerifyTemplateDialog();
    void saveSourceImage(QString);
    void saveThinImage(QString);
    void saveBinImage(QString);
    int  verifyTemplate();
    int  uploadTemplate(QString);
    int  downloadSingleTemplate(QString);
    int  downloadTemplate(ifstream&,int);
    int  uploadCharFile(QString);
    int  downloadCharFile(QString);
    int  changeSecurityLevel(int);
    int  changeBaudrate(int);
    int  changePacketSize(int);
    int  findBaudrate();
    void newSerialDevice();
    void test();
};

#endif // SFGQT_H
