#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QWidget>
#include <ts_accu.h>
#include <QDebug>

namespace Ui {
class ErrorDialog;
}

class ErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ErrorDialog(TS_Accu::contactor_error_t error, QDialog *parent = nullptr);
    ~ErrorDialog();
    void updateErrors(TS_Accu::contactor_error_t error);

private:
    Ui::ErrorDialog *ui;
    void update_labels(TS_Accu::contactor_error_t error);
};

#endif // ERRORDIALOG_H
