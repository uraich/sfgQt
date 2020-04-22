#ifndef TEMPLATEVERIFYDIALOG_H
#define TEMPLATEVERIFYDIALOG_H

#include <QDialog>

namespace Ui {
class VerifyTemplateDialog;
}

class VerifyTemplateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VerifyTemplateDialog(QWidget *parent = nullptr);
    ~VerifyTemplateDialog();
    int getTemplateNo();

private:
    Ui::VerifyTemplateDialog *ui_Verify;

};

#endif // TEMPLATEVERIFYDIALOG_H
