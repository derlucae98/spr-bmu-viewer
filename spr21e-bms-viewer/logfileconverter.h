#ifndef LOGFILECONVERTER_H
#define LOGFILECONVERTER_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QByteArray>
#include <QMessageBox>

namespace Ui {
class LogfileConverter;
}

class LogfileConverter : public QMainWindow
{
    Q_OBJECT

public:
    explicit LogfileConverter(QWidget *parent = nullptr);
    ~LogfileConverter();

private:
    typedef struct {
        uint8_t day;
        uint8_t month;
        uint16_t year;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    } rtc_date_time_t;

    typedef enum {
        STATE_STANDBY,
        STATE_PRE_CHARGE,
        STATE_OPERATE,
        STATE_ERROR
    } state_t;

    typedef enum {
        ERROR_NO_ERROR,
        ERROR_SYSTEM_NOT_HEALTHY,
        ERROR_CONTACTOR_IMPLAUSIBLE,
        ERROR_PRE_CHARGE_TOO_SHORT,
        ERROR_PRE_CHARGE_TIMEOUT
    } contactor_error_t;


    struct __attribute__((packed)) logging_data_t {
        rtc_date_time_t timestamp;
        uint16_t cellVoltage[12][12];
        uint16_t temperature[12][14];
        float current;
        float batteryVoltage;
        float dcLinkVoltage;
        uint16_t minCellVolt;
        uint16_t maxCellVolt;
        uint16_t avgCellVolt;
        uint16_t minTemperature;
        uint16_t maxTemperature;
        uint16_t avgTemperature;
        uint8_t stateMachineError;
        uint8_t stateMachineState;
        uint16_t minSoc;
        uint16_t maxSoc;
    };

    logging_data_t decode_line(QByteArray &inputLine);
    QString raw_to_csv(logging_data_t &raw);
    QString get_header();



private slots:

    void on_btnInputFile_clicked();

    void on_btnOutputFile_clicked();

    void on_btnConvert_clicked();

private:
    Ui::LogfileConverter *ui;
};

#endif // LOGFILECONVERTER_H
