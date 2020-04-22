#ifndef WRITENOTEPADDIALOG_H
#define WRITENOTEPADDIALOG_H

#include <QDialog>

namespace Ui {
class WriteNotepadDialog;
}

class WriteNotepadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WriteNotepadDialog(QWidget *parent = nullptr);
    QString getNotepadText();
    uint8_t getPageNo();
    ~WriteNotepadDialog();

private:
    Ui::WriteNotepadDialog *ui_writeNotepadDialog;
};

#endif // WRITENOTEPADDIALOG_H
