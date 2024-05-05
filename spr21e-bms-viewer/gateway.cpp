#include "gateway.h"

Gateway::Gateway(QObject *parent)
    : QObject{parent}
{
    ch1_socket = new QTcpSocket(this);
    QObject::connect(ch1_socket, &QTcpSocket::readyRead, this, &Gateway::ch1_read_frame);
    QObject::connect(ch1_socket, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState state) {
        emit state_changed(1, state);
    });

    ch2_socket = new QTcpSocket(this);
    QObject::connect(ch2_socket, &QTcpSocket::readyRead, this, &Gateway::ch2_read_frame);
    QObject::connect(ch2_socket, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState state) {
        emit state_changed(2, state);
    });
}

void Gateway::connect_device(QUrl ch1, QUrl ch2)
{
    ch1_socket->connectToHost(ch1.host(), ch1.port());
    ch2_socket->connectToHost(ch2.host(), ch2.port());
}

QString Gateway::errorString()
{
    QString ch1;
    QString ch2;
    if (ch1_socket) {
        ch1 = "Channel 1: " + ch1_socket->errorString();
    }

    if (ch2_socket) {
        ch2 = "Channel 2: " + ch2_socket->errorString();
    }
    QString ret;
    ret.append(ch1);
    ret.append(ch2);
    return ret;
}

void Gateway::ch1_read_frame()
{
    static QByteArray buffer;
    while (ch1_socket->bytesAvailable()) {
        buffer.append(ch1_socket->readAll());
        if ((buffer.length() % 13) != 0) {
            return;
        } else {
            for (int i = 0; i < buffer.length(); i+=13) {
                QByteArray buf = buffer.mid(i, 13);
                QCanBusFrame frame = convert_to_can(buf);
                //qDebug() << "New frame! " << Qt::hex << frame.frameId() << "#" << frame.payload();
                emit new_frame(1, frame);

            }
            buffer.clear();
        }
    }
}

void Gateway::ch2_read_frame()
{
    static QByteArray buffer;
    while (ch2_socket->bytesAvailable()) {
        buffer.append(ch2_socket->readAll());
        if ((buffer.length() % 13) != 0) {
            return;
        } else {
            for (int i = 0; i < buffer.length(); i+=13) {
                QByteArray buf = buffer.mid(i, 13);
                QCanBusFrame frame = convert_to_can(buf);
                //qDebug() << "New frame! " << Qt::hex << frame.frameId() << "#" << frame.payload();
                emit new_frame(2, frame);
            }
            buffer.clear();
        }
    }
}

QCanBusFrame Gateway::convert_to_can(QByteArray &data)
{
    QCanBusFrame frame;
    quint8 len = data.at(0) & 0xF;
    quint32 id = (data.at(4) << 24) | (data.at(3) << 16) | (data.at(2) << 8) | (data.at(1));
    frame.setFrameId(id);
    QByteArray payload;
    for (int i = 0; i < len; i++) {
        payload.append(data.at(i + 5));
    }
    frame.setPayload(payload);
    return frame;
}
