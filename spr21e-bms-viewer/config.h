#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QStandardItem>
#include <QCanBusFrame>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include "isotp.h"



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

    void on_btnFormatSD_clicked();

    void on_btnDownload_clicked();

    void on_cbBalancingEnabled_stateChanged(int arg1);

    void on_balancingThreshold_valueChanged(double arg1);

    void on_cbAutoSocLookup_stateChanged(int arg1);

    void on_numberOfStacks_valueChanged(int arg1);

    void on_cbLoggerEnabled_stateChanged(int arg1);

    void on_cbDeleteOldestLog_stateChanged(int arg1);

    void on_cbSdcAutoReset_stateChanged(int arg1);

    void on_btnWrite_clicked();

    void on_btnLoggerFetch_clicked();

private:
    Ui::Config *ui;

    struct config_t{
        bool globalBalancingEnable;
        quint16 balancingThreshold;
        bool automaticSocLookupEnable;
        quint8 numberOfStacks;
        bool loggerEnable;
        bool loggerDeleteOldestFile;
        bool autoResetOnPowerCycleEnable;
        quint16 crc16;
    }  __attribute__((packed));

    struct file_info_t {
        char name[20];
        quint8 handle;
        quint32 size;
    } __attribute__((packed));

    quint16 calc_crc16(quint8 *data, size_t len);

    enum {
        ID_LOAD_DEFAULT_CONFIG = 0,
        ID_QUERY_CONFIG,
        ID_UPDATE_CONFIG,
        ID_BALANCING_FEEDBACK,
        ID_SOC_LOOKUP,
        ID_SET_RTC,
        ID_CONTROL_CALIBRATION,
        ID_CALIBRATION_STATE,
        ID_CALIBRATION_VALUE,
        ID_FORMAT_SD_CARD,
        ID_FORMAT_SD_CARD_STATUS,
        ID_QUERY_LOGFILE_INFO
    };

    enum isotp_transmission_type {
        ISOTP_CAL_REQUEST,
        ISOTP_CAL_RESPONSE,
        ISOTP_LOGFILE, //Transmitted data is a logfile
    };

    enum {
        CAN_ID_CAL_REQUEST  = 0x010,
        CAN_ID_CAL_RESPONSE = 0x011
    };

    enum error_codes {
        ERROR_PARAM_DOES_NOT_EXIST = 0x00,
        ERROR_CANNOT_MODIFY_RO_PARAMETER,
        ERROR_DLC_DOES_NOT_MATCH_NUMBER_OF_BYTES,
        ERROR_NUMBER_OF_BYTES_DOES_NOT_MATCH_DATATYPE,
        ERROR_INTERNAL_ERROR,
        ERROR_CANNOT_READ_WO_PARAMETER,
        ERROR_ISOTOP_ERROR,
        ERROR_CRC_ERROR
    };

    static constexpr quint8  CARD_FORMATTING_FINISHED = 0x10;
    static constexpr quint8  CARD_FORMATTING_BUSY = 0x11;
    static constexpr quint8  ERROR_CARD_FORMATTING_FAILED = 0x12;
    static constexpr quint8  ERROR_NO_CARD  = 0x13;
    static constexpr quint16 RECV_BUF_SIZE  = 4095;
    static constexpr quint16 SEND_BUF_SIZE  = 4095;
    static constexpr quint16 ISOTP_UPLINK   = 0x013;
    static constexpr quint16 ISOTP_DOWNLINK = 0x012;


    Isotp *isotp = nullptr;
    void on_new_isotp_message(QByteArray message);

    QTimer *pollTimer = nullptr;

    void handle_format_sd_response(QByteArray &frame);
    void poll_timer_callback();

    void query_config();
    void handle_query_config_response(QByteArray &data);
    void handle_update_config_response(QByteArray &data);
    void handle_cal_response(QByteArray data);
    void handle_logfile_info_response(QByteArray data);


    config_t config;
    config_t oldConfig;
    void update_UI_config();

    void showEvent(QShowEvent *event);

signals:
    void can_send(QCanBusFrame frame);
    void isotp_new_frame(QCanBusFrame data);
};

#endif // CONFIG_H
