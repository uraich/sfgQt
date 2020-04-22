#include "savefp_imagedialog.h"
#include "ui_savefp_imagedialog.h"

SaveFP_ImageDialog::SaveFP_ImageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveFP_ImageDialog)
{
    ui->setupUi(this);
}

SaveFP_ImageDialog::~SaveFP_ImageDialog()
{
    delete ui;
}
