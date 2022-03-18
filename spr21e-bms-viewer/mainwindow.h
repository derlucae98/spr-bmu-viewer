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
#include "diagdialog.h"
#include "logfileconverter.h"


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

    void on_reqTsActive_stateChanged(int arg1);

    void on_clearErrorLog_clicked();

    void on_diagButton_clicked();

    void on_actionLogfile_converter_triggered();

private:
    enum LTCError_t{
        NOERROR         = 0x0, //!< NOERROR
        PECERROR        = 0x1, //!< PECERROR
        VALUEOUTOFRANGE = 0x2, //!< VALUEOUTOFRANGE
        OPENCELLWIRE    = 0x3, //!< OPENCELLWIRE
    };

    enum canIds {
        ID_BMS_INFO_1  = 0x1,
        ID_BMS_INFO_2  = 0x2,
        ID_BMS_INFO_3  = 0x3,
        ID_CELL_TEMP_1 = 0x4,
        ID_CELL_TEMP_2 = 0x5,
        ID_CELL_TEMP_3 = 0x6,
        ID_CELL_VOLT_1 = 0x7,
        ID_CELL_VOLT_2 = 0x8,
        ID_CELL_VOLT_3 = 0x9,
        ID_CELL_VOLT_4 = 0xA,
        ID_UID         = 0xB,
        ID_DIAG_REQUEST = 0xC,
        ID_DIAG_RESPONSE = 0xD
    };

    enum ts_state_t {
        TS_STATE_STANDBY,
        TS_STATE_PRE_CHARGING,
        TS_STATE_OPERATE,
        TS_STATE_ERROR
    };

    enum error_code_t {
        ERROR_NO_ERROR,
        ERROR_SYSTEM_NOT_HEALTHY,
        ERROR_CONTACTOR_IMPLAUSIBLE,
        ERROR_PRE_CHARGE_TOO_SHORT,
        ERROR_PRE_CHARGE_TIMEOUT
    };

    struct bms_info_t {
        float minCellVolt;
        bool minCellVoltValid;
        float maxCellVolt;
        bool maxCellVoltValid;
        float avgCellVolt;
        bool avgCellVoltValid;
        float minSoc;
        bool minSocValid;
        float maxSoc;
        bool maxSocValid;
        float batteryVoltage;
        bool batteryVoltageValid;
        float dcLinkVoltage;
        bool dcLinkVoltageValid;
        float current;
        bool currentValid;
        float isoRes;
        bool isoResValid;
        bool imdStatus;
        bool imdScStatus;
        bool amsStatus;
        bool amsScStatus;
        ts_state_t tsState;
        bool shutdownStatus;
        error_code_t error;
        float minTemp;
        bool minTempValid;
        float maxTemp;
        bool maxTempValid;
        float avgTemp;
        bool avgTempValid;
    };

    QTimer *updateTimer = nullptr;
    Ui::MainWindow *ui;

    void decomposeCellVoltage(quint8 stack, quint8 cellOffset, QByteArray payload);
    void decomposeCellTemperatures(quint8 stack, quint8 offset, QByteArray payload);
    void decomposeUid(quint8 stack, QByteArray payload);
    void decompose_bms_1(QByteArray payload);
    void decompose_bms_2(QByteArray payload);
    void decompose_bms_3(QByteArray payload);
    void decompose_balance(QCanBusFrame &frame);
    QString ts_state_to_string(ts_state_t state);
    QString returnValidity(quint8 val);

    quint16 cellVoltages[12][12];
    quint8 cellVoltageValidity[12][13];
    float temperatures[12][14];
    quint8 temperatureValidity[12][14];
    quint32 uid[12];
    bool balanceStatus[12][12];

    void setUID(QVector<quint32> uid);

    void updateCellVoltagePeriodic();
    void calculateMedian(quint16 cellVoltages[12][12], double *cellVoltageAvg);

    bool interfaceUp;

    void new_frame(QCanBusFrame frame);

    Can *can = nullptr;

    bms_info_t bmsInfo;

    QTimer *sendTimer = nullptr;
    void send_ts_req();

    QString error_to_string(error_code_t error);

    bool linkAvailable;

    void poll_balance_status();
    void handle_diag_response(QCanBusFrame &frame);
    void update_ui_balancing();


    DiagDialog *diagDialog = nullptr;
    void global_balancing_enable(bool enable);

    void closeEvent(QCloseEvent *event);

signals:


};
#endif // MAINWINDOW_H
