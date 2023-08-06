#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QStandardItem>
#include <QCanBusFrame>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QTimer>



namespace Ui {
class Config;
}

class Config : public QDialog
{
    Q_OBJECT

public:
    explicit Config(QWidget *parent = nullptr);
    ~Config();
    void can_recv(QCanBusFrame frame);

private slots:
    void on_btnLoad_clicked();
    void on_cbBalancingEnabled_stateChanged(int arg1);
    void on_balancingThreshold_valueChanged(double arg1);
    void on_cbAutoSocLookup_stateChanged(int arg1);
    void on_numberOfStacks_valueChanged(int arg1);
    void on_cbSdcAutoReset_stateChanged(int arg1);
    void on_btnWrite_clicked();

    void on_btnSyncRtc_clicked();

    void on_btnStartCal_clicked();

    void on_btnApplyCalValue_clicked();

    void on_btnForceSocLookup_clicked();

    void on_btnDefault_clicked();

private:
    Ui::Config *ui;

    struct config_t{
        bool globalBalancingEnable;
        quint16 balancingThreshold;
        bool automaticSocLookupEnable;
        quint8 numberOfStacks;
        bool autoResetOnPowerCycleEnable;
    };

    enum {
        ID_LOAD_DEFAULT_CONFIG = 0x00,
        ID_QUERY_CONFIG        = 0x01,
        ID_UPDATE_CONFIG       = 0x02,
        ID_SOC_LOOKUP          = 0x10,
        ID_SET_RTC             = 0x20,
        ID_CONTROL_CALIBRATION = 0x30,
        ID_CALIBRATION_STATE   = 0x31,
        ID_CALIBRATION_VALUE   = 0x32
    };

    enum {
        CAN_ID_STARTUP      = 0x001,
        CAN_ID_TIME         = 0x00D,
        CAN_ID_CAL_REQUEST  = 0x00E,
        CAN_ID_CAL_RESPONSE = 0x00F
    };

    enum error_codes {
        CAL_ERROR_NO_ERROR,
        CAL_ERROR_PARAM_DOES_NOT_EXIST,
        CAL_ERROR_DLC_MISMATCH, //Expected more or less bytes of payload
        CAL_ERROR_INTERNAL_ERROR
    };

    typedef enum {
        CAL_STATE_STANDBY,              //!< State machine task is suspended
        CAL_STATE_WAIT_FOR_VALUE_1,     //!< State machine waits for the user to apply the first value (voltage or current) to the corresponding input
        CAL_STATE_WAIT_FOR_VALUE_2,     //!< State machine waits for the user to apply the second value (voltage or current) to the corresponding input
        CAL_STATE_UPDATE_CALIBRATION,   //!< New calibration values are calculated and stored to the EEPROM
        CAL_STATE_FINISH                //!< New calibration is stored in the EEPROM.
    } adc_cal_state_t;

    adc_cal_state_t calState;

    QTimer *calStateTimer = nullptr;
    void poll_cal_state();
    void handle_poll_cal_state_response(QByteArray &data);
    void config_ui_disable();
    void config_ui_enable();


    void query_config();
    void update_config();

    void handle_query_config_response(QByteArray &data);
    void handle_update_config_response(QByteArray &data);
    void handle_soc_lookup_response(QByteArray &data);
    void handle_set_rtc_response(QByteArray &data);
    void handle_control_calibration_response(QByteArray &data);
    void handle_calibration_value_response(QByteArray &data);
    void handle_load_default_config_response(QByteArray &data);

    void handle_cal_response(QByteArray data);

    config_t config;
    config_t oldConfig;

    void update_UI_config();
    void update_time(QByteArray data);

    void send_frame(QByteArray payload);

    void showEvent(QShowEvent *event);

signals:
    void can_send(QCanBusFrame frame);
};

#endif // CONFIG_H
