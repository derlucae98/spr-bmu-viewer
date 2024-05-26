#include "lv_accu.h"

LV_Accu::LV_Accu(QObject *parent) : QObject(parent)
{
    ::memset(&canData, 0, sizeof(lv_battery_data_t));

    timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(500);
    QObject::connect(timeoutTimer, &QTimer::timeout, this, [=]{
        emit link_availability_changed(false);
        qDebug() << Qt::hex << fullUpdate;
        if (updateTimer) {
            updateTimer->stop();
        }
    });
    timeoutTimer->start();

    updateTimer = new QTimer(this);
    updateTimer->setInterval(500);
    QObject::connect(updateTimer, &QTimer::timeout, this, [=]{
        emit new_data(canData);
    });

    fullUpdate = 0;
}

QString LV_Accu::lv_state_to_string(lv_state_t state)
{
    switch (state) {
    case LV_Accu::LV_STATE_STANDBY:
        return "Standby";
    case LV_Accu::LV_STATE_OPERATE:
        return "Operate";
    case LV_Accu::LV_STATE_ERROR:
        return "Error";
    }
}

QString LV_Accu::sensor_status_to_string(sensor_status_t status)
{
    switch (status) {
    case LV_Accu::NOERROR:
        return "OK";
    case LV_Accu::PECERROR:
        return "PEC error";
    case LV_Accu::VALUEOUTOFRANGE:
        return "Value out of range";
    case LV_Accu::OPENWIRE:
        return "Open wire";
    default:
        return "Unknown error";
    }
}

void LV_Accu::can_frame(QCanBusFrame frame)
{

    switch (frame.frameId()) {
    case CAN_ID_LV_INFO:
        decompose_info(frame.payload());
        fullUpdate |= (1 << 0);
        break;
    case CAN_ID_LV_STATS_1:
        decompose_stats_1(frame.payload());
        fullUpdate |= (1 << 1);
        break;
    case CAN_ID_LV_STATS_2:
        decompose_stats_2(frame.payload());
        fullUpdate |= (1 << 2);
        break;
    case CAN_ID_LV_UIP:
        decompose_uip(frame.payload());
        fullUpdate |= (1 << 3);
        break;
    case CAN_ID_LV_CELL_VOLTAGE_1:
        decompose_cell_voltage_1(frame.payload());
        fullUpdate |= (1 << 4);
        break;
    case CAN_ID_LV_CELL_VOLTAGE_2:
        decompose_cell_voltage_2(frame.payload());
        fullUpdate |= (1 << 5);
        break;
    case CAN_ID_LV_CELL_TEMPERATURE_1:
        decompose_cell_temperature_1(frame.payload());
        fullUpdate |= (1 << 6);
        break;
    case CAN_ID_LV_BALANCING_FEEDBACK:
        decompose_balancing_feedback(frame.payload());
        fullUpdate |= (1 << 7);
        break;
    case CAN_ID_LV_CELL_TEMPERATURE_2:
        decompose_cell_temperature_2(frame.payload());
        fullUpdate |= (1 << 8);
        break;
    case CAN_ID_LV_STATE:
        decompose_state(frame.payload());
        fullUpdate |= (1 << 9);
        break;
    }

    if (fullUpdate == 0x3FF) {
        timeoutTimer->start();
        emit link_availability_changed(true);
        if (!updateTimer->isActive()) {
            updateTimer->start();
        }
        fullUpdate = 0;
    }
}

void LV_Accu::decompose_info(QByteArray data)
{
    canData.errorCode = static_cast<contactor_error_t>(((quint8)data.at(0) << 24) | ((quint8)data.at(1) << 16) | ((quint8)data.at(2) << 8) | ((quint8)data.at(3)));
}

void LV_Accu::decompose_stats_1(QByteArray data)
{
    canData.voltageValid = data.at(0) & 0x01;
    canData.minCellVolt = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.0001f;
    canData.maxCellVolt = (((quint8)data.at(3) << 8) | (quint8)data.at(4)) * 0.0001f;
    canData.avgCellVolt = (((quint8)data.at(5) << 8) | (quint8)data.at(6)) * 0.0001f;
}

void LV_Accu::decompose_stats_2(QByteArray data)
{
    canData.tempValid = (quint8)data.at(0) & 0x01;
    canData.minTemp = (quint8)data.at(1) * 0.5f;
    canData.maxTemp = (quint8)data.at(2) * 0.5f;
    canData.avgTemp = (quint8)data.at(3) * 0.5f;
    canData.soc = (quint8)data.at(4);
    canData.socValid = ((quint8)data.at(0) >> 1) & 0x01;
}

void LV_Accu::decompose_state(QByteArray data)
{
    canData.state = static_cast<LV_Accu::lv_state_t>(data.at(0));
    canData.criticalCellvoltage = data.at(1) & 0x01;
}

void LV_Accu::decompose_uip(QByteArray data)
{
    canData.currentValid = ((quint8)data.at(0) >> 1) & 0x01;
    canData.batteryVoltageValid = ((quint8)data.at(0) >> 0) & 0x01;
    canData.batteryVoltage = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.01f;
    canData.current = qint16((((quint8)data.at(3) << 8) | (quint8)data.at(4))) * 0.001f;
}

void LV_Accu::decompose_cell_voltage_1(QByteArray data)
{
    canData.cellVoltageStatus[0] = static_cast<sensor_status_t>((quint8)data.at(0) & 0x01);
    canData.cellVoltageStatus[1] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[2] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[3] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[0] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[1] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[2] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void LV_Accu::decompose_cell_voltage_2(QByteArray data)
{
    canData.cellVoltageStatus[4] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[5] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[6] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[3] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[4] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[5] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void LV_Accu::decompose_cell_temperature_1(QByteArray data)
{
    canData.temperatureStatus[0] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 0) & 0x03);
    canData.temperatureStatus[1] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.temperatureStatus[2] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.temperatureStatus[3] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 6) & 0x03);
    canData.temperatureStatus[4] = static_cast<sensor_status_t>(((quint8)data.at(0) >> 0) & 0x03);
    canData.temperatureStatus[5] = static_cast<sensor_status_t>(((quint8)data.at(0) >> 2) & 0x03);
    canData.temperature[0] = (quint8)data.at(2) * 0.5f;
    canData.temperature[1] = (quint8)data.at(3) * 0.5f;
    canData.temperature[2] = (quint8)data.at(4) * 0.5f;
    canData.temperature[3] = (quint8)data.at(5) * 0.5f;
    canData.temperature[4] = (quint8)data.at(6) * 0.5f;
    canData.temperature[5] = (quint8)data.at(7) * 0.5f;
}

void LV_Accu::decompose_cell_temperature_2(QByteArray data)
{
    canData.temperatureStatus[6] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 0) & 0x03);
    canData.temperatureStatus[7] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.temperature[6] = (quint8)data.at(2) * 0.5f;
    canData.temperature[7] = (quint8)data.at(3) * 0.5f;
}

void LV_Accu::decompose_balancing_feedback(QByteArray data)
{
    canData.balancingFeedback[5]   = (data.at(1) >> 5) & 0x01;
    canData.balancingFeedback[4]   = (data.at(1) >> 4) & 0x01;
    canData.balancingFeedback[3]   = (data.at(1) >> 3) & 0x01;
    canData.balancingFeedback[2]   = (data.at(1) >> 2) & 0x01;
    canData.balancingFeedback[1]   = (data.at(1) >> 1) & 0x01;
    canData.balancingFeedback[0]   = (data.at(1) >> 0) & 0x01;
}
