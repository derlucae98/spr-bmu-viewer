#ifndef CAN_H
#define CAN_H

#include <QObject>
#include <QCanBus>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDir>
#include <QProcess>
#include <QTimer>


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

    QTimer *timeout = nullptr;
    QCanBusDevice *can_device = nullptr;
    bool connect_socket();
    QLocalServer *server = nullptr;
    QLocalSocket *socket = nullptr;
    void socket_received();
    void new_client_connected();
    void message_from_client();
    QString deviceName;
    void get_frame();
    void get_devices();
    void send_heartbeat();

signals:
    void new_frame(QCanBusFrame);
    void error(QString);
    void device_up();
    void device_down();
    void available_devices(QStringList);
};

#endif // CAN_H
