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
#include "lv_accu.h"
#include "gateway.h"




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
    void on_btnConnectDevice_clicked();
    void on_clearErrorLog_clicked();
    void on_tsTakeControl_stateChanged(int arg1);
    void on_reqTsActive_stateChanged(int arg1);
    void on_btnConfig_clicked();
    void on_btnShowErrors_clicked();

    void on_tsToggleNotification_clicked();

    void on_lvToggleNotification_clicked();

    void on_tsAccuStatus_clicked();

private:
    Ui::MainWindow *ui;

    TS_Accu *tsAccu = nullptr;
    LV_Accu *lvAccu = nullptr;

    void init_ts();
    void init_lv();

    bool interfaceUp;

    Can *can = nullptr;
    void connect_can_dev();
    void disconnect_can_dev();

    Gateway *gateway = nullptr;
    void connect_gateway();
    void disconnect_gateway();

    bool tsLinkAvailable;
    bool lvLinkAvailable;
    void ts_link_available(bool available);
    void lv_link_available(bool available);

    void ui_ts_invalidate_all();
    void ui_lv_invalidate_all();

    TS_Accu::ts_battery_data_t tsBatteryData;
    LV_Accu::lv_battery_data_t lvBatteryData;

    void update_ui();
    void update_ui_ts_balancing();
    void update_ui_ts_uid();
    void update_ui_ts_voltage();
    void update_ui_ts_temperature();
    void update_ui_ts_stats();
    void update_ui_ts(TS_Accu::ts_battery_data_t data);
    bool tsNotifyOnErrors;
    bool lvNotifyOnErrors;
    void update_ui_lv(LV_Accu::lv_battery_data_t data);
    void ts_state_changed(TS_Accu::ts_state_t state, TS_Accu::contactor_error_t);
    void lv_state_changed(LV_Accu::lv_state_t state, LV_Accu::contactor_error_t);
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
