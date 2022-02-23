#ifndef HELPER_H
#define HELPER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QTimer>

class Helper : public QObject
{
    Q_OBJECT
public:
    explicit Helper(QObject *parent = nullptr);
    ~Helper();

    void setup_server();

private:

    QLocalSocket *socket = nullptr;
    void ready_read();
    void init_driver();
    void bring_pcan_up(QString pcan);
    void bring_pcan_down(QString pcan);
    void send_response(QString resp);
    bool killRequest;
    QTimer *timeout = nullptr;
    void timeout_reached();
    QString canDevice;
signals:

};

#endif // HELPER_H
