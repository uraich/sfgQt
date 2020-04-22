#include "enrolldialog.h"
#include <qdebug.h>

EnrollDialog::EnrollDialog(QWidget *parent)
    : QDialog(parent), ui_Enroll(new Ui::EnrollDialog)

{
    ui_Enroll->setupUi(this);
    if (ui_Enroll == nullptr)
        qDebug("nullptr");
    qDebug("Setting up enroll dialog");
}

QString EnrollDialog::getName() {
    return this->ui_Enroll->nameText->text();
}

int EnrollDialog::getTemplateNo() {
    return this->ui_Enroll->templateNoSpinBox->value();
}
void EnrollDialog::setTemplateNo(int templateNo) {
    ui_Enroll->templateNoSpinBox->setValue(templateNo);
}
