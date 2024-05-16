#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    interfaceUp = false;

    ui->cbSelectDevice->addItems(Can::get_available_devices());

    init_ts();

    init_lv();

    QPixmap scuderiaLogo(":/img/logo.png");

    ui->scuderiaLogo->setScaledContents(true);
    ui->scuderiaLogo->setPixmap(scuderiaLogo.scaled(2*38, 2*22, Qt::KeepAspectRatio));

    QColor bgColor = ui->tsParameters->palette().color(QWidget::backgroundRole());
    if (bgColor.lightness() < 127) {
        darkMode = true;
    } else {
        darkMode = false;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::show_error_message()
{
    // Show Messagebox on button click (or automatically if checkbox is checked)
    // Close Messagebox if error cleares
    errorDialog = new ErrorDialog(tsBatteryData.errorCode);

    auto connectionStateChanged = QObject::connect(tsAccu, &TS_Accu::ts_state_changed, this, [=](TS_Accu::ts_state_t state){
        if (state != TS_Accu::TS_STATE_ERROR) {
            if (errorDialog) {
                errorDialog->close();
            }
        }
    });

    auto connectionNewData = QObject::connect(tsAccu, &TS_Accu::new_data, this, [=](TS_Accu::ts_battery_data_t data) {
        if (errorDialog) {
            errorDialog->updateErrors(data.errorCode);
        }
    });

    errorDialog->setAttribute(Qt::WA_DeleteOnClose);
    errorDialog->exec();
    QObject::disconnect(connectionStateChanged);
    QObject::disconnect(connectionNewData);

}

void MainWindow::append_error(QString error, severity_t severity)
{
    QString severityString;
    switch (severity) {
    case SEVERITY_INFO:
        severityString = "INFO";
        ui->errorLog->setTextColor(Qt::black);
        break;
    case SEVERITY_INFO_GREEN:
        severityString = "INFO";
        ui->errorLog->setTextColor(Qt::darkGreen);
        break;
    case SEVERITY_WARNING:
        severityString = "WARNING";
        ui->errorLog->setTextColor(Qt::darkYellow);
        break;
    case SEVERITY_ERROR:
        severityString = "ERROR";
        ui->errorLog->setTextColor(Qt::red);
        break;
    }

    QDateTime dateTime = QDateTime::currentDateTime();
    ui->errorLog->append("[" + dateTime.date().toString("dd.MM.yyyy") + " " + dateTime.time().toString("hh:mm:ss") + QString("] [%1]: ").arg(severityString) + error);
}

void MainWindow::connect_gateway()
{
    gateway = new Gateway(this);
    QObject::connect(gateway, &Gateway::ch1_new_frame, tsAccu, &TS_Accu::can_frame);

    QObject::connect(gateway, &Gateway::ch1_state_changed, this, [=](QAbstractSocket::SocketState state) {
        qDebug() << "Gateway channel 1 state: " << state;
    });

    QUrl ch1;
    ch1.setHost("192.168.4.101");
    ch1.setPort(8881);
    QUrl ch2;
    ch2.setHost("192.168.4.101");
    ch2.setPort(8882);

    gateway->connect_device(ch1, ch2);
}

void MainWindow::disconnect_gateway()
{
    if (gateway) {
        gateway->disconnect_device();
        gateway->deleteLater();
    }
}

void MainWindow::on_btnConnectDevice_clicked()
{
    if (ui->cbSelectDevice->currentIndex() == 0) {
        //First Element is WCAN-01 gateway
        if (!interfaceUp) {
            connect_gateway();
        } else {
            disconnect_gateway();
        }
    } else {
        if (!interfaceUp) {
            connect_can_dev();
        } else {
            disconnect_can_dev();
        }
    }
}

void MainWindow::on_clearErrorLog_clicked()
{
    ui->errorLog->clear();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    (void) event;
    if (can) {
        can->disconnect_device();
    }
}

void MainWindow::connect_can_dev()
{
    can = new Can(this);

    can->set_device_name(ui->cbSelectDevice->currentText());
    can->connect_device();

    QObject::connect(can, &Can::error, this, [=](QString err) {
        QMessageBox mb;
        mb.setText(err);
        mb.exec();
    });
    QObject::connect(can, &Can::device_up, this, [=] {
        interfaceUp = true;
        ui->tsInfoFrame->setEnabled(true);
        ui->tsParameters->setEnabled(true);
        ui->btnConnectDevice->setText("Disconnect");
        ui->cbSelectDevice->setEnabled(false);
    });
    QObject::connect(can, &Can::device_down, this, [=] {
        interfaceUp = false;
        ui->tsInfoFrame->setEnabled(false);
        ui->tsParameters->setEnabled(false);
        ui->btnConnectDevice->setText("Connect");
        ui->cbSelectDevice->setEnabled(true);
        can->deleteLater();
    });

    QObject::connect(can, &Can::new_frame, tsAccu, &TS_Accu::can_frame);
    QObject::connect(can, &Can::new_frame, lvAccu, &LV_Accu::can_frame);
    QObject::connect(tsAccu, &TS_Accu::can_send, can, &Can::send_frame);
}

void MainWindow::disconnect_can_dev()
{
    if (can) {
        can->disconnect_device();
    }
}
