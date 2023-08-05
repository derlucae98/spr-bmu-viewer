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
        CAN_ID_CAL_REQUEST  = 0x012,
        CAN_ID_CAL_RESPONSE = 0x013,
        CAN_ID_TIME         = 0x014,
        CAN_ID_STARTUP      = 0x020
    };

    enum error_codes {
        CAL_ERROR_NO_ERROR,
        CAL_ERROR_PARAM_DOES_NOT_EXIST,
        CAL_ERROR_DLC_MISMATCH, //Expected more or less bytes of payload
        CAL_ERROR_INTERNAL_ERROR
    };


    void query_config();
    void update_config();

    void handle_query_config_response(QByteArray &data);
    void handle_update_config_response(QByteArray &data);

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
