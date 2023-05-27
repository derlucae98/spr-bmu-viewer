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

private:
    Ui::Config *ui;

    struct config_t {
        bool globalBalancingEnable;
        quint16 balancingThreshold;
        bool automaticSocLookupEnable;
        quint8 numberOfStacks;
        bool loggerEnable;
        bool loggerDeleteOldestFile;
        bool autoResetOnPowerCycleEnable;
    };

    enum {
        ID_LOAD_DEFAULT_CONFIG = 0,
        ID_GLOBAL_BALANCING_ENABLE,
        ID_BALANCING_THRESHOLD,
        ID_BALANCING_FEEDBACK,
        ID_SOC_LOOKUP,
        ID_AUTOMATIC_SOC_LOOKUP_ENABLE,
        ID_NUMBER_OF_STACKS,
        ID_LOGGER_ENABLE,
        ID_LOGGER_DELETE_OLDEST_FILE,
        ID_AUTORESET_ENABLE,
        ID_SET_GET_RTC,
        ID_CONTROL_CALIBRATION,
        ID_CALIBRATION_STATE,
        ID_CALIBRATION_VALUE,
        ID_FORMAT_SD_CARD,
        ID_SAVE_NV,
        ID_FORMAT_SD_CARD_STATUS
    };

    enum {
        CAN_ID_CAL_REQUEST  = 0x010,
        CAN_ID_CAL_RESPONSE = 0x011
    };

    static constexpr quint8 CARD_FORMATTING_FINISHED = 0x10;
    static constexpr quint8 CARD_FORMATTING_BUSY = 0x11;
    static constexpr quint8 ERROR_CARD_FORMATTING_FAILED = 0x12;
    static constexpr quint8 ERROR_NO_CARD = 0x13;

    QTimer *pollTimer = nullptr;

    void handle_format_sd_response(QCanBusFrame &frame);
    void poll_timer_callback();

signals:
    void can_send(QCanBusFrame frame);
};

#endif // CONFIG_H
