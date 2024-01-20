#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QDebug>
#include <QTimer>
#include <QtMath>
#include <QMessageBox>
#include "can.h"
#include <unistd.h>
#include <QDateTime>
#include <QProgressBar>
#include <QPixmap>
#include "logfileconverter.h"
#include "aboutdialog.h"
#include "config.h"

#define MAX_NUM_OF_SLAVES 12
#define MAX_NUM_OF_CELLS  12
#define MAX_NUM_OF_TEMPSENS 6
#define MAX_NUM_OF_LV_CELLS 6
#define MAX_NUM_OF_LV_TEMPSENS 8


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnectPcan_clicked();
    void on_clearErrorLog_clicked();
    void on_actionLogfile_converter_triggered();
    void on_actionAbout_SPR_BMS_viewer_triggered();
    void on_actionConfig_triggered();

    void on_tsTakeControl_stateChanged(int arg1);

private:
    enum LTCError_t{
        NOERROR         = 0x0, //!< NOERROR
        PECERROR        = 0x1, //!< PECERROR
        VALUEOUTOFRANGE = 0x2, //!< VALUEOUTOFRANGE
        OPENCELLWIRE    = 0x3, //!< OPENCELLWIRE
    };

    enum can_id {
        CAN_ID_TS_REQUEST         = 0x000,
        CAN_ID_STARTUP            = 0x001,
        CAN_ID_INFO               = 0x002,
        CAN_ID_STATS_1            = 0x003,
        CAN_ID_STATS_2            = 0x004,
        CAN_ID_UIP                = 0x005,
        CAN_ID_CELL_VOLTAGE_1     = 0x006,
        CAN_ID_CELL_VOLTAGE_2     = 0x007,
        CAN_ID_CELL_VOLTAGE_3     = 0x008,
        CAN_ID_CELL_VOLTAGE_4     = 0x009,
        CAN_ID_CELL_TEMPERATURE   = 0x00A,
        CAN_ID_BALANCING_FEEDBACK = 0x00B,
        CAN_ID_UNIQUE_ID          = 0x00C,
        CAN_ID_TIME               = 0x00D,
        CAN_ID_DIAG_REQUEST       = 0x00E,
        CAN_ID_DIAG_RESPONSE      = 0x00F,
        CAN_ID_LV_ACCU_STATS_2    = 0x320,
        CAN_ID_LV_ACCU_STATS_1    = 0x321,
        CAN_ID_LV_ACCU_STATE      = 0x322,
        CAN_ID_LV_ACCU_CELL_VOLT_03 = 0x323,
        CAN_ID_LV_ACCU_CELL_VOLT_45 = 0x324,
        CAN_ID_LV_ACCU_TEMP_03 = 0x325,
        CAN_ID_LV_ACCU_TEMP_47 = 0x326
    };

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

    struct can_data_t {
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

    can_data_t canData;

    struct can_data_LV_t {
        //Info
        quint32 errorCode;
        ts_state_t state;

        //Stats1
        float minCellVolt;
        float maxCellVolt;
        float avgCellVolt;
        bool voltageError;

        //Stats2
        quint16 soc;
        float batteryVoltage;
        float minTemp;
        float maxTemp;
        float avgTemp;
        bool tempValidError;

        float current;
        bool currentError;
        //Cell voltage / temperature
        float cellVoltage[MAX_NUM_OF_LV_CELLS];
        float temperature[MAX_NUM_OF_LV_TEMPSENS];

    };

    can_data_LV_t canDataLv;

    QTimer *updateTimer = nullptr;
    Ui::MainWindow *ui;

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


    void decompose_lv_stats_1(QByteArray data);
    void decompose_lv_stats_2(QByteArray data);
    void decompose_lv_state(QByteArray data);
    void decompose_lv_cell_volt_03(QByteArray data);
    void decompose_lv_cell_volt_45(QByteArray data);
    void decompose_lv_temp_03(QByteArray data);
    void decompose_lv_temp_47(QByteArray data);


    QString ts_state_to_string(ts_state_t state);
    QString returnValidity(quint8 val);

    bool interfaceUp;

    void new_frame(QCanBusFrame frame);

    Can *can = nullptr;
    QTimer *sendTimer = nullptr;
    void send_ts_req();

    QString error_to_string(contactor_error_t error);
    bool linkAvailable;

    void update_ui_balancing();
    void update_ui_uid();
    void update_ui_voltage();
    void update_ui_temperature();
    void update_ui_stats();
    void update_ui_periodic();
    void update_ui_lv();

    void closeEvent(QCloseEvent *event);

    bool darkMode;

signals:


};
#endif // MAINWINDOW_H
