#include "sfgQt.h"
#include "ui_sfgQt.h"

#include <QDebug>
#include <string.h>
#include "enrolldialog.h"
#include "notepaddialog.h"
#include <QMessageBox>
#include <QString>
#include <iostream>
#include <fstream>

SfgQt::SfgQt(QWidget *parent) :
    QMainWindow(parent), ui_sfgQt(new Ui::SfgQt)
{
    //    int br;
    //    char baudrateText[10];
    QSettings settings("UCC","sfgQt");
    ui_sfgQt->setupUi(this);

    protocol = new FPProtocol();
    protocol->fpSetDebug(true);

    QString defaultPort = settings.value("port","/dev/ttyUSB0").toString();
    port = defaultPort.toStdString();
    imageDir = settings.value("imageDir","/opt/ucc/projects/fingerprint/Qt/fingerprintImages").toString();
    templateDir = settings.value("templateDir","/opt/ucc/projects/fingerprint/Qt/fingerprintFiles").toString();
    qDebug() << "port from settings file: " << defaultPort;
    ui_sfgQt->deviceNameLineEdit->setText(defaultPort);
    ui_sfgQt->imageFileDirectoryLineEdit->setText(imageDir);
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);
    ui_sfgQt->testButton->hide();

    /* connect the signals to the corresponding slots */
    connect(ui_sfgQt->OpenDeviceButton,SIGNAL(clicked()),this, SLOT(slotOpenSerial()));
    connect(ui_sfgQt->enrollButton,SIGNAL(clicked()),this,SLOT(createEnrollDialog()));
    connect(ui_sfgQt->readNotepadButton,SIGNAL(clicked()),this,SLOT(createNotePadDialog()));
    connect(ui_sfgQt->writeNotepadButton,SIGNAL(clicked()),this,SLOT(createWriteNotePadDialog()));
    connect(ui_sfgQt->clearNotepadButton,SIGNAL(clicked()),this,SLOT(clearNotepad()));
    connect(ui_sfgQt->captureButton,SIGNAL(clicked()),this,SLOT(captureFPImage()));
    connect(ui_sfgQt->uploadImageButton,SIGNAL(clicked()),this,SLOT(uploadFPImage()));
    connect(ui_sfgQt->saveImageButton,SIGNAL(clicked()),this,SLOT(createSaveImageFileDialog()));
    connect(ui_sfgQt->cancelOpButton,SIGNAL(clicked()),this,SLOT(cancelOperation()));
    connect(ui_sfgQt->DB_EmptyButtton,SIGNAL(clicked()),this,SLOT(emptyDB()));
    connect(ui_sfgQt->DB_DeleteButton,SIGNAL(clicked()),this,SLOT(deleteDB_Entry()));
    connect(ui_sfgQt->randomButton,SIGNAL(clicked()),this,SLOT(getRandomNumber()));
    connect(ui_sfgQt->matchButton,SIGNAL(clicked()),this,SLOT(match()));
    connect(ui_sfgQt->quickSearchButton,SIGNAL(clicked()),this,SLOT(quickSearch()));
    connect(ui_sfgQt->searchButton,SIGNAL(clicked()),this,SLOT(search()));
    connect(ui_sfgQt->validTemplateButton,SIGNAL(clicked()),this,SLOT(createVerifyTemplateDialog()));

    connect(ui_sfgQt->sourceButton,SIGNAL(clicked()),this,SLOT(createSaveImageFileDialog()));
    connect(ui_sfgQt->binImageButton,SIGNAL(clicked()),this,SLOT(createSaveImageFileDialog()));
    connect(ui_sfgQt->thinImageButton,SIGNAL(clicked()),this,SLOT(createSaveImageFileDialog()));

    connect(ui_sfgQt->uploadTemplateButton,SIGNAL(clicked()),this,SLOT(createUploadTemplateDialog()));
    connect(ui_sfgQt->downloadTemplateButton ,SIGNAL(clicked()),this,SLOT(createUploadTemplateDialog()));
    connect(ui_sfgQt->uploadCharfileButton,SIGNAL(clicked()),this,SLOT(createUploadCharFileDialog()));
    connect(ui_sfgQt->downloadCharfileButton,SIGNAL(clicked()),this,SLOT(createUploadCharFileDialog()));
    connect(ui_sfgQt->uploadDBButton,SIGNAL(clicked()),this,SLOT(createUploadTemplateDialog()));
    connect(ui_sfgQt->downloadDBButton,SIGNAL(clicked()),this,SLOT(createUploadTemplateDialog()));
    connect(ui_sfgQt->findBaudrateButton,SIGNAL(clicked()),this,SLOT(findBaudrate()));
    connect(ui_sfgQt->conCaptureButton,SIGNAL(clicked()),this,SLOT(continuousCaptureFPImage()));
    connect(ui_sfgQt->ConSearchButton,SIGNAL(clicked()),this,SLOT(continuousSearch()));
    connect(ui_sfgQt->conEnrollButton,SIGNAL(clicked()),this,SLOT(createEnrollDialog()));
    connect(ui_sfgQt->deviceNameLineEdit,SIGNAL(editingFinished()),SLOT(newSerialDevice()));

    connect(ui_sfgQt->testButton,SIGNAL(clicked()),this,SLOT(test()));

    /* make sure the settings are saved even when closing with the red cross */
    connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(close()));
    /* change the font size of the status QTextEdit widget */
    QFont font = QFont();
    font.setPointSize(16);
    ui_sfgQt->statusTextEdit->setFont(font);

    ui_sfgQt->hardwareInfoText->setPlainText("Welcome to the SFG fingerprint module\n");
    ui_sfgQt->hardwareInfoText->appendPlainText("Please connect the module");
    ui_sfgQt->hardwareInfoText->appendPlainText("and open the serial line first!");

    serialIsOpen = false;
}

SfgQt::~SfgQt()
{
    delete ui_sfgQt;
}

void SfgQt::printSystemInfo() {
    QString infoText;
    infoText = QString::asprintf("Finger Database:             %d",fingerLibSize);

    ui_sfgQt->hardwareInfoText->setPlainText(infoText);
    infoText =QString::asprintf("Security Level:                %d",securityLevel);
    ui_sfgQt->hardwareInfoText->appendPlainText(infoText);
    infoText=QString::asprintf("Address:              0x%x",deviceAddress);
    ui_sfgQt->hardwareInfoText->appendPlainText(infoText);
    infoText=QString::asprintf("Package Size:                %d",dataPacketSize);
    //    sprintf(text,"Package Size: %d",dataPacketSize);
    ui_sfgQt->hardwareInfoText->appendPlainText(infoText);
    infoText=QString::asprintf("Baudrate:                   %d bps",baudrate*9600);
    ui_sfgQt->hardwareInfoText->appendPlainText(infoText);
    ui_sfgQt->hardwareInfoText->appendPlainText("Product Type:           ZFM30411");
    ui_sfgQt->hardwareInfoText->appendPlainText("Version:                20090508");
    ui_sfgQt->hardwareInfoText->appendPlainText("Manufacturer:           ZhianTec");
    ui_sfgQt->hardwareInfoText->appendPlainText("Sensor:                   ZFM-20");
}

int SfgQt::slotOpenSerial()
{
    int retCode;
    QString packetSizeText;
    QString securityLevelText;
    QSettings settings("UCC","sfgQt");
    uint16_t dataPacketSizeCode;

    if (protocol->handle >0) {
        ui_sfgQt->statusBar->showMessage("Serial line is already open. Do not re-open");
    } else {
        baudrate = static_cast<uint16_t>(settings.value("baudrate",FP_DEFAULT_BAUDRATE).toUInt());
        int br = baudrate;

        qDebug() << "Opening serial port " << QString::fromStdString(port) << " on baudrate" << baudrate*9600;
        if ((retCode = protocol->fpInit(port,static_cast<uint8_t>(baudrate))) < 0) {
            br = FP_DEFAULT_BAUDRATE;
            /* settings file may be corrupted, try with default baudrate */
            if ((retCode = protocol->fpInit(port,static_cast<uint8_t>(FP_DEFAULT_BAUDRATE))) < 0) {
                ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not open serial serial. Is the fingerprint module connected?</font>");
                return ERR_SERIAL_PORT;
            }
        }
        QString msg="<font color=\"blue\">Trying to connect at baudrate ";
        msg.append(QString::number(br*9600));
        msg.append(" </font>");
        ui_sfgQt->statusTextEdit->setHtml(msg);
        qApp->processEvents();
        /* check if the fingerprint module responds as expected */
        qDebug() << "Trying fpHandshake";
        if ((retCode = protocol->fpHandshake()) != FP_SUCCESS) {
            /* maybe bad baudrate, try on default baudrate */
            qDebug() << "Fingerprint module does not respond on baudrate " << baudrate;
            qDebug() << "Trying on default baudrate: " << FP_DEFAULT_BAUDRATE*9600;
            protocol->fpClose();
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not establish serial connection.<br>Trying the default baud rate</font>");
            qApp->processEvents();
            if ((retCode = protocol->fpInit(port, static_cast<uint8_t>(FP_DEFAULT_BAUDRATE))) < 0) {
                ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not open serial serial. Is the fingerprint module connected?</font>");
                return ERR_SERIAL_PORT;
            }
            /* if it still does not work, return error */
            if ((retCode = protocol->fpHandshake()) != FP_SUCCESS) {
                ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not establish serial connection.<br>Please try the <i>Find Baudrate</i> button </font>");
                protocol->fpClose();
                return ERR_SERIAL_PORT;
            }
            else{
                baudrate = FP_DEFAULT_BAUDRATE;
                qDebug() << "baudrate set to " << baudrate;
                ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Successfully opened the serial connection<br>on default baud rate: 57600 baud</font>");
            }

        }
        ui_sfgQt->deviceNameLineEdit->setEnabled(false);
        ui_sfgQt->statusBar->showMessage("Serial connection to fingerpint module successfull opened");
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Open device Success!</font>");
    }


    /*
     * read system information
     */
    if ((retCode = protocol->fpReadSysPara(
             &statusReg,
             &systemIdentifier,
             &fingerLibSize,
             &securityLevel,
             &deviceAddress,
             &dataPacketSizeCode,
             &baudrate)) == FP_SUCCESS) {
        qDebug() << "sys info successfully read";
        dataPacketSize = static_cast<uint16_t>(32<<dataPacketSizeCode);
        printSystemInfo();
        serialIsOpen = true;
    }
    else {
        qDebug() << "failed to read sys info";
        ui_sfgQt->statusBar->showMessage("Error when getting system parameters with fpReadSysPara()");
        protocolError(retCode,"fpReadSysPara");
        return retCode;
    }

    qDebug() << "baudrate after reading settings" << baudrate;

    /* these are the standard Unix baud rates */
    /* we may try non-standard baud rates later */
    ui_sfgQt->baudrateComboBox->addItem("9600");
    ui_sfgQt->baudrateComboBox->addItem("19200");
    ui_sfgQt->baudrateComboBox->addItem("38400");
    ui_sfgQt->baudrateComboBox->addItem("57600");
    ui_sfgQt->baudrateComboBox->addItem("115200");

    for (int i=0;i<4;i++) {
        packetSizeText = QString::number(32<<i);
        //        sprintf(packetSizeText,"%d",32<<i);
        ui_sfgQt->packetSizeComboBox->addItem(packetSizeText);
    }

    for (int i=0;i<5;i++) {
        securityLevelText = QString::number(i+1);
        //        sprintf(securityLevelText,"%d",i+1);
        ui_sfgQt->securityLevelComboBox->addItem(securityLevelText);
    }

    fpMaxTemplates = fingerLibSize;
    qDebug() << "Setting current baudrate combobox text to " << baudrate*9600;
    ui_sfgQt->baudrateComboBox->setCurrentText(QString::number(baudrate*9600));
    for (int i=0;i<4;i++)
        if ((32 << i) == dataPacketSize) {
            ui_sfgQt->packetSizeComboBox->setCurrentIndex(i);
            break;
        }
    ui_sfgQt->securityLevelComboBox->setCurrentText(QString::number(securityLevel));

    connect(ui_sfgQt->securityLevelComboBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&SfgQt::changeSecurityLevel);
    connect(ui_sfgQt->baudrateComboBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&SfgQt::changeBaudrate);
    connect(ui_sfgQt->packetSizeComboBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&SfgQt::changePacketSize);
    readSettings();
    return retCode;
}

int SfgQt::getFpMaxTemplates() {
    return this->fpMaxTemplates;
}

int SfgQt::createEnrollDialog() {
    int nextFreeTemplate=0,j;
    qDebug("Enroll Dialog");
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    QList<int> fpTemplateList;
    fpTemplateList = getTemplateTable();

    if (fpTemplateList.length() < getFpMaxTemplates()) {
        for (nextFreeTemplate=0;nextFreeTemplate<getFpMaxTemplates();nextFreeTemplate++) {
            for (j=0; j<fpTemplateList.length();j++)
                if (nextFreeTemplate == fpTemplateList[j])
                    /* this pos is occupied */
                    break;
            if (j == fpTemplateList.length())
                break;
        }

    }
    qDebug() << "next free template: " << nextFreeTemplate+1;

    EnrollDialog *enrollDialog = new EnrollDialog(this);
    this->enrollDialog = enrollDialog;
    this->enrollDialog->setWindowTitle("Enter name and template number");
    this->enrollDialog->setTemplateNo(nextFreeTemplate+1);
    this->enrollDialog->show();
    if (QObject::sender() == ui_sfgQt->enrollButton) {
       connect(this->enrollDialog,SIGNAL(accepted()),this,SLOT(doEnroll()));
    }
    else  {
       connect(this->enrollDialog,SIGNAL(accepted()),this,SLOT(continuousEnroll()));
    }
    return 1;
}

int SfgQt::doEnroll(){

    uint16_t score;
    int retCode;
    bool isValid;
    qDebug("enrolling");

    QString name = this->enrollDialog->getName();
    name = name.left(32);                // limit to 32 chars

    if (name == QString("")) {
        qDebug("No name given");
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Name of person and finger is needed</font>");
        return -1;
    }
    else {
        qDebug() << "Name : " << name;
    }
    int templateNo = this->enrollDialog->getTemplateNo();
    qDebug() << "Template number: " <<  templateNo;

    if ((retCode = isValidTemplate(templateNo-1,&isValid)) != FP_SUCCESS)
        return retCode;
    if (isValid) {
        qDebug() << "template "<<templateNo<<"is already occupied";
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Template position occupied", "Overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            qDebug() << "No was clicked";
            return retCode;
        }
    }
    cancel = false;
    qDebug() << "Start enrolling";
    /* first fingerprint, two are needed to enroll */
    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Taking first fingerprint<br>Please put your finger onto the reader</font>");
    qApp->processEvents();

    qDebug() << "take first finger print";
    if ((retCode = takeFingerprint()) != FP_SUCCESS)
        return retCode;

    if (ui_sfgQt->previewCheckBox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;

    qDebug() << "generate character file 1";
    if ((retCode = protocol->fpImg2Tz(1)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(1)");
        return retCode;
    }

    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Taking second fingerprint<br>Please first remove your finger from the reader<br>then put it back on</font>");
    qApp->processEvents();

    qDebug() << "take second finger print";
    /* second fingerprint */
    if ((retCode = takeFingerprint()) != FP_SUCCESS)
        return retCode;

    if (ui_sfgQt->previewCheckBox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;
    ui_sfgQt->progressBar->setValue(0);
    qApp->processEvents();

    qDebug() << "generate character file 2";
    if ((retCode = protocol->fpImg2Tz(2)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(2)");
        return retCode;
    }
    qDebug() << "match the character files";

    if ((retCode=protocol->fpMatch(&score)) != FP_SUCCESS) {
        if (retCode == ERR_PRINTS_DONT_MATCH) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Fingers don't match</font>");
            return retCode;
        }
        else {
            protocolError(retCode,"fpMatch");
        }
        return retCode;
    }

    /* combine the two character files */
    if ((retCode = protocol->fpRegModel()) != FP_SUCCESS) {
        protocolError(retCode,"fpRegModel");
        qDebug() << "Could combine both finger prints, error code was: " << retCode;
    }
    qDebug() << "Character files were successfully combined";
    qDebug() << "Storing to position " << templateNo << "in flash";
    if ((retCode = protocol->fpStore(1,static_cast<uint16_t>(templateNo-1))) == FP_SUCCESS) {
        /* enter the fingerprint into the table */
        QTableWidgetItem *tableItem = new QTableWidgetItem(QString(name));
        ui_sfgQt->fpTable->setItem(templateNo-2,1,tableItem);
    }
    else {
        protocolError(retCode,"fpStore");
        qDebug() << "Error entering the fingerprint into the database. Error was :" << retCode;
        return retCode;
    }

    if (score < 100) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Confidence level is low</font>");
        ui_sfgQt->confidenceLineEdit->setText(QString::number(score));
        qDebug() << "Character files match with confidence level " << score;
    }
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Enrolling was successful!</font>");
    }

    /* insert into the template database */

    return retCode;
}

int SfgQt::continuousEnroll() {
    int retCode=FP_SUCCESS;
    int nextFreeTemplate,j;
    qDebug() << "Continuous enroll";
    uint16_t score;
    bool isValid;
    cancel = false;
    qDebug("continuously enrolling");

    QString name = this->enrollDialog->getName();
    name = name.left(32);                // limit to 32 chars

    if (name == QString("")) {
        qDebug("No name given");
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Name of person and finger is needed</font>");
        return -1;
    }
    else {
        qDebug() << "Name : " << name;
    }
    int templateNo = this->enrollDialog->getTemplateNo();
    qDebug() << "Template number: " <<  templateNo;

    if ((retCode = isValidTemplate(templateNo-1,&isValid)) != FP_SUCCESS)
        return retCode;
    if (isValid) {
        qDebug() << "template "<<templateNo<<"is already occupied";
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Template position occupied", "Overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            qDebug() << "No was clicked";
            return retCode;
        }
    }

    qDebug() << "Start enrolling";
    /* first fingerprint, two are needed to enroll */
    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Taking first fingerprint<br>Please put your finger onto the reader</font>");
    qApp->processEvents();

    if (cancel) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Operation was cancelled</font>");
        return retCode;
    }
    else
        while (!cancel) {
            retCode = waitForFingerprint();
        if ((retCode != FP_SUCCESS) && (retCode != ERR_NO_FINGER))
            return retCode;
        if (retCode == FP_SUCCESS)
            break;
        }
    if (ui_sfgQt->previewCheckBox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;

    qDebug() << "generate character file 1";
    if ((retCode = protocol->fpImg2Tz(1)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(1)");
        return retCode;
    }

    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Taking second fingerprint<br>Please first remove your finger from the reader<br>then put it back on</font>");
    qApp->processEvents();

    qDebug() << "take second finger print";
    /* second fingerprint */
    if (cancel) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Operation was cancelled</font>");
        return retCode;
    }
    else
        while (!cancel) {
            retCode = waitForFingerprint();
        if ((retCode != FP_SUCCESS) && (retCode != ERR_NO_FINGER))
            return retCode;
        if (retCode == FP_SUCCESS)
            break;
        }

    if (ui_sfgQt->previewCheckBox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;
    ui_sfgQt->progressBar->setValue(0);
    qApp->processEvents();

    qDebug() << "generate character file 2";
    if ((retCode = protocol->fpImg2Tz(2)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(2)");
        return retCode;
    }
    qDebug() << "match the character files";

    if ((retCode=protocol->fpMatch(&score)) != FP_SUCCESS) {
        if (retCode == ERR_PRINTS_DONT_MATCH) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Fingers don't match</font>");
            return retCode;
        }
        else {
            protocolError(retCode,"fpMatch");
        }
        return retCode;
    }

    /* combine the two character files */
    if ((retCode = protocol->fpRegModel()) != FP_SUCCESS) {
        protocolError(retCode,"fpRegModel");
        qDebug() << "Could combine both finger prints, error code was: " << retCode;
    }
    qDebug() << "Character files were successfully combined";
    qDebug() << "Storing to position " << templateNo << "in flash";
    if ((retCode = protocol->fpStore(1,static_cast<uint16_t>(templateNo-1))) == FP_SUCCESS) {
        /* enter the fingerprint into the table */
        QTableWidgetItem *tableItem = new QTableWidgetItem(QString(name));
        ui_sfgQt->fpTable->setItem(templateNo-2,1,tableItem);
    }
    else {
        protocolError(retCode,"fpStore");
        qDebug() << "Error entering the fingerprint into the database. Error was :" << retCode;
        return retCode;
    }

    if (score < 100) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Confidence level is low</font>");
        ui_sfgQt->confidenceLineEdit->setText(QString::number(score));
        qDebug() << "Character files match with confidence level " << score;
    }
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Enrolling was successful!</font>");
    }

    if (cancel)
        return retCode;

    createEnrollDialog();
    QList<int> fpTemplateList;
    fpTemplateList = getTemplateTable();
    nextFreeTemplate = 0;
    qDebug() << "current template No: " << templateNo;
    /* First check if the template database is full */
    /* in this case propose the very first template to be overwritten */
    if (fpTemplateList.length() < getFpMaxTemplates()) {
        /* check if there is a slot higher than the current template */
        for (nextFreeTemplate=templateNo;nextFreeTemplate < getFpMaxTemplates();nextFreeTemplate++) {
            for (j=0; j<fpTemplateList.length();j++)
                if (nextFreeTemplate == fpTemplateList[j])
                    /* this pos is occupied */
                    break;
            if (j == fpTemplateList.length())
                break;
        }
        qDebug() << "next free template: " << nextFreeTemplate+1;
        if (nextFreeTemplate == getFpMaxTemplates()) {
            /* no slot free after the current template no but there must be a free one before */
            for (nextFreeTemplate=0;nextFreeTemplate < getFpMaxTemplates();nextFreeTemplate++) {
                for (j=0; j<fpTemplateList.length();j++)
                    if (nextFreeTemplate == fpTemplateList[j])
                        /* this pos is occupied */
                        break;
                if (j == fpTemplateList.length())
                    break;
            }
        }
    }
    this->enrollDialog->setTemplateNo(nextFreeTemplate+1);
    return retCode;
}

int SfgQt::createNotePadDialog() {
    int retCode = FP_SUCCESS;
    qDebug("NotePad Dialog");
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    stringstream ss;
    QString notePadAscii;
    uint8_t page[FP_PAGE_SIZE];
    for (uint8_t i=0;i<FP_NO_OF_PAGES;i++) {
        if ((retCode = protocol->fpReadNotepad(i,page)) != FP_SUCCESS) {
            protocolError(retCode,"fpReadNotepad");
            qDebug() << "Error when reading notepad";
            return retCode;
        }
        if (protocol->fpDebugIsOn()) {
            stringstream ss;
            ss << "page " << dec <<int(i);
            printMsg(ss.str());
            ss.str("");
            for (int j=0;j<FP_PAGE_SIZE/2;j++)
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(page[j]) << " ";
            printMsg(ss.str());
            ss.str("");

            for (int j=0;j<FP_PAGE_SIZE/2;j++)
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(page[j]) << " ";
            printMsg(ss.str());
        }
        uint8_t cleanedPage[FP_PAGE_SIZE+1];
        cleanedPage[FP_PAGE_SIZE]='\0';
        for (int j=0;j<FP_PAGE_SIZE;j++)
            if ((page[j] <' ') || (page[j] > '~'))
                cleanedPage[j] = '.';
            else {
                cleanedPage[j] = page[j];
            }
        notePadAscii.append(QString(reinterpret_cast <char *>(cleanedPage)));
        notePadAscii.append("\n\n");

        for (int j=0;j<2;j++) {
            uint16_t address = static_cast<uint16_t>(i*FP_PAGE_SIZE+j*FP_PAGE_SIZE/2);
            ss << "0x" << setfill('0') << setw(4) << hex << address << ": ";
            for (int k=0;k<FP_PAGE_SIZE/2;k++)
                ss << "0x" << setfill('0') << setw(2) << hex << static_cast<int>(page[j*FP_PAGE_SIZE/2+k]) << " ";
            ss << endl;
        }
    }
    QString notePadHex = QString::fromStdString(ss.str());


    NotePadDialog *notePadDialog = new NotePadDialog(this);
    notePadDialog->show();
    //    QString notePadText = "0x01 0x02 0x03 x04 0x05 0x06 0x07 0x08 0x09 x0a 0x0b 0x0c 0x0d 0x0e 0x0f \n";
    //    notePadText.append("0x11 0x12 0x13 x14 0x15 0x16 0x17 0x18 0x19 x1a 0x1b 0x1c 0x1d 0x1e 0x1f \n");
    notePadDialog->setHexText(notePadHex);
    notePadDialog->setAsciiText(notePadAscii);
    return retCode;;
}

int SfgQt::createWriteNotePadDialog() {
    int retCode = FP_SUCCESS;
    qDebug("NotePad Dialog");
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    WriteNotepadDialog *writeNotepadDialog = new WriteNotepadDialog(this);
    this->writeNotepadDialog = writeNotepadDialog;
    writeNotepadDialog->show();
    connect(writeNotepadDialog,SIGNAL(accepted()),this,SLOT(doWriteNotepad()));
    return retCode;
}

int SfgQt::doWriteNotepad() {
    int retCode = FP_SUCCESS;
    char pageText[FP_PAGE_SIZE+1];
    for (int i=0;i<FP_PAGE_SIZE+1;i++)
        pageText[i]='\0';

    uint8_t pageNo = this->writeNotepadDialog->getPageNo();
    QString notepadText = this->writeNotepadDialog->getNotepadText();
    qDebug() << "notepadText as QString: " << notepadText;
    QByteArray ba = notepadText.toLatin1();
    strncpy(pageText,ba.data(),32);

    if ((retCode = protocol->fpWriteNotepad(pageNo-1,reinterpret_cast<uint8_t *>(pageText))) != FP_SUCCESS) {
        protocolError(retCode,"fpWriteNotepad");
        return retCode;
    }
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Notepad page was successfully written</font>");
    return retCode;
}

int SfgQt::clearNotepad() {
    int retCode = FP_SUCCESS;
    uint8_t page[FP_PAGE_SIZE];
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    /* Clear the page */
    for (int i=0;i<FP_PAGE_SIZE;i++)
        page[i] = '\0';
    /* write to all pages */
    for (uint8_t i=0;i<FP_NO_OF_PAGES;i++)
        if ((retCode = protocol->fpWriteNotepad(i,page)) != FP_SUCCESS) {
            protocolError(retCode,"fpWriteNotepad");
            qDebug() << "Error when writingng notepad";
            return retCode;
        }
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Notepad was successfully cleared</font>");
    return retCode;
}
int SfgQt::captureFPImage() {
    int retCode;

    qDebug("Capture fingerprint image");
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    cancel = false;
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Please put your finger onto the reader</font>");
    qApp->processEvents();

    if ((retCode = takeFingerprint()) != FP_SUCCESS)
        return retCode;

    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Fingerprint taken!\n You may now upload the image</font>");
    ui_sfgQt->statusBar->showMessage("Fingerprint taken! You may now upload the image");
    ui_sfgQt->uploadImageButton->setEnabled(true);
    ui_sfgQt->progressBar->setValue(0);
    return retCode;
}

int SfgQt::continuousCaptureFPImage() {
    int retCode;

    qDebug("Continuous capture fingerprint image");
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    cancel = false;
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Please put your finger onto the reader</font>");
    qApp->processEvents();

    while (true) {
        if ((retCode = waitForFingerprint()) == FP_SUCCESS) {
            ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Fingerprint taken!\n You may now upload the image</font>");
            ui_sfgQt->statusBar->showMessage("Fingerprint taken! You may now upload the image");
            ui_sfgQt->uploadImageButton->setEnabled(true);
            ui_sfgQt->progressBar->setValue(0);
            break;
        }
        if (this->cancel)
            break;
    }
    ui_sfgQt->progressBar->setValue(0);
    return retCode;
}
int SfgQt::takeFingerprint() {
    int retCode=FP_SUCCESS;
    if (cancel)
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Operation was cancelled</font>");
    else
        if ((retCode = waitForFingerprint()) == ERR_NO_FINGER) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">No finger found on the reader. Timeout!</font>");
            ui_sfgQt->progressBar->setValue(0);
        }
    return retCode;
}

int SfgQt::waitForFingerprint() {
    int progress,retCode;
    progress = 0;
    this->cancel = false;
    /* wait until we see the finger on the reader */

    while ((retCode=protocol->fpGenImg()) != FP_SUCCESS){
        progress++;
        switch (retCode) {
        case ERR_NO_FINGER:
            QThread::msleep(100);
            ui_sfgQt->progressBar->setValue(progress);
            break;
        case ERR_RECEIVING_DATA_PACKAGE:
            protocolError(retCode,"fpGenImg");
            //            ui_sfgQt->statusTextEdit->document()->setPlainText("Error receiving the data package");
            qDebug() << "Error receiving the data package";
            return retCode;
        case ERR_ENROLL:
            protocolError(retCode,"fpGenImg");
            //            ui_sfgQt->statusTextEdit->document()->setPlainText("Error reading the finger image");
            qDebug() << "Enroll Error";
            return retCode;
        }
        qApp->processEvents();
        if (this->cancel) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Operation was cancelled</font>");
            ui_sfgQt->progressBar->setValue(0);
            return retCode;
        }
        if (progress >100) {
            ui_sfgQt->progressBar->setValue(0);
            return ERR_NO_FINGER;
        }
    }
    return retCode;
}

int SfgQt::uploadFPImage() {

    int i,retCode,progress;

    uint8_t *fingerPrintLine=nullptr;
    fingerPrintLine = new uint8_t[dataPacketSize];
    uint8_t packetType;
    uint8_t fingerImage[FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH*3],*fingerImagePtr,tmp;
    QByteArray pixelArray;
    QPixmap fingerprintPixmap;

    this->cancel = false;
    ui_sfgQt->statusTextEdit->setHtml("");

    if ((retCode = protocol->fpUpImage()) != FP_SUCCESS) {
        protocolError(retCode,"fpUpImage");
        delete[] fingerPrintLine;
        return retCode;
    }
    else
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Uploading image, please wait!</font>");

    packetType=0;
    i=0;
    progress = 0;
    this->fingerPrintCompressedImagePtr = this->fingerPrintCompressedImage;
    qDebug() << "data packet size: " << dataPacketSize;

    while (packetType != FP_END_PACKET) {
        if ((retCode=protocol->fpReadDataBuffer(&packetType,&dataPacketSize,fingerPrintLine)) != FP_SUCCESS) {
            protocolError(retCode,"fpReadDataBuffer");
            qDebug() << "Could not read image, error code: " << retCode;
            delete[] fingerPrintLine;
            return retCode;
        }
        else {
            progress = int(100.0*i/(FP_IMAGE_HEIGHT*(128.0/dataPacketSize)));
            ui_sfgQt->progressBar->setValue(progress);

            for (int j=0;j<dataPacketSize;j++)
                *this->fingerPrintCompressedImagePtr++ = fingerPrintLine[j];
            i++;
        }
    }
    delete[] fingerPrintLine;

    ui_sfgQt->progressBar->setValue(100);
    qDebug() << endl << i << "packets have been read";
    qApp->processEvents();
    if (this->cancel) {
        delete[] fingerPrintLine;
        return retCode;
    }
    this->fingerPrintCompressedImagePtr = this->fingerPrintCompressedImage;
    /* uncompress the data */
    fingerImagePtr = fingerImage;

    for (int j=FP_IMAGE_HEIGHT-1;j>-1;j--)
        for (int k=0;k<FP_IMAGE_WIDTH/2;k++) {
            tmp = this->fingerPrintCompressedImage[j*FP_IMAGE_WIDTH/2+k];
            for (int l=0; l<3; l++)
                *fingerImagePtr++ = tmp&0xf0;
            for (int l=0; l<3; l++)
                *fingerImagePtr++ = tmp&0x0f << 4;
        }
    qDebug() << "Pixmap data are ready";

    QImage fingerprintImage(fingerImage,FP_IMAGE_WIDTH,FP_IMAGE_HEIGHT,QImage::Format_RGB888);
    QPixmap img = QPixmap::fromImage(fingerprintImage);

    ui_sfgQt->fingerPrintImage->setPixmap(img);
    ui_sfgQt->statusTextEdit->setHtml("");
    ui_sfgQt->saveImageButton->setEnabled(true);

    return FP_SUCCESS;
}

int SfgQt::saveFPImage(QString filename) {
    int retCode;

    if (filename == nullptr) {
        qDebug() << "null ptr";
        return -1;
    }
    else
        qDebug() << "file name: " << filename;
    if (filename.endsWith(".bmp"))
        qDebug() << "file extension ok ";
    else {
        qDebug() << "file extension .bmp added";
        filename.append(".bmp");
    }
    string fn = filename.toStdString();
    const char *s = fn.c_str();

    QFileDialog *saveFPImageDialog = static_cast<QFileDialog *>(QObject::sender());
    imageDir = saveFPImageDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(imageDir);

    if ((retCode=protocol->fpCreateBitmap(this->fingerPrintCompressedImage,s))!= FP_SUCCESS) {
        protocolError(retCode,"fpCreateBitmap");
        qDebug() << "Could not save the fingerprint";
    }
    return 1;
}

void SfgQt::cancelOperation() {
    this->cancel=true;
}

int SfgQt::emptyDB() {
    int retCode;

    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }

    qDebug() << "empty the fingerprint database";
    if ((retCode = protocol->fpEmpty()) != FP_SUCCESS) {
        protocolError(retCode,"fpEmpty");
        qDebug() << "Could not empty the database. Error code was:" << retCode;
        return retCode;
    }
    /* clear out the table */
    ui_sfgQt->fpTable->clearContents();
    qDebug() << "Database successfully cleared";
    ui_sfgQt->confidenceLineEdit->setText("");
    return retCode;
}

int SfgQt::deleteDB_Entry() {
    int retCode=FP_SUCCESS;
    uint8_t templateList[32];

    QList<QTableWidgetSelectionRange> *selectedEntries= new QList<QTableWidgetSelectionRange>();
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }

    qDebug() << "delete entries from the fingerprint database";
    /* find which entries must be deleted from the fingerprint table */
    *selectedEntries = ui_sfgQt->fpTable->selectedRanges();
    if (selectedEntries->isEmpty()) {
        qDebug() << "No entry selected";
        return FP_SUCCESS;
    }
    /* get the currently occupied database entries */

    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS)
        protocolError(retCode,"fpReadConList");
    qDebug() << "Template positions:";
    QString hexString=QString("");
    for (int i=0;i<2;i++) {
        for (int j=0;j<16;j++)
            hexString.append(QString("0x%1 ").arg(templateList[16*i+j],2,16,QLatin1Char('0')));
        qDebug().noquote() << hexString;
        hexString=QString("");
    }

    int length = selectedEntries->length();
    qDebug() << "Length of list: " << length;
    for (int i=0; i<length;i++) {
        qDebug() << "top: " << selectedEntries->at(i).topRow();
        qDebug() << "bottom: " << selectedEntries->at(i).bottomRow();
        uint16_t top = static_cast<uint16_t>(selectedEntries->at(i).topRow());
        uint16_t bottom = static_cast<uint16_t>(selectedEntries->at(i).bottomRow());
        uint16_t noOfRows = bottom-top+1;
        qDebug() << "Deleting " << noOfRows << " rows starting from row " << top;
        if ((retCode = protocol->fpDeleteChar(top,noOfRows)) != FP_SUCCESS) {
            qDebug() << "Error when deleting template, error code: " << retCode;
            return retCode;
        }
        else {
            qDebug() << "Entries successfully deleted";

            for (int j=0;j<noOfRows;j++) {
                qDebug() << "Removing entries " << top +j;
                QTableWidgetItem *w=ui_sfgQt->fpTable->takeItem(top+j,0);
                delete w;
            }
            /* deleting the entries in the table */
        }
    }
    /* print the templatelist for control */
    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS)
        protocolError(retCode,"fpReadConList");
    qDebug() << "Template positions:";
    hexString=QString("");
    for (int i=0;i<2;i++) {
        for (int j=0;j<16;j++)
            hexString.append(QString("0x%1 ").arg(templateList[16*i+j],2,16,QLatin1Char('0')));
        qDebug().noquote() << hexString;
        hexString=QString("");
    }
    ui_sfgQt->confidenceLineEdit->setText("");
    return retCode;
}
/* exit the application */
/* save settings before */
void SfgQt::close() {

    /* if the serial line is not open then we cannot get at the settings */

    if (QObject::sender() == qApp) {
        saveSettings();
        qDebug() << "Closing serial connection";
        protocol->fpClose();
        serialIsOpen = false;
    }

    qApp->exit();
}
/* save the contents of t
he fingerprint table to the settings file */
/* the settings file is ~/.config/UCC/sfgQt.conf                   */

void SfgQt::saveSettings() {
    QSettings settings("UCC","sfgQt");
    qDebug() << "Save settings";
    qDebug() << "port: " << QString::fromStdString(port);
    qDebug() << "baudrate: " << baudrate;
    settings.setValue("port",QString::fromStdString(port));
    settings.setValue("baudrate",static_cast<int>(baudrate));
    if (templateDir != QString(""))
            settings.setValue("templateDir",templateDir);
    if (imageDir != QString(""))
            settings.setValue("imageDir",imageDir);
    /* get at all items in the table */

    QList<fpListEntry> fpList;
    for (int i=0;i<this->getFpMaxTemplates();i++) {
        QTableWidgetItem *fpItem = ui_sfgQt->fpTable->takeItem(i-1,1);
        if (fpItem != nullptr) {
            qDebug() << "Item in row " << i << ": " << fpItem->text();
            fpListEntry *fpListEntryPtr = new(fpListEntry);
            fpListEntryPtr->index = i;
            fpListEntryPtr->itemText = fpItem->text();
            fpList.append(*fpListEntryPtr);
        }
    }
    qDebug() << "Found " << fpList.length() << "items in fpTable";

    if (serialIsOpen)
        return;
    QList <int> templateList;
    templateList = getTemplateTable();
    /* check consistency */
    if (templateList.length() != fpList.length() )
        return;


    for (int i=0;i<fpList.length();i++)
        qDebug() << "Item " << fpList.at(i).index << " is " << fpList.at(i).itemText;

    /* generate an array with index and text for the entries in the fingerprint table */
    settings.beginWriteArray("fpTemplates");
    for (int i=0;i<fpList.length();i++) {
        settings.setArrayIndex(i);
        settings.setValue("fpIndex",fpList.at(i).index);
        settings.setValue("fpItemText",fpList.at(i).itemText);
    }
    settings.endArray();
    qDebug() << "Settings will be written to " << settings.fileName();
}
void SfgQt::readSettings() {
    QSettings settings("UCC","sfgQt");
    qDebug() << "Read settings";
    QList<fpListEntry> fpList;
    this->settingsSize = settings.beginReadArray("fpTemplates");
    qDebug() << "settings size: " << this->settingsSize;
    for (int i=0;i<this->settingsSize;i++){
        settings.setArrayIndex(i);
        fpListEntry fpTemplate;
        fpTemplate.index = settings.value("fpIndex").toInt();
        fpTemplate.itemText = settings.value("fpItemText").toString();
        qDebug() << "index: " << fpTemplate.index << " string: " << fpTemplate.itemText;
        QTableWidgetItem *fpItem = new QTableWidgetItem(fpTemplate.itemText);
        ui_sfgQt->fpTable->setItem(fpTemplate.index-1,1,fpItem);
    }
    if (checkConsistency())
        qDebug() << "Settings are consistent";
    else{
        QMessageBox *inconsistentMessage = new QMessageBox(this);
        inconsistentMessage->setWindowTitle("Bad consistency check");
        QString errMsg = QString("Hardware and fingerprint table are inconsistent\n");
        errMsg.append("Please consider clearing the template database\n");
        errMsg.append("and enrolling the prints from scratch");
        inconsistentMessage->setText(errMsg);
        inconsistentMessage-> show();
    }
}

/* Check that the number of entries in the fingerprint table corresponds to */
/* the number of occupied templates in the fingerprint module flash         */
/* Then check that the positions correspond                                 */

bool SfgQt::checkConsistency() {
    uint16_t noOfTemplates;
    uint8_t templateList[32];
    QList<int> fpTemplateList;
    int retCode;

    QSettings settings("UCC","sfgQt");
    qDebug() << "Consistency check";

    if ((retCode = protocol->fpTemplateNumber(&noOfTemplates)) == FP_SUCCESS)
        qDebug() <<"Number of valid templates: " << noOfTemplates;
    else {
        protocolError(retCode,"fpTemplateNumber");
        return false;
    }
    if (noOfTemplates == 0)
        qDebug() << "The templates library is empty";
    else
        qDebug() << "Number of occupied templates: " << noOfTemplates;

    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS) {
        protocolError(retCode,"fpReadConList");
        return false;
    }
    qDebug() << "Template positions:";
    QString hexString=QString("");
    for (int i=0;i<2;i++) {
        for (int j=0;j<16;j++)
            hexString.append(QString("0x%1 ").arg(templateList[16*i+j],2,16,QLatin1Char('0')));
        qDebug().noquote() << hexString;
        hexString=QString("")
;    }
    for (int i=0;i<32;i++) {
        uint8_t templ = templateList[i];
        uint8_t mask = 1;
        for (int j=0;j<8;j++) {
            if (templ & mask) {
                fpTemplateList.append(8*i+j);
                //                qDebug() << "template at pos " << 8*i+j;
            }
            mask <<=1;
        }
    }

    qDebug() << "settings size: " << settingsSize << "no of templates: " << fpTemplateList.length();
    if (fpTemplateList.length() != settingsSize)
        return false;

    this->settingsSize = settings.beginReadArray("fpTemplates");
    //    qDebug() << "settings size: " << this->settingsSize;
    for (int i=0;i<this->settingsSize;i++){
        settings.setArrayIndex(i);
        //        qDebug() << "hw: " << fpTemplateList.at(i);
        //        qDebug() << "settings index: " << settings.value("fpIndex").toInt();
        if (fpTemplateList.at(i) != settings.value("fpIndex").toInt() )
            return false;
    }
    return true;

}

/* match a fingerprint                                                       */
/* select an entry in the fingerprint table to which you                     */
/* want to match the print                                                   */
/* retrieve this template from flash an load int into character file 2       */
/* read the fingerprint to match with and convert into into character file 1 */
/* match the two character files                                             */

int SfgQt::match() {
    QList<QTableWidgetSelectionRange> *selectedEntries= new QList<QTableWidgetSelectionRange>();
    int retCode;
    uint16_t templateNo,score;
    /* make sure the serial port is open */
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    cancel = false;
    /* check that exactly 1 item in the fingerprint table is selected */
    *selectedEntries = ui_sfgQt->fpTable->selectedRanges();
    if (selectedEntries->isEmpty()) {
        QMessageBox *noSelectionMessage = new QMessageBox(this);
        noSelectionMessage->setWindowTitle("No template selected");
        noSelectionMessage->setText("Please select exactly 1 table entry for matching");
        noSelectionMessage-> show();
        return -1;
    }
    int length = selectedEntries->length();
    if (length != 1) {
        QMessageBox *tooManySelectionsMessage = new QMessageBox(this);
        tooManySelectionsMessage->setWindowTitle("More than 1 template selected");
        tooManySelectionsMessage->setText("Please select exactly 1 table entry for matching");
        tooManySelectionsMessage-> show();
        return -1;
    }
    /* templateNo is the template to match against */
    templateNo = static_cast<uint16_t>(selectedEntries->at(0).topRow());
    qDebug() << "template number: " << templateNo;

    /* now read the fingerprint */

    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Please put your finger onto the reader</font>");
    qApp->processEvents();

    if ((retCode = takeFingerprint()) != FP_SUCCESS) {
        //        ui_sfgQt->statusTextEdit->setHtml("font color=\"red\">Could not read the fingerprint</font>");
        return retCode;
    }
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Fingerprint successfully read</font>");
    }
    if (ui_sfgQt->matchPreviewCheckbox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;

    qDebug() << "generate character file 1";
    if ((retCode = protocol->fpImg2Tz(1)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(1)");
        return retCode;
    }
    if ((retCode = protocol->fpLoad(2,templateNo))!= FP_SUCCESS) {
        protocolError(retCode,"fpLoad");
        qDebug() << "Could not restore character file 2 from the template database in flash";
        return retCode;
    }
    if ((retCode = protocol->fpMatch(&score)) != FP_SUCCESS) {
        if (retCode ==ERR_PRINTS_DONT_MATCH)
            return 0;
        else
            qDebug() << "Error when matching: retCode: " << retCode;
    }
    else {
        qDebug() << "Matching successful, score was: " << score;
        if (score > 100)
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Fingerprints match</font>");
        else {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Confidence level is low</font>");
        }
        ui_sfgQt->confidenceLineEdit->setText(QString::number(score));
    }
    ui_sfgQt->progressBar->setValue(0);
    return true;
}

int SfgQt::search() {
    /* go through all entries and try to match */
    int retCode;
    uint16_t noOfTemplates;
    uint16_t score;
    uint8_t templateList[32];
    QList<int> fpTemplateList;

    retCode=FP_SUCCESS;
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    /* make sure there is something to compare with */
    if ((retCode = protocol->fpTemplateNumber(&noOfTemplates)) == FP_SUCCESS)
        qDebug() <<"Number of valid templates: " << noOfTemplates;
    else {
        protocolError(retCode,"fpTemplateNumber");
        return false;
    }
    if (noOfTemplates == 0) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">The template database if empty</font>");
        qDebug() << "The templates library is empty";
        return retCode;
    }
    else
        qDebug() << "Number of occupied templates: " << noOfTemplates;
    cancel = false;
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Please put your finger onto the reader</font>");
    qApp->processEvents();

    if ((retCode = takeFingerprint()) != FP_SUCCESS) {
        //        ui_sfgQt->statusTextEdit->setHtml("font color=\"red\">Could not read the fingerprint</font>");
        return retCode;
    }
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Fingerprint successfully read</font>");
    }
    if (ui_sfgQt->matchPreviewCheckbox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;

    qDebug() << "generate character file 1";
    if ((retCode = protocol->fpImg2Tz(1)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(1)");
        return retCode;
    }

    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS) {
        protocolError(retCode,"fpReadConList");
        return false;
    }
    qDebug() << "Template positions:";
    QString hexString=QString("");
    for (int i=0;i<2;i++) {
        for (int j=0;j<16;j++)
            hexString.append(QString("0x%1 ").arg(templateList[16*i+j],2,16,QLatin1Char('0')));
        qDebug().noquote() << hexString;
        hexString = QString("");
    }
    for (int i=0;i<32;i++) {
        uint8_t templ = templateList[i];
        uint8_t mask = 1;
        for (int j=0;j<8;j++) {
            if (templ & mask) {
                fpTemplateList.append(8*i+j);
                //                qDebug() << "template at pos " << 8*i+j;
            }
            mask <<=1;
        }
    }
    for (uint16_t pageId=0; pageId<fpTemplateList.length();pageId++) {
        if ((retCode = protocol->fpLoad(2,pageId))!= FP_SUCCESS) {
            protocolError(retCode,"fpLoad");
            qDebug() << "Could not restore character file 2 from the template database in flash";
            return retCode;
        }
        if ((retCode = protocol->fpMatch(&score)) != FP_SUCCESS) {
            if (retCode == ERR_PRINTS_DONT_MATCH)
                continue;
            else {
                protocolError(retCode,"fpMatch");
                qDebug() << "Error when matching: retCode: " << retCode;
                return retCode;
            }
        }
        else {
            qDebug() << "Fingerprint found on position " << pageId << " score was: " << score;
            QString foundMsg = "<font color=\"blue\">Fingerprint matches template ";
            foundMsg.append(QString::number(pageId+1));
            foundMsg.append(":<br>");

            QTableWidgetItem *fingerTextItem = ui_sfgQt->fpTable->item(pageId,0);
            if (fingerTextItem != nullptr) {
                QString fingerText = fingerTextItem->text();
                qDebug() << "finger text: " << fingerText;
                foundMsg.append(fingerText);
            }
            else {
                qDebug() << "fingerTextItem is nullptr" ;
            }
            foundMsg.append("</font>");
            qDebug() << "found message: " << foundMsg ;
            ui_sfgQt->statusTextEdit->setHtml(foundMsg);
            ui_sfgQt->confidenceLineEdit->setText(QString::number(score));
            ui_sfgQt->progressBar->setValue(0);
            return retCode;
        }
    }

    ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Fingerprints not found in template database</font>");
    ui_sfgQt->progressBar->setValue(0);
    return retCode;
}

int SfgQt::quickSearch() {
    /* get a fingerprint                                                                       */
    /* get the first and last entry in the template database and use the FP_CMD_SEARCH command */
    /* to let the fingerprint module compare the new fingerprint with those registered in the  */
    /* template database                                                                       */
    int retCode;
    uint16_t noOfTemplates;
    uint16_t pageId, score;
    QList<int> fpTemplateList;

    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    /* make sure there is something to compare with */
    if ((retCode = protocol->fpTemplateNumber(&noOfTemplates)) == FP_SUCCESS)
        qDebug() <<"Number of valid templates: " << noOfTemplates;
    else {
        protocolError(retCode,"fpTemplateNumber");
        return false;
    }
    if (noOfTemplates == 0) {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">The template database if empty</font>");
        qDebug() << "The templates library is empty";
        return retCode;
    }
    else
        qDebug() << "Number of occupied templates: " << noOfTemplates;
    cancel = false;
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Please put your finger onto the reader</font>");
    qApp->processEvents();

    if ((retCode = takeFingerprint()) != FP_SUCCESS) {
        //        ui_sfgQt->statusTextEdit->setHtml("font color=\"red\">Could not read the fingerprint</font>");
        return retCode;
    }
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"blue\">Fingerprint successfully read</font>");
    }
    if (ui_sfgQt->matchPreviewCheckbox->isChecked())
        if ((retCode = uploadFPImage()) != FP_SUCCESS)
            return retCode;

    qDebug() << "generate character file 1" ;
    if ((retCode = protocol->fpImg2Tz(1)) != FP_SUCCESS) {
        protocolError(retCode,"fpImg2Tz(1)");
        return retCode;
    }

    fpTemplateList = getTemplateTable();
    /*
    qDebug() << "templates" ;
    for (int i=0;i<fpTemplateList.length();i++)
        qDebug() << dec << fpTemplateList[i] << " ";
    qDebug() ;
    */
    uint16_t firstActiveTemplate = static_cast<uint16_t> (fpTemplateList.first());
    uint16_t lastActiveTemplate  = static_cast<uint16_t> (fpTemplateList.last());
    uint16_t noOfPages=lastActiveTemplate-firstActiveTemplate+1;

    qDebug() << "First template: " << firstActiveTemplate << ", last template: " << lastActiveTemplate ;
    if ((retCode = protocol->fpSearch(1,firstActiveTemplate,noOfPages,&pageId,&score))!= FP_SUCCESS) {
        if (retCode == ERR_NO_MATCH)
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Fingerprint not found in template database</font>");
        else
            protocolError(retCode,"fpSearch");
        ui_sfgQt->progressBar->setValue(0);
        return retCode;
    }
    qDebug() << "finger at page " << pageId+1 << "matches with score " << score ;
    QString foundMsg = "<font color=\"blue\">Fingerprint matches template ";
    foundMsg.append(QString::number(pageId+1));
    foundMsg.append(":<br>");

    QTableWidgetItem *fingerTextItem = ui_sfgQt->fpTable->item(pageId,0);
    if (fingerTextItem != nullptr) {
        QString fingerText = fingerTextItem->text();
        qDebug() << "finger text: " << fingerText ;
        foundMsg.append(fingerText);
    }
    else {
        qDebug() << "fingerTextItem is nullptr" ;
    }
    foundMsg.append("</font>");
    //    qDebug() << "found message: " << foundMsg ;
    ui_sfgQt->statusTextEdit->setHtml(foundMsg);
    ui_sfgQt->confidenceLineEdit->setText(QString::number(score));
    ui_sfgQt->progressBar->setValue(0);
    return retCode;
}

int SfgQt::continuousSearch() {
    int retCode=FP_SUCCESS;
    qDebug() << "continuous search" ;
    cancel = false;
    while (!cancel) {
        quickSearch();
        QThread::sleep(2);
    }
    ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Operation was cancelled</font>");
    return retCode;
}
int SfgQt::getRandomNumber() {
    int retCode;
    uint32_t randomNumber;
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    if ((retCode = protocol->fpGetRandomNumber(&randomNumber)) !=FP_SUCCESS) {
        protocolError(retCode,"fpGetRandomNumber");
        return retCode;
    }
    QString numberString;
    numberString.setNum(randomNumber,10);
    ui_sfgQt->randomNumber->setText(numberString);
    return retCode;
}

void SfgQt::protocolError(int errCode,QString protocolCall) {
    QMessageBox *protocolErrMsg = new QMessageBox(this);
    QString errMsg = QString("Protocol error in ");
    errMsg.append(protocolCall);

    QString errNumString;
    if (errCode < 0) {
        errMsg.append("\nError message was:   ");
        errNumString.setNum(static_cast<char>(errCode),10);
    }
    else {
        errMsg.append("\nError message was:   0x");
        errNumString.setNum(static_cast<uint8_t>(errCode),16);
    }
    errMsg.append(errNumString);
    errMsg.append("\n");

    protocolErrMsg->setWindowTitle("Protocol error");
    errMsg.append(QString::fromStdString(protocol->fpGetErrorString(static_cast<char>(errCode))));

    protocolErrMsg->setText(errMsg);
    protocolErrMsg-> show();
}

int SfgQt::uploadTemplateDB(QString filename) {
    QSettings settings("UCC","sfgQt");

    int retCode=FP_SUCCESS;
    uint16_t templateNo;
    //    uint16_t noOfTemplates;
    QList<int> fpTemplateList;
    int updateIncrement;

    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    fpTemplateList = getTemplateTable();
    if (fpTemplateList.length() == 0)
        qDebug() << "The templates library is empty" ;
    else
        qDebug() << "Number of occupied templates: " << fpTemplateList.length();

    /* needed for the progress bar */
    updateIncrement = 100/fpTemplateList.length();

    if (filename == nullptr) {
        qDebug() << "null ptr" ;
        return -1;
    }
    else
        qDebug() << "file name: " << filename ;
    if (filename.endsWith(".txt"))
        qDebug() << "file extension ok ";
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Sorry! Filename must have extension '.txt'</font>");
        return -1;
    }

    QFileDialog *uploadTemplateDBDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = uploadTemplateDBDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);

    ofstream charFile;
    charFile.open(filename.toStdString());
    /* write the magic string for template databases */
    qDebug() << "Length of template list" ;
    charFile << "TemplateDB" << endl;
    for (int j=0;j<fpTemplateList.length();j++) {
        templateNo = static_cast<uint16_t>(fpTemplateList[j]);
        charFile << "Template " << dec <<templateNo+1 << endl;
        QTableWidgetItem *fingerTextItem = ui_sfgQt->fpTable->item(templateNo,0);
        if (fingerTextItem != nullptr) {
            QString fingerText = fingerTextItem->text();
            charFile << fingerText.toStdString() << endl;
            qDebug() << "finger text: " << fingerText ;
        }
        else {
            qDebug() << "fingerTextItem is nullptr" ;
        }
        /* copy from the template DB to character file 1 */

        if ((retCode = protocol->fpLoad(1,templateNo)) != FP_SUCCESS) {
            protocolError(retCode,"fpLoad");
            return retCode;
        }
        if ((retCode = writeCharFile(charFile,1)) != FP_SUCCESS)
            return retCode;
        ui_sfgQt->progressBar->setValue(j*updateIncrement);
    }
    ui_sfgQt->progressBar->setValue(0);
    charFile.close();
    QString msg;
    msg="<font color=\"blue\">Templates successfully uploaded to file:<br>";
    msg.append(filename);
    msg.append("</font>");
    ui_sfgQt->statusTextEdit->setHtml(msg);
    return retCode;
}

int SfgQt::downloadTemplateDB(QString filename) {
    int retCode=FP_SUCCESS;
    int templateCount,updateIncrement;
    //    int templateNo;
    string line;
    string magic = "TemplateDB";
    string templateString;
    stringstream ss;
    QString fingerName;
    QSettings settings("UCC","sfgQt");
    qDebug() << "download templateDB" ;

    if (filename == nullptr) {
        qDebug() << "null ptr" ;
        return -1;
    }
    else
        qDebug() << "file name: " << filename ;
    if (filename.endsWith(".txt"))
        qDebug() << "file extension ok ";
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Sorry! Filename must have extension '.txt'</font>");
        return -1;
    }
    QFileDialog *downloadTemplateDBDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = downloadTemplateDBDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);

    ifstream templateFile;
    templateFile.open(filename.toStdString());
    if (!templateFile.good()) {
        qDebug() << "Could not open file " << filename ;
        return -1;
    }
    /* check how many templates there are in this file */
    templateCount = 0;
    while (!templateFile.eof()) {
        getline(templateFile,line);
        if (line.find("Template") != string::npos)
            templateCount++;
    }
    qDebug() << "Template count: " << templateCount ;
    updateIncrement = 100/templateCount;

    templateFile.clear();
    templateFile.seekg(0);

    getline(templateFile,line);
    if (line != magic) {
        qDebug() <<  QString() << "is not a valid template database file" ;
    }

    /* Empty the current database */
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Overwrite template database?",
                                  "This will erase the entire template database\nOk to overwrite?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::No) {
        return retCode;;
    }
    if ((retCode = protocol->fpEmpty())) {
        qDebug() << "Error when attempting to clear the template database" ;
    }
    qDebug() << "Template database was successfully cleared!" ;
    ui_sfgQt->fpTable->clearContents();
    templateCount = 0;
    while (!templateFile.eof()) {
        /* get the template position from the file */
        if ((retCode = downloadTemplate(templateFile,-1)) == FP_SUCCESS)
            templateCount++;
        ui_sfgQt->progressBar->setValue(templateCount*updateIncrement);
        qApp->processEvents();
    }
    ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"blue\">Template database successfully downloaded</font>");
    ui_sfgQt->progressBar->setValue(0);
    templateFile.close();
    return retCode;
}

int SfgQt::writeCharFile(ostream& charFile, int charFileNo) {
    int retCode;
    uint8_t charFileData[FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS], *charFileDataPtr;
    uint8_t *fingerPrintLine,*fingerPrintLinePtr,packetType;

    fingerPrintLine = new uint8_t[dataPacketSize];
    qDebug() << "Start upload" ;
    if ((retCode = protocol->fpUpChar(static_cast<uint8_t>(charFileNo))) != FP_SUCCESS) {
        protocolError(retCode,"fpUpChar");
        return retCode;
    }

    packetType=0;
    charFileDataPtr=charFileData;
    /* collect the data */
    while (packetType != FP_END_PACKET) {
        if ((retCode=protocol->fpReadDataBuffer(&packetType,&dataPacketSize,fingerPrintLine)) != FP_SUCCESS) {
            protocolError(retCode,"fpReadDataBuffer");
            delete [] fingerPrintLine;
            return retCode;
        }
        else {
            fingerPrintLinePtr = fingerPrintLine;
            for (int i=0;i<dataPacketSize;i++) {
                *charFileDataPtr++ = *fingerPrintLinePtr++;
            }
        }
    }
    delete [] fingerPrintLine;

    charFileDataPtr=charFileData;
    for (int i=0;i<FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS;i++) {
        if (i==0)
            charFile << "0x" << hex <<setfill('0') <<setw(4) << i <<": ";
        if ((i!=0) && (!(i%16))) {
            charFile << endl;
            if ((i!=0) && (!(i%FP_DEFAULT_DATA_PKT_LENGTH)))
                charFile << endl;
            charFile << "0x" << hex <<setfill('0') <<setw(4) << i << ": ";
        }
        charFile << "0x" << hex <<setfill('0') <<setw(2) << int(*charFileDataPtr++) << " ";
    }

    //        qDebug() << "Reading packet " << i ;

    charFile << endl;
    qDebug() << FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS/dataPacketSize << " packets have been written" ;

    return FP_SUCCESS;
}

void SfgQt::getDirectoryPath() {
    QSettings settings("UCC","sfgQt");

    QString directoryName = QFileDialog::getExistingDirectory(this, tr("Fingerprint File Directory"),
                                                              imageDir,
                                                              QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    qDebug() << "Directory: " << directoryName;
    if (directoryName == "")
        return;
    else {
        ui_sfgQt->imageFileDirectoryLineEdit->setText(directoryName);
        imageDir = directoryName;
    }
}

void SfgQt::createSaveImageFileDialog() {
    /* check if an image file is available */

    if (!ui_sfgQt->saveImageButton->isEnabled()) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("No Fingerprint Image");
        notOpenMessage->setText("No Fingerprint Image is available\nPlease upload a fingerprint first");
        notOpenMessage-> show();
        return;
    }
    QFileDialog *saveImageFileDialog = new QFileDialog();
    saveImageFileDialog->setOption(QFileDialog::DontUseNativeDialog, true);
    //we need qt layout
    saveImageFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    saveImageFileDialog->setDirectory(imageDir);

    QObject *sender = QObject::sender();

    if (sender == ui_sfgQt->sourceButton) {
        qDebug() << "sender was source button" ;
        connect(saveImageFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(saveSourceImage(QString)));
        saveImageFileDialog->setWindowTitle("Enter filename for source image, extension: .txt");
    }

    else if (sender == ui_sfgQt->binImageButton) {
        qDebug() << "sender was bin button" ;
        connect(saveImageFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(saveBinImage(QString)));
        saveImageFileDialog->setWindowTitle("Enter filename for binary image, extension: .bin");
    }
    else if (sender == ui_sfgQt->thinImageButton) {
        qDebug() << "sender was thin image button" ;
        connect(saveImageFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(saveThinImage(QString)));
        saveImageFileDialog->setWindowTitle("Enter filename for thin image, extension.txt");
    }
    else {
         qDebug() << "sender was saveImageButton" ;
         connect(saveImageFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(saveFPImage(QString)));
         saveImageFileDialog->setWindowTitle("Enter filename for thin image, extension.bmp");
    }
    saveImageFileDialog->show();
    return;
}

void SfgQt::saveSourceImage(QString filename) {
    if (filename == "")
        qDebug() << "no filename given" ;

    if (!(filename.endsWith(".txt"))) {
        qDebug() << "filename should have .txt extension" ;
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Sorry, filename must have '.txt' extension</font>");
        return;
    } 

    qDebug() << "filename: " << filename ;

    /* check if file exists already */
    ifstream testExistance;
    testExistance.open(filename.toStdString());
    if (testExistance.is_open()) {
        qDebug() << "File " << filename << "exists" ;
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File exists", "File exists!\nOk to overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            testExistance.close();
            return;
        }
    }
    testExistance.close();

    QFileDialog *saveSourceImageDialog = static_cast<QFileDialog *>(QObject::sender());
    imageDir = saveSourceImageDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(imageDir);

    ofstream sourceFile;
    sourceFile.open(filename.toStdString());

    uint8_t *imageSourcePtr = this->fingerPrintCompressedImage;
    for (int i=0;i<FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH/2;i++) {
        if (i==0)
            sourceFile << "0x" << hex <<setfill('0') <<setw(4) << i <<": ";
        if ((i!=0) && (!(i%16))) {
            sourceFile << endl;
            sourceFile << "0x" << hex <<setfill('0') <<setw(4) << i << ": ";
        }
        sourceFile << "0x" << hex <<setfill('0') <<setw(2) << int(*imageSourcePtr++) << " ";
    }
    sourceFile << endl;
    sourceFile.close();
    return;
}

void SfgQt::saveThinImage(QString filename) {
    qDebug() << "save thin image" ;
    if (filename == "")
        qDebug() << "no filename given" ;

    if (!(filename.endsWith(".txt"))) {
        qDebug() << "filename should have .txt extension" ;
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Sorry, filename must have '.txt' extension</font>");
        return;
    }
    qDebug() << "filename: " << filename ;

    /* check if file exists already */
    ifstream testExistance;
    testExistance.open(filename.toStdString());
    if (testExistance.is_open()) {
        qDebug() << "File " << filename << "exists" ;
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File exists", "File exists!\nOk to overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            testExistance.close();
            return;
        }
    }
    testExistance.close();

    QFileDialog *saveThinImageDialog = static_cast<QFileDialog *>(QObject::sender());
    imageDir = saveThinImageDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(imageDir);

    ofstream sourceFile;
    sourceFile.open(filename.toStdString());
    if (!sourceFile.is_open()) {

        return;
    }

    uint8_t *imageSourcePtr = this->fingerPrintCompressedImage;
    for (int i=0;i<FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH/2;i++) {
        if (i==0)
            sourceFile << "0x" << hex <<setfill('0') <<setw(4) << i <<": ";
        /* get the next 2 pixel values */
        uint8_t tmp = *imageSourcePtr++;
        /* Here fore the upper 4 bits */
        for (int k=0;k<3;k++) {
            /* check if we have to start a new line */
            if ((i!=0) && (!((i*6+k)%16))) {
                sourceFile << endl;
                sourceFile << "0x" << hex <<setfill('0') <<setw(4) << i*6+k << ": ";
            }
            sourceFile << "0x" << hex <<setfill('0') <<setw(2) << int(tmp &0xf0) << " ";
        }
        /* here for the lower 4 bits */
        for (int k=0;k<3;k++) {
            /* check if we have to start a new line */
            if ((i!=0) && (!((i*6+3+k)%16))) {
                sourceFile << endl;
                sourceFile << "0x" << hex <<setfill('0') <<setw(4) << i*6+3+k << ": ";
            }
            sourceFile << "0x" << hex <<setfill('0') << setw(2) << int((tmp & 0xf)<< 4) << " ";
        }
    }
    sourceFile << endl;
    sourceFile.close();
    return;
}
void SfgQt::saveBinImage(QString filename) {
    qDebug() << "save binary image" ;
    if (filename == "")
        qDebug() << "no filename given" ;

    if (!(filename.endsWith(".bin"))) {
        qDebug() << "filename should have .bin extension" ;
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Sorry, filename must have '.bin' extension</font>");
        return;
    }
    qDebug() << "filename: " << filename ;

    /* check if file exists already */
    ifstream testExistance;
    testExistance.open(filename.toStdString());
    if (testExistance.is_open()) {
        qDebug() << "File " << filename << "exists" ;
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "File exists", "File exists!\nOk to overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            testExistance.close();
            return;
        }
    }
    testExistance.close();

    QFileDialog *saveBinImageDialog = static_cast<QFileDialog *>(QObject::sender());
    imageDir = saveBinImageDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(imageDir);

    ofstream binFile;
    binFile.open(filename.toStdString(),ios::out|ios::binary);
    if (!binFile.is_open()) {
        ui_sfgQt->statusTextEdit->document()->setHtml("<font color=\"red\">Could not open file for writing</font>");
        return;
    }
    binFile.write(reinterpret_cast<const char *>(fingerPrintCompressedImage),FP_IMAGE_HEIGHT*FP_IMAGE_WIDTH/2);
    binFile.close();
}


void SfgQt::createVerifyTemplateDialog() {

    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return;
    }
    VerifyTemplateDialog *verifyTemplateDialog = new VerifyTemplateDialog(this);
    this->verifyTemplateDialog = verifyTemplateDialog;
    connect(this->verifyTemplateDialog,SIGNAL(accepted()),this,SLOT(verifyTemplate()));
    verifyTemplateDialog->show();
}


int SfgQt::verifyTemplate() {
    int retCode;
    bool isValid;
    int templateToBeVerified = this->verifyTemplateDialog->getTemplateNo();
    qDebug() << "Template to be verified: " << templateToBeVerified ;
    if ((retCode =isValidTemplate(templateToBeVerified-1,&isValid)) != FP_SUCCESS)
        return retCode;
    if (isValid) {
        QString msg = "<font color=\"blue\">Template ";
        msg.append(QString::number(templateToBeVerified));
        msg.append(" is a valid template</font>");
        ui_sfgQt->statusTextEdit->document()->setHtml(msg);
    }
    else {
        QString msg = "<font color=\"red\">Template ";
        msg.append(QString::number(templateToBeVerified));
        msg.append(" is not valid");
        ui_sfgQt->statusTextEdit->setHtml(msg);
    }
    return retCode;
}

int SfgQt::isValidTemplate(int templateNo, bool *isValid){
    int retCode;
    uint16_t noOfTemplates;
    uint8_t templateList[32];
    QList<int> fpTemplateList;
    /* check if the database is empty */
    if ((retCode = protocol->fpTemplateNumber(&noOfTemplates)) == FP_SUCCESS) {
        qDebug() <<"Number of valid templates: " << noOfTemplates;
    }
    else {
        protocolError(retCode,"fpTemplateNumber");
        *isValid = false;
        return retCode;
    }
    if (noOfTemplates == 0) {
        *isValid = false;
        return retCode;
    }
    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS) {
        protocolError(retCode,"fpReadConList");
        return false;
    }
    qDebug() << "Template positions:";
    QString hexString=QString("");
    for (int i=0;i<2;i++) {
        for (int j=0;j<16;j++)
            hexString.append(QString("0x%1 ").arg(templateList[16*i+j],2,16,QLatin1Char('0')));
        qDebug().noquote() << hexString;
        hexString=QString("");
    }

    for (int i=0;i<32;i++) {
        uint8_t templ = templateList[i];
        uint8_t mask = 1;
        for (int j=0;j<8;j++) {
            if (templ & mask) {
                fpTemplateList.append(8*i+j);
                //                qDebug() << "template at pos " << 8*i+j ;
            }
            mask <<=1;
        }
    }
    for (int i=0; i<fpTemplateList.length(); i++)
        if ( templateNo == fpTemplateList[i]) {
            *isValid = true;
            return retCode;
        }
    *isValid = false;
    return retCode;
}

int SfgQt::createUploadTemplateDialog() {
    qDebug() << "upload template Dialog" ;
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    QFileDialog* uploadTemplateDialog = new QFileDialog();
    uploadTemplateDialog->setOption(QFileDialog::DontUseNativeDialog, true); //we need qt layout
    if ((QObject::sender() == ui_sfgQt->uploadTemplateButton) || (QObject::sender() == ui_sfgQt->uploadDBButton)){
        uploadTemplateDialog->setAcceptMode(QFileDialog::AcceptSave);
    }
    else {
        uploadTemplateDialog->setAcceptMode(QFileDialog::AcceptOpen);
    }
    uploadTemplateDialog->setDirectory(templateDir);
    QGridLayout *layout = static_cast<QGridLayout*>(uploadTemplateDialog->layout());
    QList< QPair<QLayoutItem*, QList<int> > > moved_items;
    for(int i = 0; i < layout->count(); i++) {
        int row, column, rowSpan, columnSpan;
        layout->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
        if (row >= 2) {
            QList<int> list;
            list << (row + 1) << column << rowSpan << columnSpan;
            moved_items << qMakePair(layout->takeAt(i), list);
            i--; // takeAt has shifted the rest items
        }
    }

    for(int i = 0; i < moved_items.count(); i++) {
        layout->addItem(moved_items[i].first,
                        moved_items[i].second[0],
                moved_items[i].second[1],
                moved_items[i].second[2],
                moved_items[i].second[3]);
    }

    QHBoxLayout *templateSelectionlayout = new QHBoxLayout();
    QLabel* uploadTemplateLabel = new QLabel("Template to be uploaded: ");
    if (QObject::sender() == ui_sfgQt->downloadTemplateButton)
        uploadTemplateLabel->setText("Template to be downloaded to position: ");
    templateSelectionlayout->addWidget(uploadTemplateLabel);

    uploadTemplateLabel->setAlignment(Qt::AlignLeft);
    uploadTemplateLabel->setAlignment(Qt::AlignVCenter);
    QSpinBox *templateSelector = new QSpinBox();
    this->templateSelector = templateSelector;
    templateSelector->setMinimum(1);
    templateSelector->setMaximum(150);
    templateSelector->setMaximumWidth(50);
    templateSelectionlayout->addWidget(templateSelector);
    templateSelectionlayout->addSpacing(400);
    layout->addLayout(templateSelectionlayout,2,0,1,4);

    uploadTemplateDialog->show();
    if (QObject::sender() == ui_sfgQt->uploadTemplateButton)
        connect(uploadTemplateDialog,SIGNAL(fileSelected(QString)),this,SLOT(uploadTemplate(QString)));
    else if(QObject::sender() == ui_sfgQt->uploadDBButton)
        connect(uploadTemplateDialog,SIGNAL(fileSelected(QString)),this,SLOT(uploadTemplateDB(QString)));
    else if (QObject::sender() == ui_sfgQt->downloadTemplateButton)
        connect(uploadTemplateDialog,SIGNAL(fileSelected(QString)),this,SLOT(downloadSingleTemplate(QString)));
    else
        connect(uploadTemplateDialog,SIGNAL(fileSelected(QString)),this,SLOT(downloadTemplateDB(QString)));
    return 1;
}

int SfgQt::uploadTemplate(QString filename) {
    int retCode=FP_SUCCESS;
    int templateNo;
    QList<int> fpTemplateList;

    qDebug() << "uploadTemplate " ;
    qDebug() << "Filename: " << filename ;
    templateNo = templateSelector->value();
    qDebug() << "Template No: " << templateNo ;;

    /* if we don't get a filename, just exit */
    if (filename == nullptr)
        return retCode;
    /* get a new image directory if it was changed */
    QFileDialog *sourceFileDialog = static_cast<QFileDialog *>(QObject::sender()) ;
    QString newTemplateDir = sourceFileDialog->directory().dirName();
    qDebug() << "New directory name: " << newTemplateDir;
    bool validTemplate;
    if ((retCode=isValidTemplate(templateNo-1,&validTemplate)) != FP_SUCCESS)
        return retCode;
    if (!validTemplate) {
        QString msg = "<font color=\"red\">Template ";
        msg.append(QString::number(templateNo));
        msg.append(" is not a valid template in the database");
        ui_sfgQt->statusTextEdit->setHtml(msg);
        return retCode;
    }
    qDebug() << "Template" << templateNo << "is a valid template in the database" ;

    QFileDialog *uploadTemplateDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = uploadTemplateDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);

    ofstream charFile;
    charFile.open(filename.toStdString());
    /* write the magic string for template databases */
    charFile << "Template " << templateNo << endl;
    QTableWidgetItem *fingerTextItem = ui_sfgQt->fpTable->item(templateNo-1,0);
    if (fingerTextItem != nullptr) {
        QString fingerText = fingerTextItem->text();
        charFile << fingerText.toStdString() << endl;
        qDebug() << "finger text: " << fingerText ;
    }
    else
        qDebug() << "fingerTextItem is nullptr" ;

    /* copy template to character file 1 */
    if ((retCode = protocol->fpLoad(1,static_cast<uint16_t>(templateNo-1))) != FP_SUCCESS) {
        protocolError(retCode,"fpLoad");
        return retCode;
    }

    if ((retCode = writeCharFile(charFile,1)) != FP_SUCCESS) {
        qDebug() << "error in writeCharFile" ;
        return retCode;
    }
    return retCode;
}
int SfgQt::downloadSingleTemplate(QString filename) {
    int retCode=FP_SUCCESS;
    int templateNo;
    bool isValid;
    string line;
    /* if we don't get a filename, just exit */
    if (filename == nullptr)
        return retCode;

    /* get a new image directory if it was changed */
    QFileDialog *downloadTemplateDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = downloadTemplateDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);

    templateNo = templateSelector->value();
    /* check if there is already a valid template at that position */
    if ((retCode = isValidTemplate(templateNo,&isValid))!= FP_SUCCESS)
        return retCode;
    if (isValid) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Template position occupied", "Overwrite?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No) {
            qDebug() << "No was clicked";
            return retCode;
        }
    }
    ifstream templateFile;
    templateFile.open(filename.toStdString(),ifstream::in);
    if (templateFile.is_open())
        qDebug() << "Character file successfully opened" ;
    else {
        qDebug() << "Could not open the characterfile\n";
    }
    if ((retCode = downloadTemplate(templateFile,templateNo)) != FP_SUCCESS)
        qDebug() << "Error when downloading the template" ;

    QString msg = "<font color=\"blue\">File:  was successfully downloaded to ";
    msg.append("template position ");
    msg.append(QString::number(templateNo));
    msg.append("</font>");
    ui_sfgQt->statusTextEdit->setHtml(msg);

    return retCode;
}

int SfgQt::downloadTemplate(ifstream& templateFile,int templateNo) {
    int retCode=FP_SUCCESS;
    string line;
    string magic="Template";

    /* read magic header */
    getline(templateFile,line);
    /* it we have an empty line, try next one */
    if(line[0]== '\0')
        getline(templateFile,line);
    /* if we are at the end of the file, return */
    if (templateFile.eof())
        return retCode;
    qDebug() << QString::fromStdString(line) ;

    /* find out if the file is a template file */
    stringstream ss = stringstream(line);
    string templateNoString;
    ss >> templateNoString;
    if (templateNo < 0) {
        ss >> dec >> templateNo;
    }

    qDebug() << "magic string: " << QString::fromStdString(templateNoString) << "templateNo: " << templateNo ;
    if (templateNoString == magic)
        //    if (line.find(magic) != string::npos)
        qDebug() << "File is a template file" ;
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">The selected file in not a template file</font>");
        templateFile.close();
        return -1;
    }
    QString fingerName;
    getline(templateFile,line);
    fingerName = QString::fromStdString(line);

    qDebug() << "Fingername: " << fingerName ;
    /* check if the database position already contains a valid template */
    /* save the template to character file 1 */

    if ((retCode = readAndDownloadCharfile(templateFile,dataPacketSize,1)) != FP_SUCCESS)
        return retCode;
    /* now copy character file 1 to the template selected */
    qDebug() << "Saving to template position" << templateNo ;
    if ((retCode = protocol->fpStore(1,static_cast<uint8_t>(templateNo-1))) != FP_SUCCESS) {
        protocolError(retCode,"fpStore");
        return retCode;
    }

    qDebug() << "Writing to fpTable pos " << templateNo-1 ;
    QTableWidgetItem *tableItem = new QTableWidgetItem(QString(fingerName));
    ui_sfgQt->fpTable->setItem(templateNo-2,1,tableItem);
    return retCode;
}

int SfgQt::createUploadCharFileDialog() {
    qDebug() << "upload template Dialog" ;
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    QFileDialog* uploadCharFileDialog = new QFileDialog();
    uploadCharFileDialog->setOption(QFileDialog::DontUseNativeDialog, true); //we need qt layout
    if (QObject::sender() == ui_sfgQt->uploadCharfileButton)
        uploadCharFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    else {
        uploadCharFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    }
    qDebug() << "setting tenmplate dir: " << templateDir;
    uploadCharFileDialog->setDirectory(templateDir);
    QGridLayout *layout = static_cast<QGridLayout*>(uploadCharFileDialog->layout());
    QList< QPair<QLayoutItem*, QList<int> > > moved_items;
    for(int i = 0; i < layout->count(); i++) {
        int row, column, rowSpan, columnSpan;
        layout->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
        if (row >= 2) {
            QList<int> list;
            list << (row + 1) << column << rowSpan << columnSpan;
            moved_items << qMakePair(layout->takeAt(i), list);
            i--; // takeAt has shifted the rest items
        }
    }

    for(int i = 0; i < moved_items.count(); i++) {
        layout->addItem(moved_items[i].first,
                        moved_items[i].second[0],
                moved_items[i].second[1],
                moved_items[i].second[2],
                moved_items[i].second[3]);
    }

    QHBoxLayout *charFileSelectionlayout = new QHBoxLayout();
    QLabel* uploadCharFileLabel = new QLabel("Character File Buffer No: ");
    charFileSelectionlayout->addWidget(uploadCharFileLabel);

    uploadCharFileLabel->setAlignment(Qt::AlignLeft);
    uploadCharFileLabel->setAlignment(Qt::AlignVCenter);
    QSpinBox *charFileSelector = new QSpinBox();
    this->charFileSelector = charFileSelector;
    charFileSelector->setMinimum(1);
    charFileSelector->setMaximum(2);
    charFileSelector->setMaximumWidth(50);
    charFileSelectionlayout->addWidget(charFileSelector);
    charFileSelectionlayout->addSpacing(400);
    layout->addLayout(charFileSelectionlayout,2,0,1,4);

    uploadCharFileDialog->show();
    if (QObject::sender() == ui_sfgQt->uploadCharfileButton)
        connect(uploadCharFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(uploadCharFile(QString)));
    else {
        connect(uploadCharFileDialog,SIGNAL(fileSelected(QString)),this,SLOT(downloadCharFile(QString)));
    }
    return 1;
}

int SfgQt::uploadCharFile(QString filename) {
    int retCode = FP_SUCCESS;
    int charFileNo;
    /* if we don't get a filename, just exit */
    if (filename == nullptr)
        return retCode;
    charFileNo = charFileSelector->value();

    QFileDialog *uploadCharfileDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = uploadCharfileDialog->directory().absolutePath();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);
    ofstream charFile;
    charFile.open(filename.toStdString());
    /* magic header */
    charFile << "CharFile " << charFileNo << endl;
    if ((retCode = writeCharFile(charFile,charFileNo)) != FP_SUCCESS) {
        qDebug() << "error in writeCharFile" ;
        return retCode;
    }
    return retCode;
}
int SfgQt::downloadCharFile(QString filename) {
    int retCode = FP_SUCCESS;
    int charFileNo;
    string line;
    /* if we don't get a filename, just exit */
    if (filename == nullptr)
        return retCode;
    charFileNo = charFileSelector->value();
    QFileDialog *uploadCharfileDialog = static_cast<QFileDialog *>(QObject::sender());
    templateDir = uploadCharfileDialog->directory().dirName();
    ui_sfgQt->templateDirectoryLineEdit->setText(templateDir);

    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return 0;
    }
    ifstream charFile;
    charFile.open(filename.toStdString(),ifstream::in);
    if (charFile.is_open())
        qDebug() << "Character file successfully opened";
    else {
        qDebug() << "Could not open the characterfile";
    }
    string magic="CharFile";
    /* read magic header */
    getline(charFile,line);
    qDebug() << "magic line: " << QString::fromStdString(line) ;
    if (line.find(magic) != string::npos)
        qDebug() << "File is a character file" ;
    else {
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">The selected file in not a character file</font>");
        charFile.close();
        return -1;
    }
    if ((retCode = readAndDownloadCharfile(charFile,dataPacketSize,charFileNo)) == FP_SUCCESS) {
        QString msg = "<font color=\"blue\">File: ";
        msg.append(filename);
        msg.append(" was successfully downloaded to ");
        msg.append("character file ");
        msg.append(QString::number(charFileNo));
        msg.append("</font>");
        ui_sfgQt->statusTextEdit->setHtml(msg);
    }


    charFile.close();
    return retCode;
}

int SfgQt::readAndDownloadCharfile(istream& charFile, uint16_t dataPacketSize, int charFileNo){
    int retCode = FP_SUCCESS;
    string line;
    uint16_t address;
    uint8_t lineValues[16],*lineValuePtr;
    uint8_t values[FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS],*valuePtr;

    qDebug() << "readAndDownloadCharfile  to charfile " << charFileNo;

    if ((retCode = protocol->fpDownChar(static_cast<uint8_t>(charFileNo))) != FP_SUCCESS) {
        protocolError(retCode,"fpDownChar");
        return retCode;
    }
    /* read all 12 blocks */
    valuePtr = values;
    for (int i=0;i<FP_TEMPLATE_BLOCKS;i++) {
        for (int j=0;j<FP_DEFAULT_DATA_PKT_LENGTH/16;j++) {
            getline(charFile, line);
            if (line[0] == '\0') {
                --j;
                continue;
            }
            parse(line,&address,lineValues);
            lineValuePtr = lineValues;
            for (int k=0;k<16;k++)
                *valuePtr++ = *lineValuePtr++;
        }
    }
    qDebug() << "charFile successfully read in" ;

    valuePtr = values;

    for (int i=0;i<FP_DEFAULT_DATA_PKT_LENGTH*FP_TEMPLATE_BLOCKS/dataPacketSize-1;i++) {
        if((retCode = protocol->fpSendData(FP_DATA_PACKET,&dataPacketSize,valuePtr)) != FP_SUCCESS) {
            protocolError(retCode,"fpSendData");
            return retCode;
        }
        qDebug() << "Sending packet " << i ;
        valuePtr += dataPacketSize;
    }
    if((retCode = protocol->fpSendData(FP_END_PACKET,&dataPacketSize,valuePtr)) != FP_SUCCESS) {
        protocolError(retCode,"fpSendData");
        return retCode;
    }
    return retCode;

}


void SfgQt::parse(string line, uint16_t *address, uint8_t *values) {
    std::stringstream ss(line);
    stringstream value_ss;
    string addressString; string valueStrings[16];
    int tmp;
    /* split into string tokens with separator ' ' */
    ss >> addressString;
    for (int i=0;i<16;i++)
        ss >> valueStrings[i];
    /* remove 0x and ":" */
    addressString.erase(0,2); addressString.erase(5,1);
    value_ss = stringstream(addressString);
    value_ss >> hex >> *address;

    for (int i=0;i<16;i++) {
        /* remove "0x" */
        valueStrings[i].erase(0,2);
        value_ss = stringstream(valueStrings[i]);
        value_ss >> hex >> setw(2) >> tmp;
        values[i] = static_cast<uint8_t>(tmp);
    }
}

QList<int> SfgQt::getTemplateTable() {
    QList<int> fpTemplateList;
    uint8_t templateList[32];
    uint16_t noOfTemplates;
    int retCode;
    if (!serialIsOpen) {
        QMessageBox *notOpenMessage = new QMessageBox(this);
        notOpenMessage->setWindowTitle("Serial port not open");
        notOpenMessage->setText("Serial connection not open\nPlease open the serial line with the \'Open Device\' button ´first");
        notOpenMessage-> show();
        return fpTemplateList;
    }
    if ((retCode = protocol->fpTemplateNumber(&noOfTemplates)) == FP_SUCCESS) {
        qDebug() <<"Number of valid templates: " << noOfTemplates;
    }
    if (noOfTemplates == 0)
        return fpTemplateList;

    if ((retCode = protocol->fpReadConList(0,templateList)) != FP_SUCCESS) {
        protocolError(retCode,"fpReadConList");
        return fpTemplateList;
    }
    for (int i=0;i<32;i++) {
        uint8_t templ = templateList[i];
        uint8_t mask = 1;
        for (int j=0;j<8;j++) {
            if (8*i+j > this->getFpMaxTemplates())
                break;
            if (templ & mask) {
                fpTemplateList.append(8*i+j);
//                qDebug() << "template at pos " << 8*i+j ;
            }
            mask <<=1;
        }
    }

    return fpTemplateList;
}
int SfgQt::changeSecurityLevel(int index) {
    int retCode=FP_SUCCESS;
    qDebug() << "New security level" << securityLevel ;
    if ((retCode = protocol->fpSetSysPara(FP_SECURITY,static_cast<uint8_t>(index+1))) != FP_SUCCESS) {
        protocolError(retCode,"fpSetSysPara");
        return retCode;
    }
    securityLevel = static_cast<uint16_t>(index+1);
    qDebug() << "securityLevel: " << securityLevel ;
    printSystemInfo();
    return retCode;
}

int SfgQt::changeBaudrate(int index) {
    int retCode = FP_SUCCESS;
    qDebug() << "changeBaudrate" ;
    qDebug() << "Index: " << index ;
    qDebug() << "from combobox: " << ui_sfgQt->baudrateComboBox->currentIndex();
    switch (index) {
    case 0:
    case 1:
        baudrate=static_cast<uint16_t>(index+1);
        break;
    case 2:
        baudrate=4;
        break;
    case 4:
        baudrate=12;
        break;
    case 3:
    default:
        baudrate=6;
    }
    qDebug() << "New baudrate: " << baudrate*9600 ;
    printSystemInfo();

    if ((retCode = protocol->fpSetSysPara(FP_BAUDRATE,static_cast<uint8_t>(baudrate))) != FP_SUCCESS) {
        protocolError(retCode,"fpSetSysPara");
        return retCode;
    }

    /* close the serial line, which should now run on a different baud rate and re-open it with
     * the new baud rate */
    qDebug() << "Closing the serial connection" ;
    protocol->fpClose();
    protocol->handle = -1;
    qDebug() << "Re-open the serial connection with baud rate " << baudrate*9600 ;
    if ((retCode = protocol->fpInit(port, static_cast<uint8_t>(baudrate))) < 0) {
        /* settings file may be corrupted, gtry with default baudrate */
        if ((retCode = protocol->fpInit(port, static_cast<uint8_t>(FP_DEFAULT_BAUDRATE))) < 0) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not open serial serial. Is the fingerprint module connected?</font>");
            return ERR_SERIAL_PORT;
        }
    }

    /* check if the fingerprint module responds as epected */
    if ((retCode = protocol->fpHandshake()) != FP_SUCCESS) {
        /* maybe bad baudrate, try on default baudrate */
        qDebug() << "Fingerprint module does not respond on baudrate " << baudrate ;
        qDebug() << "Trying on default baudrate: " << FP_DEFAULT_BAUDRATE*9600 ;
        protocol->fpClose();
        if ((retCode = protocol->fpInit(port,static_cast<uint8_t>(FP_DEFAULT_BAUDRATE))) < 0) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Could not open serial serial. Is the fingerprint module connected?</font>");
            return ERR_SERIAL_PORT;
        }
        /* if it still does not work, return error */
        if ((retCode = protocol->fpHandshake()) != FP_SUCCESS)
            return ERR_SERIAL_PORT;
        ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Baudrate reset to default: 57600 baud</font>");
        return retCode;
    }
    QString msg=QString("<font color=\"blue\">Baudrate successfully set to ");
    msg.append(QString::number(baudrate*9600));
    msg.append(" baud</font>");
    ui_sfgQt->statusTextEdit->setHtml(msg);

    return retCode;
}

int SfgQt::changePacketSize(int index) {
    int retCode = FP_SUCCESS;
    if (index < 0)
        return retCode;
    if ((retCode = protocol->fpSetSysPara(FP_PACKET_SIZE,static_cast<uint8_t>(index))) != FP_SUCCESS) {
        protocolError(retCode,"fpSetSysPara");
        return retCode;
    }
    dataPacketSize= static_cast<uint16_t>(32<<index);
    printSystemInfo();
    qDebug() << "New packet size: " << dataPacketSize ;
    return retCode;
}

void SfgQt::printData(uint8_t *data, uint16_t size) {
    uint8_t *dataPtr = data;
    qDebug() << "Block of size" << size ;
    QString hexString = QString("");
    for (int i=0;i<static_cast<int>(size);i++) {
        if ((i!=0) && (!(i%16))) {
            qDebug().noquote() << hexString;
            hexString = QString("");
        }
        hexString.append(QString("0x%1 ").arg(*dataPtr++,2,16,QLatin1Char('0')));
    }
    qDebug().noquote() << hexString;
}
int SfgQt::findBaudrate() {
    int retCode = FP_SUCCESS;
    int baudrateTable[] = {9600,19200,38400,57600,115200};
    QSettings settings("UCC","sfgQt");
    if (serialIsOpen) {
        QString msg=QString("<font color=\"blue\">Succeeded to establish a connection <br>at baudrate ");
        msg.append(QString::number(baudrate*9600));
        msg.append(QString(" baud</font>"));
        ui_sfgQt->statusTextEdit->setHtml(msg);
        return retCode;
    }
    qDebug() << "findBaudrate" ;
    for (int i=0;i<5;i++) {
        ui_sfgQt->progressBar->setValue(20*(i+1));
        qApp->processEvents();
        if ((retCode = protocol->fpInit(port, static_cast<uint8_t>(baudrateTable[i]/9600))) != FP_SUCCESS) {
            ui_sfgQt->statusTextEdit->setHtml("<font color=\"red\">Cannot open serial port!<br>Is the reader connected?</font>");
            ui_sfgQt->progressBar->setValue(0);
            return retCode;
        }
        else
            qDebug() << "Serial connection successfully opened on baudrate " << QString::number(baudrateTable[i]) ;

        if ((retCode = protocol->fpHandshake()) == FP_SUCCESS) {
            qDebug() << "Succeeded to establish a connection at baudrate " << QString::number(baudrateTable[i]) ;
            QString msg=QString("<font color=\"blue\">Succeeded to establish a connection <br>at baudrate ");
            msg.append(QString::number(baudrateTable[i]));
            msg.append(QString(" baud</font>"));
            ui_sfgQt->statusTextEdit->setHtml(msg);
            baudrate = static_cast<uint16_t>(baudrateTable[i]/9600);
            ui_sfgQt->deviceNameLineEdit->setEnabled(false);
            break;
        }
        else
            protocol->fpClose();
    }

    qDebug() << "Save settings" ;
    qDebug() << "baudrate: " << baudrate ;
    qDebug() << "handle: " << protocol -> handle ;
    settings.setValue("baudrate",baudrate);

    ui_sfgQt->progressBar->setValue(0);
    return retCode;
}

void SfgQt::newSerialDevice() {
    QString newSerialPort = ui_sfgQt->deviceNameLineEdit->text();
    this->port = newSerialPort.toStdString();
    qDebug() << "new serial device" << newSerialPort;
}

void SfgQt::test() {


}
