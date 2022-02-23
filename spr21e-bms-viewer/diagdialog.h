#ifndef DIAGDIALOG_H
#define DIAGDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QPushButton>

namespace Ui {
class DiagDialog;
}

class DiagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiagDialog(QWidget *parent = nullptr);
    ~DiagDialog();

private slots:
    void on_checkBox_stateChanged(int arg1);


private:
    Ui::DiagDialog *ui;
    void close_clicked();

signals:
    void balancing_enable(bool);
};

#endif // DIAGDIALOG_H
