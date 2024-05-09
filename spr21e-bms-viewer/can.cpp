#include "can.h"

const QString Can::serverName = "spr_bms_viewer_helper";

Can::Can(QObject *parent) : QObject(parent)
{

}

Can::~Can()
{

}

void Can::init()
{
    get_devices();
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

void Can::connect_device()
{
#ifdef Q_OS_LINUX
    QProcess *process = new QProcess();

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            qDebug() << "pcan up success";
            emit device_up();
        } else {
            qDebug() << "pcan up failed";
        }
        process->deleteLater();
    });

    process->start("ip", QStringList({"link", "set", this->deviceName, "up", "type", "can", "bitrate", QString::number(1000000UL), "restart-ms", "100"})); //Requires root
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
    QProcess *process = new QProcess();

    QObject::connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus)
        if (exitCode == 0) {
            qDebug() << "pcan down success";
            emit device_down();
        } else {
            qDebug() << "pcan down failed";
        }
        process->deleteLater();
    });

    process->start("ip", QStringList({"link", "set", this->deviceName, "down"})); //Requires root
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

void Can::get_frame()
{
    while(can_device->framesAvailable()) {
        emit new_frame(can_device->readFrame());
    }
}
