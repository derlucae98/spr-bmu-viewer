#include "errordialog.h"
#include "ui_errordialog.h"

ErrorDialog::ErrorDialog(TS_Accu::contactor_error_t error, QDialog *parent) :
    QDialog(parent),
    ui(new Ui::ErrorDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setWindowFlags(Qt::Dialog);

    QList<QLabel *> labels = this->findChildren<QLabel *>();

    for (auto &i : labels) {
        i->setStyleSheet("color: rgb(215, 214, 213)");
    }

    update_labels(error);
}

ErrorDialog::~ErrorDialog()
{
    delete ui;
}

void ErrorDialog::updateErrors(TS_Accu::contactor_error_t error)
{
    update_labels(error);
}

void ErrorDialog::update_labels(TS_Accu::contactor_error_t error)
{
    QList<QLabel *> labels = this->findChildren<QLabel *>();
    for (int i = 0; i < labels.size(); i++) {
        if (error & (1 << i)) {
            labels[i]->setStyleSheet("color: red; font-weight: bold");
        } else {
            labels[i]->setStyleSheet("color: rgb(215, 214, 213); font-weight: normal");
        }
    }
}
