#include "gateway.h"

Gateway::Gateway(QObject *parent)
    : QObject{parent}
{
    ch1_socket = new QTcpSocket(this);
    QObject::connect(ch1_socket, &QTcpSocket::readyRead, this, &Gateway::ch1_read_frame);
    QObject::connect(ch1_socket, &QTcpSocket::stateChanged, this, &Gateway::ch1_state_changed);

    ch2_socket = new QTcpSocket(this);
    QObject::connect(ch2_socket, &QTcpSocket::readyRead, this, &Gateway::ch2_read_frame);
    QObject::connect(ch2_socket, &QTcpSocket::stateChanged, this, &Gateway::ch2_state_changed);
}

void Gateway::connect_device(QUrl ch1, QUrl ch2)
{
    ch1_socket->connectToHost(ch1.host(), ch1.port());
    ch2_socket->connectToHost(ch2.host(), ch2.port());
}

void Gateway::disconnect_device()
{
    ch1_socket->disconnectFromHost();
    ch2_socket->disconnectFromHost();
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
    ret.append("\n");
    ret.append(ch2);
    return ret;
}

void Gateway::ch1_send_frame(QCanBusFrame frame)
{
    QByteArray payload;
    payload.resize(13);
    ::memset(payload.data(), 0, payload.length());

    payload[0] = frame.payload().length() & 0x0F;
    payload[1] = frame.frameId() & 0xFF;
    payload[2] = frame.frameId() >> 8;
    payload[3] = frame.frameId() >> 16;
    payload[4] = frame.frameId() >> 24;
    for (int i = 0; i < frame.payload().length(); i++) {
        payload[i + 5] = frame.payload().at(i);
    }
    if (ch1_socket && ch1_socket->isOpen()) {
        ch1_socket->write(payload);
    }
}

void Gateway::ch2_send_frame(QCanBusFrame frame)
{
    QByteArray payload;
    payload.resize(13);
    ::memset(payload.data(), 0, payload.length());

    payload[0] = frame.payload().length() & 0x0F;
    payload[1] = frame.frameId() & 0xFF;
    payload[2] = frame.frameId() >> 8;
    payload[3] = frame.frameId() >> 16;
    payload[4] = frame.frameId() >> 24;
    for (int i = 0; i < frame.payload().length(); i++) {
        payload[i + 5] = frame.payload().at(i);
    }
    if (ch2_socket && ch2_socket->isOpen()) {
        ch2_socket->write(payload);
    }
}

void Gateway::ch1_read_frame()
{
    /*  The Gateway packs up to 50 CAN frames into one TCP/IP frame
     *  Each transmitted CAN frame is 13 bytes long
     *  We store the incomming data in a buffer and check if the buffers' length
     *  is divisible without remainder. If this is not the case, we skip the current
     *  cycle and wait for the rest of the incoming data.
     *  Otherwise we break the buffer down into 13 byte long parts and parse the incoming data
     *  with convert_to_can(), which creates a QCanBusFrame
     */

    static QByteArray buffer;
    while (ch1_socket->bytesAvailable()) {
        buffer.append(ch1_socket->readAll());
        if ((buffer.length() % 13) != 0) {
            return;
        } else {
            for (int i = 0; i < buffer.length(); i += 13) {
                QByteArray buf = buffer.mid(i, 13);
                QCanBusFrame frame = convert_to_can(buf);
                if (frame.frameType() != QCanBusFrame::InvalidFrame) {
                    emit ch1_new_frame(frame);
                }
            }
            buffer.clear();
        }
    }
}

void Gateway::ch2_read_frame()
{
    /*  The Gateway packs up to 50 CAN frames into one TCP/IP frame
     *  Each transmitted CAN frame is 13 bytes long
     *  We store the incomming data in a buffer and check if the buffers' length
     *  is divisible without remainder. If this is not the case, we skip the current
     *  cycle and wait for the rest of the incoming data.
     *  Otherwise we break the buffer down into 13 byte long parts and parse the incoming data
     *  with convert_to_can(), which creates a QCanBusFrame
     */

    static QByteArray buffer;
    while (ch2_socket->bytesAvailable()) {
        buffer.append(ch2_socket->readAll());
        if ((buffer.length() % 13) != 0) {
            return;
        } else {
            for (int i = 0; i < buffer.length(); i+=13) {
                QByteArray buf = buffer.mid(i, 13);
                QCanBusFrame frame = convert_to_can(buf);
                if (frame.frameType() != QCanBusFrame::InvalidFrame) {
                    emit ch2_new_frame(frame);
                }
                //qDebug() << Qt::hex << frame.frameId() << "#" << frame.payload().toHex();
            }
            buffer.clear();
        }
    }
}

QCanBusFrame Gateway::convert_to_can(QByteArray &data)
{
    // See User Manual for protocol specification
    QCanBusFrame frame;

    /* DLC is the lower nibble of the first byte
     * Frame identifier (Standard, Extended) and RTR frames are ignored
     */
    quint8 len = data.at(0) & 0xF;

    // Sanity check: In case of implausible length, return invalid frame
    if (data.length() != 13 || len > 8) {
        frame.setFrameType(QCanBusFrame::InvalidFrame);
        return frame;
    }

    /* Frame ID is composed of the following four bytes.
     * Endianess is contrary to the definition in the manual!
     */
    quint32 id = (data.at(4) << 24) | (data.at(3) << 16) | (data.at(2) << 8) | (data.at(1));

    frame.setFrameId(id);
    QByteArray payload;

    for (int i = 0; i < len; i++) {
        payload.append(data.at(i + 5));
    }

    frame.setPayload(payload);

    return frame;
}
