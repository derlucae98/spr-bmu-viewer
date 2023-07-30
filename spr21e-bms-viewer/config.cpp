#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setEnabled(false);

    pollTimer = new QTimer();
    pollTimer->setInterval(1000);
    QObject::connect(pollTimer, &QTimer::timeout, this, &Config::poll_timer_callback);
    ui->downloadProgress->setVisible(false);
    ui->status->setText("");
    ::memset(&config, 0, sizeof(config_t));
    ::memset(&oldConfig, 0, sizeof(config_t));

    isotp = new Isotp(this);
    QObject::connect(isotp, &Isotp::send_can, this, &Config::can_send);
    QObject::connect(this, &Config::isotp_new_frame, isotp, &Isotp::on_can_message);
    QObject::connect(isotp, &Isotp::new_message, this, &Config::on_new_isotp_message);

    isotp->init_link(ISOTP_UPLINK, SEND_BUF_SIZE, RECV_BUF_SIZE, 5);

}

Config::~Config()
{
    delete ui;
}

void Config::can_recv(QCanBusFrame frame)
{
    if (frame.frameId() == ISOTP_DOWNLINK) {
        emit isotp_new_frame(frame);
    }
}

void Config::on_btnLoad_clicked()
{
    query_config();
}


void Config::on_btnFormatSD_clicked()
{
    if (config.loggerEnable) {
        QMessageBox::StandardButton ret = QMessageBox::question(this, "Format SD card",
                                                                "Logger must be stopped in order to format SD card.\n"
                                                                "Would you like to stop the logger now?",
                                                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        config.loggerEnable = false;
        if (ret == QMessageBox::Yes) {
            update_config();
        } else {
            return;
        }
    }
    QMessageBox::StandardButton ret = QMessageBox::question(this, "Format SD card",
                                                            "All data will be lost!\n"
                                                            "This process can take several minutes and cannot be cancelled. Proceed?",
                                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        QByteArray payload;
        payload.append(char(ISOTP_CAL_REQUEST));
        payload.append(quint8(ID_FORMAT_SD_CARD | 0x80));
        payload.append(char(0));
        isotp->send(payload);
        pollTimer->start();
    }
}

void Config::handle_format_sd_response(QByteArray &frame)
{
    switch (frame.at(3)) {
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
    payload.append(char(ISOTP_CAL_REQUEST));
    payload.append(quint8(ID_FORMAT_SD_CARD_STATUS));
    payload.append(char(0));
    isotp->send(payload);
}

void Config::query_config()
{
    ui->status->setText("Requesting config...");
    QByteArray payload;
    payload.append(char(ISOTP_CAL_REQUEST));
    payload.append(quint8(ID_QUERY_CONFIG));
    payload.append(char(0));
    isotp->send(payload);
}

void Config::update_config()
{
    ui->status->setText("Updating config...");
    quint16 crc = calc_crc16((quint8*)&this->config, sizeof(config_t) - sizeof(quint16));
    this->config.crc16 = crc;
    QByteArray payload = QByteArray::fromRawData((const char*)&this->config, sizeof(config_t));
    QByteArray send;
    send.append(char(ISOTP_CAL_REQUEST));
    send.append(quint8(ID_UPDATE_CONFIG | 0x80));
    send.append(sizeof(config_t));
    send.append(payload);
    isotp->send(send);
}

void Config::system_reset()
{
    QByteArray send;
    send.append(char(ISOTP_CAL_REQUEST));
    send.append(quint8(ID_RESTART_SYSTEM | 0x80));
    send.append(char(0));
    isotp->send(send);
}

void Config::handle_query_config_response(QByteArray &data)
{
    if (data.at(1) == (ID_QUERY_CONFIG & 0x80)) {
        QMessageBox::critical(this, "Error requesting config!", "Could not query config!");

        return;
    }

    if ((data.size() != (sizeof(config_t) + 3)) || data.at(2) != sizeof(config_t)) {
        QMessageBox::critical(this, "Error requesting config!", "Could not query config due to transmission error!");
        this->close();
        return;
    }

    //Copy raw data into local config variable
    config_t config;
    ::memcpy(&config, data.data_ptr()->data() + 3, sizeof(config_t));

    quint16 crcIs = calc_crc16((quint8*)&config, sizeof(config_t) - sizeof(quint16));
    if (crcIs != config.crc16) {
        QMessageBox::critical(this, "Error requesting config!", "Could not query config due to CRC error!");
        return;
    }

    this->config = config;
    update_UI_config();
    ui->status->setText("Ready.");
}

void Config::handle_update_config_response(QByteArray &data)
{
    if (data.at(1) == ID_UPDATE_CONFIG) {
        ui->status->setText("Ready.");
    } else {
        QMessageBox::critical(this, "Error updating config!", QString("Could not update config! Error code: %1").arg(data.at(3)));
        ui->status->setText("Update failed!");
        return;
    }
}

void Config::handle_cal_response(QByteArray data)
{
    switch (data.at(1) & 0x7F) {
        case ID_LOAD_DEFAULT_CONFIG:
            break;
        case ID_QUERY_CONFIG:
            handle_query_config_response(data);
            break;
        case ID_UPDATE_CONFIG:
            handle_update_config_response(data);
            break;
        case ID_BALANCING_FEEDBACK:
            break;
        case ID_SOC_LOOKUP:
            break;
        case ID_SET_RTC:
            break;
        case ID_CONTROL_CALIBRATION:
            break;
        case ID_CALIBRATION_STATE:
            break;
        case ID_CALIBRATION_VALUE:
            break;
        case ID_FORMAT_SD_CARD:
        case ID_FORMAT_SD_CARD_STATUS:
            handle_format_sd_response(data);
            break;
        case ID_QUERY_LOGFILE_INFO:
            handle_logfile_info_response(data);
        break;
    }
}

void Config::handle_logfile_info_response(QByteArray data)
{
    if ((data.size() - 3) % sizeof(file_info_t) != 0) {
        qDebug() << "Error fetching logfile data!";
        return;
    }
    ui->logList->clear();

    quint8 numberOfLogfiles = (data.size() - 3) / sizeof(file_info_t);
    file_info_t files[numberOfLogfiles];
    char *rawData = data.data_ptr()->data() + 3;

    for (quint8 i = 0; i < numberOfLogfiles; i++) {
        ::memcpy(&files[i], rawData + (i * sizeof(file_info_t)), sizeof(file_info_t));
    }



    for (quint8 i = 0; i < numberOfLogfiles; i++) {
        // Calculate length of files
        quint32 lenghtSeconds = files[i].size / (10 * sizeof(logging_data_t));
        ui->logList->addItem(QString(files[i].name) + QString("\t %1").arg(QDateTime::fromTime_t(lenghtSeconds).toUTC().toString("hh:mm:ss")) + "\t (" + QString::number(((files[i].size - 1) / 1000) + 1) + " kB)");
    }
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
    if (config.loggerEnable) {
        ui->lLoggerEnabled->setText("Logger enabled");
        ui->lLoggerEnabled->setStyleSheet("color:green");
    } else {
        ui->lLoggerEnabled->setText("Logger disabled");
        ui->lLoggerEnabled->setStyleSheet("color:red");
    }

    //Clear all stylesheets after querying config
    ui->cbBalancingEnabled->setStyleSheet("");
    ui->balancingThreshold->setPalette(this->style()->standardPalette());
    ui->cbAutoSocLookup->setStyleSheet("");
    ui->numberOfStacks->setPalette(this->style()->standardPalette());
    ui->cbLoggerEnabled->setStyleSheet("");
    ui->cbDeleteOldestLog->setStyleSheet("");
    ui->cbSdcAutoReset->setStyleSheet("");

    ui->status->setText("Ready.");
    this->setEnabled(true);
}


void Config::showEvent(QShowEvent *event)
{
    query_config();
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

void Config::on_new_isotp_message(QByteArray message)
{
    switch (message.at(0)) {
    case ISOTP_CAL_RESPONSE:
        handle_cal_response(message);
        break;
    case ISOTP_LOGFILE:
        break;
    }
}

quint16 Config::calc_crc16(quint8 *data, size_t len)
{
    const uint16_t CRC16_TABLE[] = {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
        0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
        0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
        0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
        0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
        0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
        0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
        0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
        0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
        0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
        0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
        0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
        0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
        0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
        0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
        0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
        0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
        0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
        0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
        0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
        0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
        0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
        0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    };
    int count = len >> 2;
    quint32 *p = (quint32 *) data;
    quint32 reg;
    quint16 crc = 0; // start value
    while(count-- > 0) {
        reg = *p++;
        crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ reg) & 0xFF];
        crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ (reg >> 8)) & 0xFF];
        crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ (reg >> 16)) & 0xFF];
        crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ (reg >> 24)) & 0xFF];
    }
    count = len % 4;
    data = (quint8 *)p;
    while(count-- > 0) {
        crc = (crc << 8) ^ CRC16_TABLE[((crc >> 8) ^ *data++) & 0xFF];
    }
    return crc;
}


void Config::on_btnWrite_clicked()
{
    update_config();
}


void Config::on_btnLoggerFetch_clicked()
{
    QByteArray payload;
    payload.append(char(ISOTP_CAL_REQUEST));
    payload.append(quint8(ID_QUERY_LOGFILE_INFO));
    payload.append(char(0));
    isotp->send(payload);
}

