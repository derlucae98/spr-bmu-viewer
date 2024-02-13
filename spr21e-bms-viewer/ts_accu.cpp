#include "ts_accu.h"

QStringList TS_Accu::contactor_error_list  = {
    "Error cleared.",
    "IMD fault!",
    "AMS fault!",
    "Implausible contactor!",
    "Implausible DC-Link voltage!",
    "Implausible Battery voltage!",
    "Implausible current!",
    "Current out of range!",
    "Pre-charge timeout!",
    "SDC open!",
    "AMS pwerstage waiting for reset...",
    "IMD pwerstage waiting for reset..."
};


TS_Accu::TS_Accu(QWidget *parent) : QWidget(parent)
{
    ::memset(&canData, 0, sizeof(ts_battery_data_t));

    linkAvailable = false;
    tsControl = false;
    tsActive = false;

    timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(200);
    QObject::connect(timeoutTimer, &QTimer::timeout, this, [=]{
        linkAvailable = false;
        emit link_availability_changed(false);
        if (updateTimer) {
            updateTimer->stop();
        }
    });
    timeoutTimer->start();

    sendTimer = new QTimer(this);
    sendTimer->setInterval(100);
    QObject::connect(sendTimer, &QTimer::timeout, this, &TS_Accu::send_timer_timeout);

    updateTimer = new QTimer(this);
    updateTimer->setInterval(500);
    QObject::connect(updateTimer, &QTimer::timeout, this, [=]{
        emit new_data(canData);
    });
}

void TS_Accu::can_frame(QCanBusFrame frame)
{
    emit can_frame_forward(frame, {});
    static uint16_t fullUpdate = 0;

    switch (frame.frameId()) {
    case CAN_ID_INFO:
        decompose_info(frame.payload());
        fullUpdate |= (1 << 0);
        break;
    case CAN_ID_STATS_1:
        decompose_stats_1(frame.payload());
        fullUpdate |= (1 << 1);
        break;
    case CAN_ID_STATS_2:
        decompose_stats_2(frame.payload());
        fullUpdate |= (1 << 2);
        break;
    case CAN_ID_UIP:
        decompose_uip(frame.payload());
        fullUpdate |= (1 << 3);
        break;
    case CAN_ID_CELL_VOLTAGE_1:
        decompose_cell_voltage_1(frame.payload());
        fullUpdate |= (1 << 4);
        break;
    case CAN_ID_CELL_VOLTAGE_2:
        decompose_cell_voltage_2(frame.payload());
        fullUpdate |= (1 << 5);
        break;
    case CAN_ID_CELL_VOLTAGE_3:
        decompose_cell_voltage_3(frame.payload());
        fullUpdate |= (1 << 6);
        break;
    case CAN_ID_CELL_VOLTAGE_4:
        decompose_cell_voltage_4(frame.payload());
        fullUpdate |= (1 << 7);
        break;
    case CAN_ID_CELL_TEMPERATURE:
        decompose_cell_temperature(frame.payload());
        fullUpdate |= (1 << 8);
        break;
    case CAN_ID_BALANCING_FEEDBACK:
        decompose_balancing_feedback(frame.payload());
        fullUpdate |= (1 << 9);
        break;
    case CAN_ID_UNIQUE_ID:
        decompose_unique_id(frame.payload());
        fullUpdate |= (1 << 10);
        break;
    }

    if (fullUpdate == 0x7FF) {
        linkAvailable = true;
        timeoutTimer->start();
        emit link_availability_changed(true);
        if (!updateTimer->isActive()) {
            updateTimer->start();
        }
        fullUpdate = 0;
    }
}

void TS_Accu::open_config_dialog()
{
    Config *configDialog = new Config(this);
    QObject::connect(this, &TS_Accu::can_frame_forward, configDialog, &Config::can_recv);
    QObject::connect(configDialog, &Config::can_send, this, &TS_Accu::can_send);
    configDialog->setAttribute(Qt::WA_DeleteOnClose);
    configDialog->exec();
}

bool TS_Accu::link_available()
{
    return linkAvailable;
}

void TS_Accu::ts_take_control(bool control)
{
    tsControl = control;
    if (control) {
        sendTimer->start();
    } else {
        sendTimer->stop();
        QByteArray payload;
        QCanBusFrame frame;
        frame.setFrameId(CAN_ID_TS_REQUEST);
        payload.append((char)0);
        payload.append((char)0);
        frame.setPayload(payload);
        emit can_send(frame);
    }
}

void TS_Accu::ts_activate(bool active)
{
    tsActive = active;
}

void TS_Accu::decompose_info(QByteArray data)
{
    static ts_state_t tsStateOld = TS_STATE_STANDBY;

    canData.errorCode = static_cast<contactor_error_t>(((quint8)data.at(0) << 24) | ((quint8)data.at(1) << 16) | ((quint8)data.at(2) << 8) | ((quint8)data.at(3)));
    canData.isolationResistance = ((quint8)data.at(4) << 8) | (quint8)data.at(5);
    canData.isolationResistanceValid = (quint8)data.at(6) >> 7;
    canData.tsState = static_cast<ts_state_t>((quint8)data.at(6) & 0x03);

    if (tsStateOld != canData.tsState) {
        emit ts_state_changed(canData.tsState, canData.errorCode);
    }

    tsStateOld =  canData.tsState;
}

void TS_Accu::decompose_stats_1(QByteArray data)
{
    canData.voltageValid = data.at(0) & 0x01;
    canData.minCellVolt = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.0001f;
    canData.maxCellVolt = (((quint8)data.at(3) << 8) | (quint8)data.at(4)) * 0.0001f;
    canData.avgCellVolt = (((quint8)data.at(5) << 8) | (quint8)data.at(6)) * 0.0001f;
}

void TS_Accu::decompose_stats_2(QByteArray data)
{
    canData.dcLinkVoltageValid = ((quint8)data.at(0) >> 2) & 0x01;
    canData.socValid = ((quint8)data.at(0) >> 1) & 0x01;
    canData.tempValid = (quint8)data.at(0) & 0x01;
    canData.minTemp = (quint8)data.at(1) * 0.5f;
    canData.maxTemp = (quint8)data.at(2) * 0.5f;
    canData.avgTemp = (quint8)data.at(3) * 0.5f;
    canData.minSoc = (quint8)data.at(4);
    canData.maxSoc = (quint8)data.at(5);
    canData.dcLinkVoltage = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) / 10.0f;
}

void TS_Accu::decompose_uip(QByteArray data)
{
    canData.batteryPowerValid = ((quint8)data.at(0) >> 2) & 0x01;
    canData.currentValid = ((quint8)data.at(0) >> 1) & 0x01;
    canData.batteryVoltageValid = ((quint8)data.at(0) >> 0) & 0x01;
    canData.batteryVoltage = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.1f;
    canData.current = qint16((((quint8)data.at(3) << 8) | (quint8)data.at(4))) * 0.00625f;
    canData.batteryPower = (((quint8)data.at(5) << 8) | (quint8)data.at(6)) * 0.0025f;
}

void TS_Accu::decompose_cell_voltage_1(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][0] = static_cast<sensor_status_t>((quint8)data.at(0) & 0x01);
    canData.cellVoltageStatus[index][1] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[index][2] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[index][3] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[index][0] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][1] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][2] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void TS_Accu::decompose_cell_voltage_2(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][4] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[index][5] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[index][6] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[index][3] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][4] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][5] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void TS_Accu::decompose_cell_voltage_3(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][7] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[index][8] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[index][9] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[index][6] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][7] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][8] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void TS_Accu::decompose_cell_voltage_4(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][10] = static_cast<sensor_status_t>((quint8)data.at(1) & 0x03);
    canData.cellVoltageStatus[index][11] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.cellVoltageStatus[index][12] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.cellVoltage[index][9]  = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][10] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][11] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void TS_Accu::decompose_cell_temperature(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.temperatureStatus[index][0] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 0) & 0x03);
    canData.temperatureStatus[index][1] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 2) & 0x03);
    canData.temperatureStatus[index][2] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 4) & 0x03);
    canData.temperatureStatus[index][3] = static_cast<sensor_status_t>(((quint8)data.at(1) >> 6) & 0x03);
    canData.temperatureStatus[index][4] = static_cast<sensor_status_t>(((quint8)data.at(0) >> 0) & 0x03);
    canData.temperatureStatus[index][5] = static_cast<sensor_status_t>(((quint8)data.at(0) >> 2) & 0x03);
    canData.temperature[index][0] = (quint8)data.at(2) * 0.5f;
    canData.temperature[index][1] = (quint8)data.at(3) * 0.5f;
    canData.temperature[index][2] = (quint8)data.at(4) * 0.5f;
    canData.temperature[index][3] = (quint8)data.at(5) * 0.5f;
    canData.temperature[index][4] = (quint8)data.at(6) * 0.5f;
    canData.temperature[index][5] = (quint8)data.at(7) * 0.5f;
}

void TS_Accu::decompose_balancing_feedback(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;

    canData.balance[index][11]  = (data.at(0) >> 3) & 0x01;
    canData.balance[index][10]  = (data.at(0) >> 2) & 0x01;
    canData.balance[index][9]   = (data.at(0) >> 1) & 0x01;
    canData.balance[index][8]   = (data.at(0) >> 0) & 0x01;
    canData.balance[index][7]   = (data.at(1) >> 7) & 0x01;
    canData.balance[index][6]   = (data.at(1) >> 6) & 0x01;
    canData.balance[index][5]   = (data.at(1) >> 5) & 0x01;
    canData.balance[index][4]   = (data.at(1) >> 4) & 0x01;
    canData.balance[index][3]   = (data.at(1) >> 3) & 0x01;
    canData.balance[index][2]   = (data.at(1) >> 2) & 0x01;
    canData.balance[index][1]   = (data.at(1) >> 1) & 0x01;
    canData.balance[index][0]   = (data.at(1) >> 0) & 0x01;
}

void TS_Accu::decompose_unique_id(QByteArray data)
{
    quint8 stack = (quint8)(data.at(0) >> 4) & 0x0F;
    canData.UID[stack] = (quint8)data.at(1) << 24 | (quint8)data.at(2) << 16 | (quint8)data.at(3) << 8 | (quint8)data.at(4);
}

void TS_Accu::send_timer_timeout()
{
    QCanBusFrame frame;
    QByteArray payload;
    frame.setFrameId(CAN_ID_TS_REQUEST);
    payload.append(0x01);
    payload.append(tsActive ? (char)0xFF : (char)0x00);
    frame.setPayload(payload);
    emit can_send(frame);
}

QStringList TS_Accu::contactor_error_to_string(TS_Accu::contactor_error_t error)
{
    QStringList errorStrings;
    quint32 err = static_cast<quint32>(error);
    if (!err) {
        errorStrings.push_back(contactor_error_list[0]);
    } else {
        for (size_t i = 0; i < 32; i++) {
            quint32 index = (1 << i);
            if (err & index) {
                errorStrings.push_back(contactor_error_list[i + 1]);
            }
        }
    }
    return errorStrings;
}

QString TS_Accu::sensor_status_to_string(sensor_status_t status)
{
    switch (status) {
    case TS_Accu::NOERROR:
        return "OK";
    case TS_Accu::PECERROR:
        return "PEC error";
    case TS_Accu::VALUEOUTOFRANGE:
        return "Value out of range";
    case TS_Accu::OPENWIRE:
        return "Open wire";
    default:
        return "Unknown error";
    }
}


QString TS_Accu::ts_state_to_string(TS_Accu::ts_state_t state)
{
    switch (state) {
    case TS_Accu::TS_STATE_STANDBY:
        return "Standby";
    case TS_Accu::TS_STATE_PRE_CHARGING:
        return "Pre-charging";
    case TS_Accu::TS_STATE_OPERATE:
        return "Operate";
    case TS_Accu::TS_STATE_ERROR:
        return "Error";
    }
}
