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

    static const QString bitrate;
    QLocalSocket *socket = nullptr;
    void ready_read();
    void load_driver();
    void bring_pcan_up();
    void bring_pcan_down();
    void send_response(QString resp);
    void handle_error(QLocalSocket::LocalSocketError err);
    QTimer *timeout = nullptr;
    void timeout_reached();
    QString canDevice;
signals:

};

#endif // HELPER_H
