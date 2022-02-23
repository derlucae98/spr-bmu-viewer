#include "diagdialog.h"
#include "ui_diagdialog.h"

DiagDialog::DiagDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DiagDialog)
{
    ui->setupUi(this);
    this->setFixedSize(this->width(), this->height());
    this->setWindowTitle("Diagnostic");
    QPushButton *cancelButton = ui->buttonBox->button(QDialogButtonBox::Close);
    QObject::connect(cancelButton, &QPushButton::clicked, this, &DiagDialog::close_clicked);
}

DiagDialog::~DiagDialog()
{
    delete ui;
}

void DiagDialog::on_checkBox_stateChanged(int arg1)
{
    emit balancing_enable(static_cast<bool>(arg1));
}


void DiagDialog::close_clicked()
{
    this->hide();
}

