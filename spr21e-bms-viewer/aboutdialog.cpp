#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->qtVersion->setText(QString("Qt %1.%2.%3").arg(QT_VERSION_MAJOR).arg(QT_VERSION_MINOR).arg(QT_VERSION_PATCH));
    ui->version->setText(QString("%1.%2.%3").arg(VERS_MAJOR).arg(VERS_MINOR).arg(VERS_BUILD));
    ui->author->setText(AUTHORS);
    QPixmap scuderiaLogo(":/img/logo_text.png");
    ui->logo->setScaledContents(true);
    ui->logo->setPixmap(scuderiaLogo);
    this->setFixedSize(this->size());
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
