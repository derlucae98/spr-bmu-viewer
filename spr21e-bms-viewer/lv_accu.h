#ifndef LV_ACCU_H
#define LV_ACCU_H

#include <QObject>
#include <QCanBusFrame>
#include <QTimer>
#include <QDebug>

class LV_Accu : public QObject
{
    Q_OBJECT
public:
    explicit LV_Accu(QObject *parent = nullptr);

    static constexpr int MAX_NUM_OF_LV_CELLS = 6;
    static constexpr int MAX_NUM_OF_LV_TEMPSENS = 8;

    enum contactor_error_t {
        ERROR_NO_ERROR                          = 0x0,
        ERROR_IMD_FAULT                         = 0x1,
        ERROR_AMS_FAULT                         = 0x2,
        ERROR_IMPLAUSIBLE_CONTACTOR             = 0x4,
        ERROR_IMPLAUSIBLE_DC_LINK_VOLTAGE       = 0x8,
        ERROR_IMPLAUSIBLE_BATTERY_VOLTAGE       = 0x10,
        ERROR_IMPLAUSIBLE_CURRENT               = 0x20,
        ERROR_CURRENT_OUT_OF_RANGE              = 0x40,
        ERROR_PRE_CHARGE_TIMEOUT                = 0x80,
        ERROR_SDC_OPEN                          = 0x100,
        ERROR_AMS_POWERSTAGE_DISABLED           = 0x200,
        ERROR_IMD_POWERSTAGE_DISABLED           = 0x400,
        ERROR_AMS_CELL_VOLTAGE_OUT_OF_RANGE     = 0x800,
        ERROR_AMS_CELL_TEMPERATURE_OUT_OF_RANGE = 0x1000,
        ERROR_AMS_CELL_OPEN_WIRE                = 0x2000,
        ERROR_AMS_TEMPERATURE_OPEN_WIRE         = 0x4000,
        ERROR_AMS_DAISYCHAIN_ERROR              = 0x8000
    };

    enum sensor_status_t {
        NOERROR         = 0x0, //!< NOERROR
        PECERROR        = 0x1, //!< PECERROR
        VALUEOUTOFRANGE = 0x2, //!< VALUEOUTOFRANGE
        OPENWIRE    = 0x3, //!< OPENWIRE
    };

    enum lv_state_t {
        LV_STATE_STANDBY,
        LV_STATE_OPERATE,
        LV_STATE_ERROR
    };

    struct lv_battery_data_t {
        contactor_error_t errorCode;

        float minCellVolt;
        float maxCellVolt;
        float avgCellVolt;
        bool voltageValid;

        float minTemp;
        float maxTemp;
        float avgTemp;
        bool tempValid;

        float soc;
        bool socValid;
        float batteryVoltage;
        bool batteryVoltageValid;
        float current;
        bool currentValid;

        float cellVoltage[MAX_NUM_OF_LV_CELLS];
        sensor_status_t cellVoltageStatus[MAX_NUM_OF_LV_CELLS+1];
        float temperature[MAX_NUM_OF_LV_TEMPSENS];
        sensor_status_t temperatureStatus[MAX_NUM_OF_LV_TEMPSENS];
        bool balancingFeedback[MAX_NUM_OF_LV_CELLS];
        lv_state_t state;
        bool criticalCellvoltage;
    };

    static QString lv_state_to_string(LV_Accu::lv_state_t state);
    static QStringList contactor_error_to_string(LV_Accu::contactor_error_t error);
    static QString sensor_status_to_string(LV_Accu::sensor_status_t status);

    void can_frame(QCanBusFrame frame); //Connect to CAN receive signal
    bool link_available();

private:

    enum can_id {
        CAN_ID_LV_STARTUP = 0x301,
        CAN_ID_LV_INFO    = 0x252,
        CAN_ID_LV_STATS_1 = 0x253,
        CAN_ID_LV_STATS_2 = 0x254,
        CAN_ID_LV_UIP     = 0x255,
        CAN_ID_LV_CELL_VOLTAGE_1 = 0x256,
        CAN_ID_LV_CELL_VOLTAGE_2 = 0x257,
        CAN_ID_LV_CELL_TEMPERATURE_1 = 0x25A,
        CAN_ID_LV_CELL_TEMPERATURE_2 = 0x25B,
        CAN_ID_LV_BALANCING_FEEDBACK = 0x25C,
        CAN_ID_LV_STATE = 0x251
    };

    lv_battery_data_t canData;

    void decompose_info(QByteArray data);
    void decompose_stats_1(QByteArray data);
    void decompose_stats_2(QByteArray data);
    void decompose_state(QByteArray data);
    void decompose_uip(QByteArray data);
    void decompose_cell_voltage_1(QByteArray data);
    void decompose_cell_voltage_2(QByteArray data);
    void decompose_cell_temperature_1(QByteArray data);
    void decompose_cell_temperature_2(QByteArray data);
    void decompose_balancing_feedback(QByteArray data);

    uint16_t fullUpdate;
    QTimer *timeoutTimer = nullptr;
    QTimer *updateTimer = nullptr;

signals:
    void link_availability_changed(bool available);
    void new_data(lv_battery_data_t data);
    void lv_state_changed(LV_Accu::lv_state_t, LV_Accu::contactor_error_t error);
};

#endif // LV_ACCU_H
