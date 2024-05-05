#ifndef GATEWAY_H
#define GATEWAY_H
/* ECAN-W01 CAN-WIFI-Gateway
 * Gateway operates as TCP Server
 * Connect both channels
 * This module uses QCanBusFrames to exchange data
 */

#include <QObject>
#include <QTcpSocket>
#include <QCanBusFrame>
#include <QUrl>

class Gateway : public QObject
{
    Q_OBJECT
public:
    explicit Gateway(QObject *parent = nullptr);
    void connect_device(QUrl ch1, QUrl ch2);
    void disconnect_device();
    QString errorString();
private:
    QTcpSocket *ch1_socket = nullptr;
    QTcpSocket *ch2_socket = nullptr;
    void ch1_read_frame();
    void ch2_read_frame();
    QCanBusFrame convert_to_can(QByteArray &data);

signals:
    void new_frame(quint8 channel, QCanBusFrame frame);
    void state_changed(quint8 channel, QAbstractSocket::SocketState);
};

#endif // GATEWAY_H
