#include "notepaddialog.h"
#include "ui_notepaddialog.h"

NotePadDialog::NotePadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NotePadDialog)
{
    ui->setupUi(this);
}

NotePadDialog::~NotePadDialog()
{
    delete ui;
}
void NotePadDialog::setHexText(QString text) {
    ui->hexNotepadTextEdit->setPlainText(text);
    return;
}

void NotePadDialog::setAsciiText(QString text) {
    ui->asciiNotepadTextEdit->setPlainText(text);
    return;
}
