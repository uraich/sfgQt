#include "verifyTemplateDialog.h"
#include "ui_verifyTemplateDialog.h"

VerifyTemplateDialog::VerifyTemplateDialog(QWidget *parent) :
    QDialog(parent),
    ui_Verify(new Ui::VerifyTemplateDialog)
{
    ui_Verify->setupUi(this);
}

VerifyTemplateDialog::~VerifyTemplateDialog()
{
    delete ui_Verify;
}

int VerifyTemplateDialog::getTemplateNo() {
    return ui_Verify->verifyTemplateSpinBox->value();
}
