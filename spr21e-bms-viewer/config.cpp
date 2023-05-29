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
    currentCmd = ID_GLOBAL_BALANCING_ENABLE;
    ::memset(&config, 0, sizeof(config_t));
    ::memset(&oldConfig, 0, sizeof(config_t));

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
        handle_balancing_enable_response(frame);
        break;
    case ID_BALANCING_THRESHOLD:
        handle_balancing_threshold_response(frame);
        break;
    case ID_BALANCING_FEEDBACK:
        break;
    case ID_SOC_LOOKUP:
        break;
    case ID_AUTOMATIC_SOC_LOOKUP_ENABLE:
        handle_automatic_soc_lookup_response(frame);
        break;
    case ID_NUMBER_OF_STACKS:
        handle_number_of_stacks_response(frame);
        break;
    case ID_LOGGER_ENABLE:
        handle_logger_enable_response(frame);
        break;
    case ID_LOGGER_DELETE_OLDEST_FILE:
        handle_logger_delete_oldest_response(frame);
        break;
    case ID_AUTORESET_ENABLE:
        handle_auto_reset_response(frame);
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
    load_config();
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

void Config::load_config()
{
    ui->status->setText("Requesting config...");
    request_cmd(currentCmd);
}

void Config::request_cmd(quint8 ID)
{
    QByteArray payload;
    QCanBusFrame req;
    req.setFrameId(CAN_ID_CAL_REQUEST);
    quint8 cmd = ID;
    payload.append(cmd);
    payload.append((quint8)0);
    req.setPayload(payload);
    emit can_send(req);
}

void Config::update_UI_config()
{
    oldConfig = config;
    ui->cbBalancingEnabled->setChecked(config.globalBalancingEnable);
    ui->cbAutoSocLookup->setChecked(config.automaticSocLookupEnable);
    ui->cbLoggerEnabled->setChecked((bool)config.loggerEnable);
    ui->cbDeleteOldestLog->setChecked(config.loggerDeleteOldestFile);
    ui->cbSdcAutoReset->setChecked(config.autoResetOnPowerCycleEnable);
    ui->numberOfStacks->setValue(config.numberOfStacks);
    ui->balancingThreshold->setValue((float)config.balancingThreshold / 10000.0f);
    ui->status->setText("Ready.");
}

void Config::handle_balancing_enable_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.globalBalancingEnable = frame.payload().at(2);
        currentCmd = ID_BALANCING_THRESHOLD;
        load_config();
    }
}

void Config::handle_balancing_threshold_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.balancingThreshold = (quint16)(frame.payload().at(3) << 8) | (quint8)(frame.payload().at(2));
        currentCmd = ID_AUTOMATIC_SOC_LOOKUP_ENABLE;
        load_config();
    }
}

void Config::handle_automatic_soc_lookup_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.automaticSocLookupEnable = frame.payload().at(2);
        currentCmd = ID_NUMBER_OF_STACKS;
        load_config();
    }
}

void Config::handle_number_of_stacks_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.numberOfStacks = frame.payload().at(2);
        currentCmd = ID_LOGGER_ENABLE;
        load_config();
    }
}

void Config::handle_logger_enable_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.loggerEnable = frame.payload().at(2);
        currentCmd = ID_LOGGER_DELETE_OLDEST_FILE;
        load_config();
    }
}

void Config::handle_logger_delete_oldest_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.loggerDeleteOldestFile = frame.payload().at(2);
        currentCmd = ID_AUTORESET_ENABLE;
        load_config();
    }
}

void Config::handle_auto_reset_response(QCanBusFrame &frame)
{
    if (response_error(frame)) {
        qDebug() << "Error occurred!";
        return;
    }

    if (response_to_set_request(frame)) {

    } else {
        config.autoResetOnPowerCycleEnable = frame.payload().at(2);
        currentCmd = ID_GLOBAL_BALANCING_ENABLE;
        update_UI_config();
    }
}

bool Config::response_error(QCanBusFrame &frame)
{
    if (frame.payload().at(0) & 0x80) {
        // Error occurred
        return true;
    }
    return false;
}

bool Config::response_to_set_request(QCanBusFrame &frame)
{
    if (frame.payload().at(1) == 0) {
        // Response to a set request
        return true;
    } else {
        // Response to a get request
        return false;
    }
}

void Config::showEvent(QShowEvent *event)
{
    load_config();
}



void Config::on_btnDownload_clicked()
{

}


void Config::on_cbBalancingEnabled_stateChanged(int arg1)
{
    config.globalBalancingEnable = (bool)arg1;
    if (config.globalBalancingEnable != oldConfig.globalBalancingEnable) {
        ui->cbBalancingEnabled->setStyleSheet("color:red");
    } else {
        ui->cbBalancingEnabled->setStyleSheet("");
    }
}


void Config::on_balancingThreshold_valueChanged(double arg1)
{
    config.balancingThreshold = (quint16)((arg1 + 0.00005) * 10000);
    if (config.balancingThreshold != oldConfig.balancingThreshold) {
        QPalette palette = ui->balancingThreshold->palette();
        palette.setColor(QPalette::Text, QColor(0xFF, 0, 0));
        ui->balancingThreshold->setPalette(palette);
    } else {
        ui->balancingThreshold->setPalette(this->style()->standardPalette());
    }
}


void Config::on_cbAutoSocLookup_stateChanged(int arg1)
{
    config.automaticSocLookupEnable = (bool)arg1;
    if (config.automaticSocLookupEnable != oldConfig.automaticSocLookupEnable) {
        ui->cbAutoSocLookup->setStyleSheet("color:red");
    } else {
        ui->cbAutoSocLookup->setStyleSheet("");
    }
}


void Config::on_numberOfStacks_valueChanged(int arg1)
{
    config.numberOfStacks = (quint8)(arg1);
    if (config.numberOfStacks != oldConfig.numberOfStacks) {
        QPalette palette = ui->numberOfStacks->palette();
        palette.setColor(QPalette::Text, QColor(0xFF, 0, 0));
        ui->numberOfStacks->setPalette(palette);
    } else {
        ui->numberOfStacks->setPalette(this->style()->standardPalette());
    }
}


void Config::on_cbLoggerEnabled_stateChanged(int arg1)
{
    config.loggerEnable = (bool)arg1;
    if (config.loggerEnable != oldConfig.loggerEnable) {
        ui->cbLoggerEnabled->setStyleSheet("color:red");
    } else {
        ui->cbLoggerEnabled->setStyleSheet("");
    }
}


void Config::on_cbDeleteOldestLog_stateChanged(int arg1)
{
    config.loggerDeleteOldestFile = (bool)arg1;
    if (config.loggerDeleteOldestFile != oldConfig.loggerDeleteOldestFile) {
        ui->cbDeleteOldestLog->setStyleSheet("color:red");
    } else {
        ui->cbDeleteOldestLog->setStyleSheet("");
    }
}


void Config::on_cbSdcAutoReset_stateChanged(int arg1)
{
    config.autoResetOnPowerCycleEnable = (bool)arg1;
    if (config.autoResetOnPowerCycleEnable != oldConfig.autoResetOnPowerCycleEnable) {
        ui->cbSdcAutoReset->setStyleSheet("color:red");
    } else {
        ui->cbSdcAutoReset->setStyleSheet("");
    }
}

