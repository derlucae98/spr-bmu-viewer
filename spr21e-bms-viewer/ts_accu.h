#ifndef TS_ACCU_H
#define TS_ACCU_H

#include <QObject>
#include <QCanBusFrame>
#include <QTimer>
#include "config.h"

class TS_Accu : public QWidget
{
    Q_OBJECT
public:
    explicit TS_Accu(QWidget *parent = nullptr);

    static constexpr int MAX_NUM_OF_SLAVES    = 12;
    static constexpr int  MAX_NUM_OF_CELLS    = 12;
    static constexpr int  MAX_NUM_OF_TEMPSENS = 6;

    enum ts_state_t {
        TS_STATE_STANDBY,
        TS_STATE_PRE_CHARGING,
        TS_STATE_OPERATE,
        TS_STATE_ERROR
    };

    enum contactor_error_t {
        ERROR_NO_ERROR                    = 0x0,
        ERROR_IMD_FAULT                   = 0x1,
        ERROR_AMS_FAULT                   = 0x2,
        ERROR_IMPLAUSIBLE_CONTACTOR       = 0x4,
        ERROR_IMPLAUSIBLE_DC_LINK_VOLTAGE = 0x8,
        ERROR_IMPLAUSIBLE_BATTERY_VOLTAGE = 0x10,
        ERROR_IMPLAUSIBLE_CURRENT         = 0x20,
        ERROR_CURRENT_OUT_OF_RANGE        = 0x40,
        ERROR_PRE_CHARGE_TIMEOUT          = 0x80,
        ERROR_SDC_OPEN                    = 0x100,
        ERROR_AMS_POWERSTAGE_DISABLED     = 0x200,
        ERROR_IMD_POWERSTAGE_DISABLED     = 0x400
    };

    enum LTCError_t{
        NOERROR         = 0x0, //!< NOERROR
        PECERROR        = 0x1, //!< PECERROR
        VALUEOUTOFRANGE = 0x2, //!< VALUEOUTOFRANGE
        OPENCELLWIRE    = 0x3, //!< OPENCELLWIRE
    };

    struct ts_battery_data_t {
        //Info
        float isolationResistance;
        bool isolationResistanceValid;
        quint32 errorCode;
        ts_state_t tsState;

        //Stats1
        float minCellVolt;
        float maxCellVolt;
        float avgCellVolt;
        bool voltageValid;

        //Stats2
        quint16 minSoc;
        quint16 maxSoc;
        bool socValid;
        float dcLinkVoltage;
        bool dcLinkVoltageValid;
        float minTemp;
        float maxTemp;
        float avgTemp;
        bool tempValid;

        //UIP
        float batteryVoltage;
        bool batteryVoltageValid;
        float current;
        bool currentValid;
        float batteryPower;
        bool batteryPowerValid;

        //Unique ID
        quint32 UID[MAX_NUM_OF_SLAVES];

        //Cell voltage / temperature
        float cellVoltage[MAX_NUM_OF_SLAVES][MAX_NUM_OF_CELLS];
        quint8 cellVoltageStatus[MAX_NUM_OF_SLAVES][MAX_NUM_OF_CELLS+1];
        float temperature[MAX_NUM_OF_SLAVES][MAX_NUM_OF_TEMPSENS];
        quint8 temperatureStatus[MAX_NUM_OF_SLAVES][MAX_NUM_OF_TEMPSENS];

        //Balancing feedback
        quint8 balance[MAX_NUM_OF_SLAVES][MAX_NUM_OF_CELLS];

        //Time
        quint32 uptime;
        quint32 sysTime;
    };



    void can_frame(QCanBusFrame frame); //Connect to CAN receive signal
    void open_config_dialog();
    bool link_available();
    void ts_take_control(bool control);
    void ts_activate(bool active);

private:
    enum can_id {
        CAN_ID_TS_REQUEST         = 0x100,
        CAN_ID_STARTUP            = 0x101,
        CAN_ID_INFO               = 0x102,
        CAN_ID_STATS_1            = 0x103,
        CAN_ID_STATS_2            = 0x104,
        CAN_ID_UIP                = 0x105,
        CAN_ID_CELL_VOLTAGE_1     = 0x106,
        CAN_ID_CELL_VOLTAGE_2     = 0x107,
        CAN_ID_CELL_VOLTAGE_3     = 0x108,
        CAN_ID_CELL_VOLTAGE_4     = 0x109,
        CAN_ID_CELL_TEMPERATURE   = 0x10A,
        CAN_ID_BALANCING_FEEDBACK = 0x10B,
        CAN_ID_UNIQUE_ID          = 0x10C,
        CAN_ID_TIME               = 0x10D,
        CAN_ID_DIAG_REQUEST       = 0x10E,
        CAN_ID_DIAG_RESPONSE      = 0x10F
    };

    ts_battery_data_t canData;

    void decompose_info(QByteArray data);
    void decompose_stats_1(QByteArray data);
    void decompose_stats_2(QByteArray data);
    void decompose_uip(QByteArray data);
    void decompose_cell_voltage_1(QByteArray data);
    void decompose_cell_voltage_2(QByteArray data);
    void decompose_cell_voltage_3(QByteArray data);
    void decompose_cell_voltage_4(QByteArray data);
    void decompose_cell_temperature(QByteArray data);
    void decompose_balancing_feedback(QByteArray data);
    void decompose_unique_id(QByteArray data);

    bool linkAvailable;
    QTimer *timeoutTimer = nullptr;
    QTimer *sendTimer = nullptr;
    QTimer *updateTimer = nullptr;
    void send_timer_timeout();
    bool tsControl;
    bool tsActive;

signals:
    void link_availability_changed(bool available);
    void can_send(QCanBusFrame frame); //connect to CAN send slot
    void new_data(ts_battery_data_t data);
    void can_frame_forward(QCanBusFrame, QPrivateSignal); //Needed for config dialog
};

#endif // TS_ACCU_H
