#include "config.h"
#include "ui_config.h"

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());
    this->setEnabled(false);

    ui->status->setText("");
    ::memset(&config, 0, sizeof(config_t));
    ::memset(&oldConfig, 0, sizeof(config_t));

    calStateTimer = new QTimer(this);
    calStateTimer->setInterval(100);
    QObject::connect(calStateTimer, &QTimer::timeout, this, &Config::poll_cal_state);
    calState = CAL_STATE_STANDBY;
    ui->calStatus->setText("");

    config_ui_disable();

}

Config::~Config()
{
    delete ui;
}

void Config::can_recv(QCanBusFrame frame)
{
    if (frame.frameId() == CAN_ID_CAL_RESPONSE) {
        handle_cal_response(frame.payload());
    }
    if (frame.frameId() == CAN_ID_STARTUP) {
        query_config();
    }
    if (frame.frameId() == CAN_ID_TIME) {
        update_time(frame.payload());
    }
}

void Config::on_btnLoad_clicked()
{
    query_config();
}

void Config::query_config()
{
    ui->status->setText("Requesting config...");
    QByteArray payload;
    payload.append(char(ID_QUERY_CONFIG));
    send_frame(payload);
}

void Config::update_config()
{
    ui->status->setText("Updating config...");
    QByteArray payload;

    payload.append(ID_UPDATE_CONFIG);
    payload.append(char(0)); //Page
    payload.append(config.balancingThreshold & 0xFF);
    payload.append(config.balancingThreshold >> 8);
    payload.append(((config.globalBalancingEnable & 0x01) << 6)
                 | ((config.automaticSocLookupEnable & 0x01) << 5)
                 | ((config.autoResetOnPowerCycleEnable & 0x01) << 4)
                 | (config.numberOfStacks & 0x0F));
    payload.append(char(0));
    payload.append(char(0));
    payload.append(char(0));

    send_frame(payload);
}

void Config::handle_query_config_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error requesting config!", QString("Could not query config! Error %1").arg((quint8)data.at(1)));
        return;
    }

    qDebug() << "Config: " << data.toHex();
    config_t config;
    config.balancingThreshold = ((quint16)data.at(3) << 8) | (quint8)data.at(2);
    config.numberOfStacks = (quint8)data.at(4) & 0x0F;
    config.autoResetOnPowerCycleEnable = ((quint8)data.at(4) >> 4) & 0x01;
    config.automaticSocLookupEnable = ((quint8)data.at(4) >> 5) & 0x01;
    config.globalBalancingEnable = (data.at(4) >> 6) & 0x01;

    qDebug() << "Balancing threshold: " << config.balancingThreshold;
    this->config = config;
    update_UI_config();
    ui->status->setText("Ready.");
}

void Config::handle_update_config_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error requesting config!", QString("Could not query config! Error %1").arg((quint8)data.at(1)));
        return;
    }

    ui->status->setText("Ready.");
}

void Config::handle_soc_lookup_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error updating SOC!", QString("Could not force SOC update! Error %1").arg((quint8)data.at(1)));
        return;
    }
}

void Config::handle_set_rtc_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error setting RTC!", QString("Could not set RTC! Error %1").arg((quint8)data.at(1)));
        return;
    }
}

void Config::handle_control_calibration_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error initializing ADC calibration!", QString("Could not initialize ADC calibration! Error %1").arg((quint8)data.at(1)));
        return;
    }
}

void Config::handle_calibration_value_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error setting calibration value!", QString("Could not set calibration value! Error %1").arg((quint8)data.at(1)));
        return;
    }
}

void Config::handle_load_default_config_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        QMessageBox::critical(this, "Error loading default config!", QString("Could not load default config! Error %1").arg((quint8)data.at(1)));
        return;
    }
}

void Config::handle_cal_response(QByteArray data)
{
    quint8 ID = data.at(0) & 0xF7;

    switch (ID) {
        case ID_LOAD_DEFAULT_CONFIG:
            handle_load_default_config_response(data);
            break;
        case ID_QUERY_CONFIG:
            handle_query_config_response(data);
            break;
        case ID_UPDATE_CONFIG:
            handle_update_config_response(data);
            break;
        case ID_SOC_LOOKUP:
            handle_soc_lookup_response(data);
            break;
        case ID_SET_RTC:
            handle_set_rtc_response(data);
            break;
        case ID_CONTROL_CALIBRATION:
            handle_control_calibration_response(data);
            break;
        case ID_CALIBRATION_STATE:
            handle_poll_cal_state_response(data);
            break;
        case ID_CALIBRATION_VALUE:
            handle_calibration_value_response(data);
            break;
    }
}

void Config::update_UI_config()
{
    oldConfig = config;
    ui->cbBalancingEnabled->setChecked(config.globalBalancingEnable);
    ui->cbAutoSocLookup->setChecked(config.automaticSocLookupEnable);
    ui->cbSdcAutoReset->setChecked(config.autoResetOnPowerCycleEnable);
    ui->numberOfStacks->setValue(config.numberOfStacks);
    ui->balancingThreshold->setValue((float)config.balancingThreshold / 10000.0f);

    //Clear all stylesheets after querying config
    ui->cbBalancingEnabled->setStyleSheet("");
    ui->balancingThreshold->setPalette(this->style()->standardPalette());
    ui->cbAutoSocLookup->setStyleSheet("");
    ui->numberOfStacks->setPalette(this->style()->standardPalette());
    ui->cbSdcAutoReset->setStyleSheet("");

    ui->status->setText("Ready.");
    this->setEnabled(true);
}

void Config::update_time(QByteArray data)
{
    if (data.length() != 8) {
        return;
    }

    quint32 uptime;
    ::memcpy(&uptime, data.data_ptr()->data(), sizeof(quint32));
    quint32 unixTimestamp;
    ::memcpy(&unixTimestamp, data.data_ptr()->data() + 4, sizeof(quint32));
    QDateTime dateTime;
    dateTime.setTime_t(unixTimestamp);


    QString uptimeStr = QString("%1").arg(QDateTime::fromTime_t(uptime / 10).toUTC().toString("hh:mm:ss"));

    ui->lUptime->setText(uptimeStr);
    ui->bmuTime->setText(dateTime.toString("dd.MM.yyyy hh:mm:ss"));
}

void Config::send_frame(QByteArray payload)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_ID_CAL_REQUEST);
    frame.setPayload(payload);
    emit can_send(frame);
}


void Config::showEvent(QShowEvent *event)
{
    (void) event;
    query_config();
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

void Config::on_cbSdcAutoReset_stateChanged(int arg1)
{
    config.autoResetOnPowerCycleEnable = (bool)arg1;
    if (config.autoResetOnPowerCycleEnable != oldConfig.autoResetOnPowerCycleEnable) {
        ui->cbSdcAutoReset->setStyleSheet("color:red");
    } else {
        ui->cbSdcAutoReset->setStyleSheet("");
    }
}

void Config::on_btnWrite_clicked()
{
    update_config();
}


void Config::on_btnSyncRtc_clicked()
{
    QDateTime dateTime(QDateTime::currentDateTime());
    quint32 unixTime = dateTime.toTime_t();
    QByteArray payload;
    payload.append(ID_SET_RTC);
    payload.resize(5);
    ::memcpy(payload.data_ptr()->data() + 1, &unixTime, sizeof(quint32));
    send_frame(payload);
}


void Config::on_btnStartCal_clicked()
{
    QByteArray payload;
    payload.append(ID_CONTROL_CALIBRATION);
    payload.append(ui->calInput->currentIndex() + 1);
    send_frame(payload);
    calStateTimer->start();
}

void Config::poll_cal_state()
{
    QByteArray payload;
    payload.append(ID_CALIBRATION_STATE);
    send_frame(payload);
}

void Config::handle_poll_cal_state_response(QByteArray &data)
{
    if (data.at(0) & 0x08) {
        calStateTimer->stop();
        QMessageBox::critical(this, "Error polling cal state!", QString("Could not poll cal state! Error %1").arg((quint8)data.at(1)));
        return;
    }

    switch (data.at(1)) {
    case CAL_STATE_STANDBY:
        calStateTimer->stop();
        ui->calStatus->setText("Calibration standby.");
        config_ui_disable();
        break;
    case CAL_STATE_WAIT_FOR_VALUE_1:
        ui->calStatus->setText("Calibration waiting for first value.");
        config_ui_enable();
        break;
    case CAL_STATE_WAIT_FOR_VALUE_2:
        ui->calStatus->setText("Calibration waiting for second value.");
        config_ui_enable();
        break;
    case CAL_STATE_UPDATE_CALIBRATION:
        ui->calStatus->setText("Calibration updating...");
        config_ui_disable();
        break;
    case CAL_STATE_FINISH: {
            QByteArray payload;
            payload.append(ID_CONTROL_CALIBRATION);
            payload.append(char(0));
            send_frame(payload);
            ui->calStatus->setText("Calibration finished!");
            config_ui_disable();
            break;
        }
    default:
        break;
    }
}

void Config::config_ui_disable()
{
    ui->label_8->setEnabled(false);
    ui->calActualValue->setEnabled(false);
    ui->btnApplyCalValue->setEnabled(false);
    ui->calInput->setEnabled(true);
    ui->btnStartCal->setEnabled(true);
}

void Config::config_ui_enable()
{
    ui->label_8->setEnabled(true);
    ui->calActualValue->setEnabled(true);
    ui->btnApplyCalValue->setEnabled(true);
    ui->calInput->setEnabled(false);
    ui->btnStartCal->setEnabled(false);
}


void Config::on_btnApplyCalValue_clicked()
{
    float val = ui->calActualValue->value();
    QByteArray payload;
    payload.append(ID_CALIBRATION_VALUE);
    payload.resize(5);
    ::memcpy(payload.data_ptr()->data() + 1, &val, sizeof(float));
    send_frame(payload);
}


void Config::on_btnForceSocLookup_clicked()
{
    QByteArray payload;
    payload.append(ID_SOC_LOOKUP);
    send_frame(payload);
}


void Config::on_btnDefault_clicked()
{
    QByteArray payload;
    payload.append(ID_LOAD_DEFAULT_CONFIG);
    send_frame(payload);
}

