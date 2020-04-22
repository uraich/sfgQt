#ifndef NOTEPADDIALOG_H
#define NOTEPADDIALOG_H

#include <QDialog>

namespace Ui {
class NotePadDialog;
}

class NotePadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotePadDialog(QWidget *parent = nullptr);
    ~NotePadDialog();
    void setHexText(QString);
    void setAsciiText(QString);
private:
    Ui::NotePadDialog *ui;

};

#endif // NOTEPADDIALOG_H
