#include "helper.h"

Helper::Helper(QObject *parent) : QObject(parent)
{
    killRequest = false;
    timeout = new QTimer(this);
    timeout->setInterval(2000);
    QObject::connect(timeout, &QTimer::timeout, this, &Helper::timeout_reached);
    setup_server();
}

Helper::~Helper()
{

}

void Helper::setup_server()
{
    socket = new QLocalSocket();
    QObject::connect(socket, &QLocalSocket::readyRead, this, &Helper::ready_read);
    QObject::connect(socket, &QLocalSocket::errorOccurred, this, [=](QLocalSocket::LocalSocketError err) {
        (void) err;
        exit(EXIT_FAILURE);
    });
    socket->connectToServer("spr_bms_viewer_helper", QLocalSocket::ReadWrite);
    socket->write("server running");
    qDebug() << "Running!";
}

void Helper::ready_read()
{
    if (socket) {
        QByteArray raw = socket->readAll();
        QString data(raw);
        if (data == "pcan init driver") {
            qDebug() << "Driver init";
            init_driver();
            timeout->start();
        } else if (data.mid(0, 7) == "pcan up") {
            QString pcan = data.mid(8, 4); //canx
            bring_pcan_up(pcan);
            canDevice = pcan;
        } else if (data.mid(0, 9) == "pcan down") {
            QString pcan = data.mid(10, 4); //canx
            qDebug() << "Bringing device down...";
            bring_pcan_down(pcan);
            if (killRequest) {
                qDebug() << "Selfdestroy";
                exit(EXIT_SUCCESS);
            }
        } else if (data.mid(0, 4) == "kill") {
            QString pcan = data.mid(5, 4); //canx
            bring_pcan_down(pcan);
            exit(EXIT_SUCCESS);
        } else if (data == "heartbeat") {
            qDebug() << "heartbeat";
            timeout->start();
        }
    }
}

void Helper::init_driver()
{
    QProcess *initPcan = new QProcess(this);

    QObject::connect(initPcan, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            send_response("pcan init driver success");
        } else {
            send_response("pcan init driver failed");
        }
        initPcan->deleteLater();
    });
    initPcan->start("modprobe", QStringList({"pcan"}));
}

void Helper::bring_pcan_up(QString pcan)
{
    QProcess *process = new QProcess();
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            send_response("pcan up success");
        } else {
            send_response("pcan up failed");
        }
        process->deleteLater();
    });

    process->start("ip", QStringList({"link", "set", pcan, "up", "type", "can", "bitrate", "1000000", "restart-ms", "100"}));
}

void Helper::bring_pcan_down(QString pcan)
{
    QProcess *process = new QProcess();
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            send_response("pcan down success");
        } else {
            send_response("pcan down failed");
        }
        process->deleteLater();
    });

    process->start("ip", QStringList({"link", "set", pcan, "down"}));
}

void Helper::send_response(QString resp)
{
    if (socket) {
        socket->write(resp.toLocal8Bit());
    }
}

void Helper::timeout_reached()
{
    bring_pcan_down(canDevice);
    exit(EXIT_SUCCESS);
}
