#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QDebug>
#include <QTimer>
#include <QtMath>
#include "can.h"
#include <QDateTime>
#include <QPixmap>
#include "ts_accu.h"
#include "errordialog.h"


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
    void on_tsTakeControl_stateChanged(int arg1);
    void on_reqTsActive_stateChanged(int arg1);
    void on_btnConfig_clicked();
    void on_btnShowErrors_clicked();

private:
    Ui::MainWindow *ui;

    TS_Accu *tsAccu = nullptr;

    enum can_id {
        CAN_ID_LV_ACCU_STATS_2    = 0x320,
        CAN_ID_LV_ACCU_STATS_1    = 0x321,
        CAN_ID_LV_ACCU_STATE      = 0x322,
        CAN_ID_LV_ACCU_CELL_VOLT_03 = 0x323,
        CAN_ID_LV_ACCU_CELL_VOLT_45 = 0x324,
        CAN_ID_LV_ACCU_TEMP_03 = 0x325,
        CAN_ID_LV_ACCU_TEMP_47 = 0x326
    };

    struct can_data_LV_t {


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

    void decompose_lv_stats_1(QByteArray data);
    void decompose_lv_stats_2(QByteArray data);
    void decompose_lv_state(QByteArray data);
    void decompose_lv_cell_volt_03(QByteArray data);
    void decompose_lv_cell_volt_45(QByteArray data);
    void decompose_lv_temp_03(QByteArray data);
    void decompose_lv_temp_47(QByteArray data);

    bool interfaceUp;

    void new_frame(QCanBusFrame frame);

    Can *can = nullptr;


    bool linkAvailable;
    void ts_link_available(bool available);

    TS_Accu::ts_battery_data_t tsBatteryData;
    void update_ui();
    void update_ui_balancing();
    void update_ui_uid();
    void update_ui_voltage();
    void update_ui_temperature();
    void update_ui_stats();
    void update_ui_ts(TS_Accu::ts_battery_data_t data);
    void update_ui_lv();
    void ts_state_changed(TS_Accu::ts_state_t state, TS_Accu::contactor_error_t);
    void show_error_message();
    QString tsErrorString;

    enum severity_t {
        SEVERITY_INFO,
        SEVERITY_INFO_GREEN,
        SEVERITY_WARNING,
        SEVERITY_ERROR
    };

    void append_error(QString error, severity_t severity);
    void get_error_reason(TS_Accu::contactor_error_t error);
    void closeEvent(QCloseEvent *event);
    ErrorDialog *errorDialog = nullptr;

    bool darkMode;

signals:


};
#endif // MAINWINDOW_H
