#ifndef CAN_H
#define CAN_H

#include <QObject>
#include <QCanBus>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <unistd.h>
#include <QMessageBox>


class Can : public QObject
{
    Q_OBJECT
public:
    explicit Can(QObject *parent = nullptr);
    ~Can();
    void init();

    void connect_device();
    void disconnect_device();
    void send_frame(QCanBusFrame frame);
    void set_device_name(QString deviceName);


private:

    static const QString serverName;
    QTimer *timeout = nullptr;
    QCanBusDevice *can_device = nullptr;

    QString deviceName;
    void get_frame();
    void get_devices();

signals:
    void new_frame(QCanBusFrame);
    void error(QString);
    void device_up();
    void device_down();
    void available_devices(QStringList);
};

#endif // CAN_H
