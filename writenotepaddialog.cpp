#include "writenotepaddialog.h"
#include "ui_writenotepaddialog.h"

WriteNotepadDialog::WriteNotepadDialog(QWidget *parent) :
    QDialog(parent),
    ui_writeNotepadDialog(new Ui::WriteNotepadDialog)
{
    ui_writeNotepadDialog->setupUi(this);
}

WriteNotepadDialog::~WriteNotepadDialog()
{
    delete ui_writeNotepadDialog;
}
QString WriteNotepadDialog::getNotepadText() {
    return ui_writeNotepadDialog->notepadText->text().left(32);
}

uint8_t WriteNotepadDialog::getPageNo() {
    int pageNo = ui_writeNotepadDialog->notepadPageNo->value();
    return static_cast<uint8_t>(pageNo);
}
