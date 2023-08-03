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

void Config::handle_cal_response(QByteArray data)
{
    quint8 ID = data.at(0) & 0xF7;

    switch (ID) {
        case ID_LOAD_DEFAULT_CONFIG:
            break;
        case ID_QUERY_CONFIG:
            handle_query_config_response(data);
            break;
        case ID_UPDATE_CONFIG:
            handle_update_config_response(data);
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

void Config::send_frame(QByteArray payload)
{
    QCanBusFrame frame;
    frame.setFrameId(CAN_ID_CAL_REQUEST);
    frame.setPayload(payload);
    can_send(frame);
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

