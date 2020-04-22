#ifndef ENROLLDIALOG_H
#define ENROLLDIALOG_H
#include "ui_enrollDialog.h"
#include <QDialog>

namespace Ui
{
    class EnrollDialog;
}

class EnrollDialog : public QDialog {
    Q_OBJECT

public:
    EnrollDialog(QWidget *parent = nullptr);
    QString getName();
    int getTemplateNo();
    void setTemplateNo(int);

private:
    Ui::EnrollDialog *ui_Enroll;
    QString fingerName="";

};

#endif // ENROLLDIALOG_H
