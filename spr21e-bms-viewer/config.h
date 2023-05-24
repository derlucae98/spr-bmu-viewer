#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QStandardItem>

namespace Ui {
class Config;
}

class Config : public QDialog
{
    Q_OBJECT

public:
    explicit Config(QWidget *parent = nullptr);
    ~Config();

private slots:
    void on_btnLoad_clicked();

private:
    Ui::Config *ui;

};

#endif // CONFIG_H
