#include "can.h"

const QString Can::serverName = "spr_bms_viewer_helper";

Can::Can(QObject *parent) : QObject(parent)
{

}

Can::~Can()
{
    if (server) {
        disconnect_device();
        QLocalServer::removeServer(serverName);
    }
}

void Can::init()
{
#ifdef Q_OS_LINUX
    if (geteuid()) {
        QMessageBox mb;
        mb.setText("Some parts of this software require root privileges! Click OK and enter your password or run as root.");
        mb.exec();
    }

    timeout = new QTimer(this);
    timeout->setInterval(100);
    QObject::connect(timeout, &QTimer::timeout, this, &Can::send_heartbeat);
    timeout->start();
    server = new QLocalServer();
    server->setSocketOptions(QLocalServer::WorldAccessOption);
    QObject::connect(server, &QLocalServer::newConnection, this, &Can::new_client_connected);
    if(!server->listen(serverName)) {
        qDebug() << "Could not start server!";
    }

    QProcess *process = new QProcess();
    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if ((exitCode == 127) || (exitCode == 8)) {
            exit(EXIT_FAILURE);
        }
        process->deleteLater();
    });

    QString path = QDir::currentPath() + /* "/../../" + "bms-viewer-helper/build*/ "/bms-viewer-helper";
    process->start("pkexec", QStringList({path}));
#elif defined Q_OS_WINDOWS
    get_devices();
#endif
}

void Can::get_devices()
{
    QString errorString;
    const QList<QCanBusDeviceInfo> devices = QCanBus::instance()->availableDevices(
        QStringLiteral("peakcan"), &errorString);
    if (!errorString.isEmpty()){
        qDebug() << errorString;
    }
    QStringList names;
    for (const auto &it : devices) {
#ifdef Q_OS_LINUX
        QString channelNumber;
        //peakcan plugin returns device names in format usb0, usb1, ...,
        //socketcan needs the names in format can0, can1, ...
        //NOTE: QCanBusDeviceInfo::channel will fail, if you use a dual channel pcan and a single channel pcan at the same time!
        //Device name is the safer solution here.
        channelNumber = it.name().at(3);
        names.append(QString("can%1").arg(channelNumber));
#elif defined Q_OS_WINDOWS
        names.append(it.name());
#endif
    }
    emit available_devices(names);
}

void Can::send_heartbeat()
{
#ifdef Q_OS_LINUX
    if (socket) {
        socket->write("heartbeat");
    }
#else
    qDebug() << "Can::send_heartbeat() not available on Windows!";
#endif
}

void Can::connect_device()
{
#ifdef Q_OS_LINUX
    if (socket) {
        socket->write(QString("pcan up %1").arg(deviceName).toLocal8Bit());
    }
#else
    if (connect_socket()) {
        emit device_up();
    }
#endif
}

void Can::disconnect_device()
{
    if (can_device) {
        can_device->disconnectDevice();
    }
#ifdef Q_OS_LINUX
    if (socket) {
        socket->write(QString("pcan down %1").arg(deviceName).toLocal8Bit());
    }
#elif defined Q_OS_WINDOWS
    emit device_down();
#endif
}

void Can::send_frame(QCanBusFrame frame)
{
    if (can_device) {
        can_device->clear();
        can_device->writeFrame(frame);
    }
}

void Can::set_device_name(QString deviceName)
{
    this->deviceName = deviceName;
}

bool Can::connect_socket()
{
    QString errorString;
#ifdef Q_OS_LINUX
    can_device = QCanBus::instance()->createDevice(
        QStringLiteral("socketcan"), QString("%1").arg(deviceName), &errorString);
#elif defined Q_OS_WINDOWS
    can_device = QCanBus::instance()->createDevice(
        QStringLiteral("peakcan"), QString("%1").arg(deviceName), &errorString);
#endif
    can_device->setConfigurationParameter(QCanBusDevice::BitRateKey, 1000000);
    if (!can_device) {
        qDebug("Can device init failed");
        return false;
    }
    if (!can_device->connectDevice()) {
        qDebug("Can device init failed");
        return false;
    }
    QObject::connect(can_device, &QCanBusDevice::framesReceived, this, &Can::get_frame);
    qDebug("Pcan init successful");
    return true;
}

void Can::new_client_connected()
{
#ifdef Q_OS_LINUX
    socket = server->nextPendingConnection();
    QObject::connect(socket, &QLocalSocket::readyRead, this, &Can::message_from_client);
#else
    qDebug() << "Can::new_client_connected() not available on Windows!";
#endif
}

void Can::message_from_client()
{
#ifdef Q_OS_LINUX
    QByteArray raw = socket->readAll();
    QString data(raw);
    qDebug() << data;

    if (data.compare("server running") == 0) {
        qDebug() << "Server running!";
        socket->write("pcan init driver");

    } else if (data.compare("pcan init driver success") == 0) {
        qDebug() << "Driver loaded!";
        get_devices();

    } else if (data.compare("pcan init driver failed") == 0) {
        emit error("Could not load pcan driver!");
        exit(EXIT_FAILURE);

    } else if (data.compare("pcan up success") == 0) {
        if (connect_socket()) {
            emit device_up();
        } else {
            emit error("Could not connect to can socket!");
        }

    } else if (data.compare("pcan up failed") == 0) {
        emit error("Could not bring can interface up!");


    } else if (data.compare("pcan down success") == 0) {
        emit device_down();
    } else if (data.compare("pcan down failed") == 0) {
        emit error("Could not bring can interface down!");
    }
#else
    qDebug() << "Can::message_from_client() not available on Windows!";
#endif
}

void Can::get_frame()
{
    while(can_device->framesAvailable()) {
        emit new_frame(can_device->readFrame());
    }
}
