#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());


}

Config::~Config()
{
    delete ui;
}

void Config::on_btnLoad_clicked()
{

}

