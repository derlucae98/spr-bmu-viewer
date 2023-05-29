#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());

    pollTimer = new QTimer();
    pollTimer->setInterval(1000);
    QObject::connect(pollTimer, &QTimer::timeout, this, &Config::poll_timer_callback);
    ui->downloadProgress->setVisible(false);
    ui->status->setText("");


}

Config::~Config()
{
    delete ui;
}

void Config::can_recv(QCanBusFrame frame)
{
    if (frame.frameId() != CAN_ID_CAL_RESPONSE) {
        return;
    }
    switch (frame.payload().at(0) & 0x7F) {
    case ID_LOAD_DEFAULT_CONFIG:
        break;
    case ID_GLOBAL_BALANCING_ENABLE:
        break;
    case ID_BALANCING_THRESHOLD:
        break;
    case ID_BALANCING_FEEDBACK:
        break;
    case ID_SOC_LOOKUP:
        break;
    case ID_AUTOMATIC_SOC_LOOKUP_ENABLE:
        break;
    case ID_NUMBER_OF_STACKS:
        break;
    case ID_LOGGER_ENABLE:
        break;
    case ID_LOGGER_DELETE_OLDEST_FILE:
        break;
    case ID_AUTORESET_ENABLE:
        break;
    case ID_SET_GET_RTC:
        break;
    case ID_CONTROL_CALIBRATION:
        break;
    case ID_CALIBRATION_STATE:
        break;
    case ID_CALIBRATION_VALUE:
        break;
    case ID_FORMAT_SD_CARD:
    case ID_FORMAT_SD_CARD_STATUS:
        handle_format_sd_response(frame);
        break;
    }
}

void Config::on_btnLoad_clicked()
{

}


void Config::on_btnFormatSD_clicked()
{
    QMessageBox::StandardButton ret = QMessageBox::question(this, "Format SD card",
                                                            "All data will be lost!\n"
                                                            "This process can take several minutes and cannot be cancelled. Proceed?",
                                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QByteArray payload;
        QCanBusFrame req;
        req.setFrameId(CAN_ID_CAL_REQUEST);
        quint8 cmd = ID_FORMAT_SD_CARD;
        cmd |= 0x80;
        payload.append(cmd);
        payload.append((quint8)0);
        req.setPayload(payload);
        emit can_send(req);
        pollTimer->start();
    }
}

void Config::handle_format_sd_response(QCanBusFrame &frame)
{
    switch (frame.payload().at(2)) {
    case CARD_FORMATTING_FINISHED:
        pollTimer->stop();
        ui->status->setText("Done!");
        ui->tabWidget->setEnabled(true);
        break;
    case CARD_FORMATTING_BUSY:
        ui->status->setText("Formatting SD card...");
        ui->tabWidget->setEnabled(false);
        break;
    case ERROR_CARD_FORMATTING_FAILED:
        pollTimer->stop();
        ui->status->setText("Formatting failed!");
        ui->tabWidget->setEnabled(true);
        break;
    case ERROR_NO_CARD:
        pollTimer->stop();
        ui->status->setText("No card!");
        ui->tabWidget->setEnabled(true);
        break;
    }
}

void Config::poll_timer_callback()
{
    QByteArray payload;
    QCanBusFrame req;
    req.setFrameId(CAN_ID_CAL_REQUEST);
    quint8 cmd = ID_FORMAT_SD_CARD_STATUS;
    payload.append(cmd);
    payload.append((quint8)0);
    req.setPayload(payload);
    emit can_send(req);
}


void Config::on_btnDownload_clicked()
{

}

