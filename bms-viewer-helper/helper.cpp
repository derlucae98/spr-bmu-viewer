#include "helper.h"

const QString Helper::bitrate = "1000000";

Helper::Helper(QObject *parent) : QObject(parent)
{
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
    // Connect to the IPC server opened by the main software
    socket = new QLocalSocket();
    QObject::connect(socket, &QLocalSocket::readyRead, this, &Helper::ready_read);
    QObject::connect(socket, &QLocalSocket::errorOccurred, this, &Helper::handle_error);
    socket->connectToServer("spr_bms_viewer_helper", QLocalSocket::ReadWrite);
    socket->write("server running");
    qDebug() << "Server is running!";
    timeout->start();
}

void Helper::ready_read()
{
    if (socket == nullptr) {
        return;
    }

    QByteArray raw = socket->readAll();
    QString data(raw);

    timeout->start(); //Restart the timer

    if (data == "pcan init driver") {
        qDebug() << "Loading PCAN driver...";
        load_driver();
    } else if (data.mid(0, 7) == "pcan up") {
        QString pcan = data.mid(8, 4); //canx
        qDebug() << "Bringing " << pcan << "up...";
        canDevice = pcan;
        bring_pcan_up();
    } else if (data.mid(0, 9) == "pcan down") {
        QString pcan = data.mid(10, 4); //canx
        qDebug() << "Bringing device down...";
        canDevice = pcan; //TODO: Could be removed, can device should not change in one session
        bring_pcan_down();
    } else if (data.mid(0, 4) == "kill") {
        QString pcan = data.mid(5, 4); //canx
        canDevice = pcan; //TODO: Could be removed, can device should not change in one session
        bring_pcan_down();
        exit(EXIT_SUCCESS);
    } else if (data == "heartbeat") {
        qDebug() << "heartbeat";
        timeout->start();
    }
}

void Helper::load_driver()
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
    initPcan->start("modprobe", QStringList({"pcan"})); //Requires root
}

void Helper::bring_pcan_up()
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

    process->start("ip", QStringList({"link", "set", canDevice, "up", "type", "can", "bitrate", bitrate, "restart-ms", "100"})); //Requires root
}

void Helper::bring_pcan_down()
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

    process->start("ip", QStringList({"link", "set", canDevice, "down"})); //Requires root
}

void Helper::send_response(QString resp)
{
    if (socket) {
        socket->write(resp.toLocal8Bit());
    }
}

void Helper::handle_error(QLocalSocket::LocalSocketError err)
{
    Q_UNUSED(err)
    qDebug() << socket->errorString();
    exit(EXIT_SUCCESS);
}

void Helper::timeout_reached()
{
    bring_pcan_down();
    exit(EXIT_SUCCESS);
}
