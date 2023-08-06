#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (geteuid()) {
        QMessageBox mb;
        mb.setText("Some parts of this software require root privileges! Click OK and enter your password or run as root.");
        mb.exec();
    }

    updateTimer = new QTimer();
    updateTimer->setInterval(150);
    QObject::connect(updateTimer, &QTimer::timeout, this, &MainWindow::update_ui_periodic);


    interfaceUp = false;
    can = new Can(this);
    QObject::connect(can, &Can::error, this, [=](QString err) {
        QMessageBox mb;
        mb.setText(err);
        mb.exec();
    });
    QObject::connect(can, &Can::device_up, this, [=] {
        interfaceUp = true;
        ui->infoFrame->setEnabled(true);
        ui->parameters->setEnabled(true);
        ui->btnConnectPcan->setText("Disconnect");
        ui->cbSelectPCAN->setEnabled(false);
        updateTimer->start();
    });
    QObject::connect(can, &Can::device_down, this, [=] {
        interfaceUp = false;
        ui->infoFrame->setEnabled(false);
        ui->parameters->setEnabled(false);
        ui->btnConnectPcan->setText("Connect");
        ui->cbSelectPCAN->setEnabled(true);
        updateTimer->stop();
    });
    QObject::connect(can, &Can::new_frame, this, &MainWindow::new_frame);
    QObject::connect(can, &Can::available_devices, this, [=] (QStringList names) {
        ui->cbSelectPCAN->addItems(names);
        this->show();});
    can->init();

    sendTimer = new QTimer(this);
    sendTimer->setInterval(100);
    QObject::connect(sendTimer, &QTimer::timeout, this, &MainWindow::send_ts_req);

    ui->infoFrame->setEnabled(false);
    ui->parameters->setEnabled(false);

    linkAvailable = false;
    QPixmap scuderiaLogo(":/img/logo.png");

    ui->scuderiaLogo->setScaledContents(true);
    ui->scuderiaLogo->setPixmap(scuderiaLogo.scaled(2*38, 2*22, Qt::KeepAspectRatio));

    QColor bgColor = ui->parameters->palette().color(QWidget::backgroundRole());
    if (bgColor.lightness() < 127) {
        darkMode = true;
    } else {
        darkMode = false;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::new_frame(QCanBusFrame frame)
{
    static uint16_t fullUpdate = 0;

    switch (frame.frameId()) {
    case CAN_ID_INFO:
        decompose_info(frame.payload());
        fullUpdate |= (1 << 0);
        break;
    case CAN_ID_STATS_1:
        decompose_stats_1(frame.payload());
        fullUpdate |= (1 << 1);
        break;
    case CAN_ID_STATS_2:
        decompose_stats_2(frame.payload());
        fullUpdate |= (1 << 2);
        break;
    case CAN_ID_UIP:
        decompose_uip(frame.payload());
        fullUpdate |= (1 << 3);
        break;
    case CAN_ID_CELL_VOLTAGE_1:
        decompose_cell_voltage_1(frame.payload());
        fullUpdate |= (1 << 4);
        break;
    case CAN_ID_CELL_VOLTAGE_2:
        decompose_cell_voltage_2(frame.payload());
        fullUpdate |= (1 << 5);
        break;
    case CAN_ID_CELL_VOLTAGE_3:
        decompose_cell_voltage_3(frame.payload());
        fullUpdate |= (1 << 6);
        break;
    case CAN_ID_CELL_VOLTAGE_4:
        decompose_cell_voltage_4(frame.payload());
        fullUpdate |= (1 << 7);
        break;
    case CAN_ID_CELL_TEMPERATURE:
        decompose_cell_temperature(frame.payload());
        fullUpdate |= (1 << 8);
        break;
    case CAN_ID_BALANCING_FEEDBACK:
        decompose_balancing_feedback(frame.payload());
        fullUpdate |= (1 << 9);
        break;
    case CAN_ID_UNIQUE_ID:
        decompose_unique_id(frame.payload());
        fullUpdate |= (1 << 10);
        break;
    }

    if (fullUpdate == 0x7FF) {
        linkAvailable = true;
        fullUpdate = 0;
    }
}

void MainWindow::send_ts_req()
{
    QCanBusFrame frame;
    QByteArray payload;
    frame.setFrameId(0);
    payload.append(0xFF);
    frame.setPayload(payload);
    can->send_frame(frame);
}

QString MainWindow::error_to_string(contactor_error_t error)
{
//    switch (error) {
//    case ERROR_NO_ERROR:
//        return "Error cleared.";
//    case ERROR_SYSTEM_NOT_HEALTHY:
//        return "System not healthy!";
//    case ERROR_CONTACTOR_IMPLAUSIBLE:
//        return "Contactor state implausible!";
//    case ERROR_PRE_CHARGE_TOO_SHORT:
//        return "Pre-charge too short!";
//    case ERROR_PRE_CHARGE_TIMEOUT:
//        return "Pre-charge timed out!";
//    }
//    return "Undefined error.";
}


QString MainWindow::ts_state_to_string(ts_state_t state)
{
    switch (state) {
    case TS_STATE_STANDBY:
        return "Standby";
    case TS_STATE_PRE_CHARGING:
        return "Pre-charging";
    case TS_STATE_OPERATE:
        return "Operate";
    case TS_STATE_ERROR:
        return "Error";
    }
}

QString MainWindow::returnValidity(quint8 val)
{
    switch (val) {
    case NOERROR:
        return "OK";
    case PECERROR:
        return "PEC error";
    case VALUEOUTOFRANGE:
        return "Value out of range";
    case OPENCELLWIRE:
        return "Open wire";
    default:
        return "Unknown error";
    }
}

void MainWindow::update_ui_periodic()
{
    update_ui_voltage();
    update_ui_temperature();
    update_ui_stats();
    update_ui_balancing();
    update_ui_uid();
}

void MainWindow::on_btnConnectPcan_clicked()
{
    if (!interfaceUp) {
        can->set_device_name(ui->cbSelectPCAN->currentText());
        can->connect_device();


    } else {
        can->disconnect_device();

    }
}

void MainWindow::on_reqTsActive_stateChanged(int arg1)
{
    QCanBusFrame frame;
    QByteArray payload;
    frame.setFrameId(0);
    if (arg1 == 0) {
        sendTimer->stop();
        payload.append((quint8)0);
        frame.setPayload(payload);
        can->send_frame(frame);
    } else {
        sendTimer->start();
    }
}

void MainWindow::on_clearErrorLog_clicked()
{
    ui->errorLog->clear();
}

void MainWindow::on_actionLogfile_converter_triggered()
{
    LogfileConverter *logfileConverter = new LogfileConverter();
    logfileConverter->setAttribute(Qt::WA_DeleteOnClose);
    logfileConverter->show();
}

void MainWindow::on_actionAbout_SPR_BMS_viewer_triggered()
{
    AboutDialog *aboutDialog = new AboutDialog();
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void MainWindow::on_actionConfig_triggered()
{
    Config *configDialog = new Config(this);
    QObject::connect(can, &Can::new_frame, configDialog, &Config::can_recv);
    QObject::connect(configDialog, &Config::can_send, can, &Can::send_frame);
    configDialog->setAttribute(Qt::WA_DeleteOnClose);
    configDialog->show();
}

void MainWindow::decompose_info(QByteArray data)
{
    canData.errorCode = ((quint8)data.at(0) << 24) | ((quint8)data.at(1) << 16) | ((quint8)data.at(2) << 8) | ((quint8)data.at(3));
    canData.isolationResistance = ((quint8)data.at(4) << 8) | (quint8)data.at(5);
    canData.isolationResistanceValid = (quint8)data.at(6) >> 7;
    canData.tsState = static_cast<ts_state_t>((quint8)data.at(6) & 0x03);
}

void MainWindow::decompose_stats_1(QByteArray data)
{
    canData.voltageValid = data.at(0) & 0x01;
    canData.minCellVolt = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.0001f;
    canData.maxCellVolt = (((quint8)data.at(3) << 8) | (quint8)data.at(4)) * 0.0001f;
    canData.avgCellVolt = (((quint8)data.at(5) << 8) | (quint8)data.at(6)) * 0.0001f;
}

void MainWindow::decompose_stats_2(QByteArray data)
{
    canData.dcLinkVoltageValid = ((quint8)data.at(0) >> 2) & 0x01;
    canData.socValid = ((quint8)data.at(0) >> 1) & 0x01;
    canData.tempValid = (quint8)data.at(0) & 0x01;
    canData.minTemp = (quint8)data.at(1) * 0.5f;
    canData.maxTemp = (quint8)data.at(2) * 0.5f;
    canData.avgTemp = (quint8)data.at(3) * 0.5f;
    canData.minSoc = (quint8)data.at(4);
    canData.maxSoc = (quint8)data.at(5);
    canData.dcLinkVoltage = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) / 10.0f;
}

void MainWindow::decompose_uip(QByteArray data)
{
    canData.batteryPowerValid = ((quint8)data.at(0) >> 2) & 0x01;
    canData.currentValid = ((quint8)data.at(0) >> 1) & 0x01;
    canData.batteryVoltageValid = ((quint8)data.at(0) >> 0) & 0x01;
    canData.batteryVoltage = (((quint8)data.at(1) << 8) | (quint8)data.at(2)) * 0.1f;
    canData.current = (((quint8)data.at(3) << 8) | (quint8)data.at(4)) * 0.00625f;
    canData.batteryPower = (((quint8)data.at(5) << 8) | (quint8)data.at(6)) * 0.0025f;
}

void MainWindow::decompose_cell_voltage_1(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][0] = (quint8)data.at(0) & 0x01;
    canData.cellVoltageStatus[index][1] = (quint8)data.at(1) & 0x03;
    canData.cellVoltageStatus[index][2] = ((quint8)data.at(1) >> 2) & 0x03;
    canData.cellVoltageStatus[index][3] = ((quint8)data.at(1) >> 4) & 0x03;
    canData.cellVoltage[index][0] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][1] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][2] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void MainWindow::decompose_cell_voltage_2(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][4] = (quint8)data.at(1) & 0x03;
    canData.cellVoltageStatus[index][5] = ((quint8)data.at(1) >> 2) & 0x03;
    canData.cellVoltageStatus[index][6] = ((quint8)data.at(1) >> 4) & 0x03;
    canData.cellVoltage[index][3] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][4] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][5] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void MainWindow::decompose_cell_voltage_3(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][7] = (quint8)data.at(1) & 0x03;
    canData.cellVoltageStatus[index][8] = ((quint8)data.at(1) >> 2) & 0x03;
    canData.cellVoltageStatus[index][9] = ((quint8)data.at(1) >> 4) & 0x03;
    canData.cellVoltage[index][6] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][7] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][8] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void MainWindow::decompose_cell_voltage_4(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.cellVoltageStatus[index][10] = (quint8)data.at(1) & 0x03;
    canData.cellVoltageStatus[index][11] = ((quint8)data.at(1) >> 2) & 0x03;
    canData.cellVoltageStatus[index][12] = ((quint8)data.at(1) >> 4) & 0x03;
    canData.cellVoltage[index][9]  = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canData.cellVoltage[index][10] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canData.cellVoltage[index][11] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void MainWindow::decompose_cell_temperature(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;
    canData.temperatureStatus[index][0] = ((quint8)data.at(1) >> 0) & 0x03;
    canData.temperatureStatus[index][1] = ((quint8)data.at(1) >> 2) & 0x03;
    canData.temperatureStatus[index][2] = ((quint8)data.at(1) >> 4) & 0x03;
    canData.temperatureStatus[index][3] = ((quint8)data.at(1) >> 6) & 0x03;
    canData.temperatureStatus[index][4] = ((quint8)data.at(0) >> 0) & 0x03;
    canData.temperatureStatus[index][5] = ((quint8)data.at(0) >> 2) & 0x03;
    canData.temperature[index][0] = (quint8)data.at(2) * 0.5f;
    canData.temperature[index][1] = (quint8)data.at(3) * 0.5f;
    canData.temperature[index][2] = (quint8)data.at(4) * 0.5f;
    canData.temperature[index][3] = (quint8)data.at(5) * 0.5f;
    canData.temperature[index][4] = (quint8)data.at(6) * 0.5f;
    canData.temperature[index][5] = (quint8)data.at(7) * 0.5f;
}

void MainWindow::decompose_balancing_feedback(QByteArray data)
{
    quint8 index = (quint8)data.at(0) >> 4;

    canData.balance[index][11]  = (data.at(0) >> 3) & 0x01;
    canData.balance[index][10]  = (data.at(0) >> 2) & 0x01;
    canData.balance[index][9]   = (data.at(0) >> 1) & 0x01;
    canData.balance[index][8]   = (data.at(0) >> 0) & 0x01;
    canData.balance[index][7]   = (data.at(1) >> 7) & 0x01;
    canData.balance[index][6]   = (data.at(1) >> 6) & 0x01;
    canData.balance[index][5]   = (data.at(1) >> 5) & 0x01;
    canData.balance[index][4]   = (data.at(1) >> 4) & 0x01;
    canData.balance[index][3]   = (data.at(1) >> 3) & 0x01;
    canData.balance[index][2]   = (data.at(1) >> 2) & 0x01;
    canData.balance[index][1]   = (data.at(1) >> 1) & 0x01;
    canData.balance[index][0]   = (data.at(1) >> 0) & 0x01;
}

void MainWindow::decompose_unique_id(QByteArray data)
{
    quint8 stack = (quint8)(data.at(0) >> 4) & 0x0F;
    canData.UID[stack] = (quint8)data.at(1) << 24 | (quint8)data.at(2) << 16 | (quint8)data.at(3) << 8 | (quint8)data.at(4);
}

void MainWindow::update_ui_balancing()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1); // Voltages
    for (quint16 stack = 0; stack < MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < MAX_NUM_OF_CELLS; cell++) {
            if (canData.balance[stack][cell]) {
                volts->child(stack)->setBackground(cell+2, Qt::darkBlue);
                volts->child(stack)->setForeground(cell+2, Qt::white);

            } else {
                volts->child(stack)->setBackground(cell+2, Qt::transparent);

                if (darkMode) {
                    volts->child(stack)->setForeground(cell+2, Qt::white);
                } else {
                    volts->child(stack)->setForeground(cell+2, Qt::black);
                }
            }
        }
    }
}

void MainWindow::update_ui_uid()
{
    QTreeWidgetItem *uid = ui->parameters->topLevelItem(0);
    for (quint16 stack = 0; stack < MAX_NUM_OF_SLAVES; stack++) {
        uid->child(stack)->setText(1, QString::number(canData.UID[stack], 16).toUpper());
    }
}

void MainWindow::update_ui_voltage()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1);
    QTreeWidgetItem *openWire = ui->parameters->topLevelItem(2); //Open Wires
    for (quint16 stack = 0; stack < MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < MAX_NUM_OF_CELLS; cell++) {
            volts->child(stack)->setText(cell+2, QString::number(canData.cellVoltage[stack][cell], 'f', 4));
            openWire->child(stack)->setText(cell+2, returnValidity(canData.cellVoltageStatus[stack][cell+1]));
        }
        openWire->child(stack)->setText(1, returnValidity(canData.cellVoltageStatus[stack][0]));
    }
    ::memset(canData.cellVoltage, 0, MAX_NUM_OF_SLAVES * MAX_NUM_OF_CELLS);
}

void MainWindow::update_ui_temperature()
{
    QTreeWidgetItem *temps = ui->parameters->topLevelItem(3); //Temperatures
    for (quint16 stack = 0; stack < MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 tempsens = 0; tempsens < MAX_NUM_OF_TEMPSENS; tempsens++) {
            if (canData.temperatureStatus[stack][tempsens] == NOERROR){
                temps->child(stack)->setText(tempsens + 1, QString::number(canData.temperature[stack][tempsens], 'f', 1));
            } else {
                temps->child(stack)->setText(tempsens + 1, returnValidity(canData.temperatureStatus[stack][tempsens]));
            }
        }
    }
    ::memset(canData.temperature, 0, MAX_NUM_OF_SLAVES * MAX_NUM_OF_TEMPSENS);
}

void MainWindow::update_ui_stats()
{
    if (canData.voltageValid) {
        ui->minCellVolt->setText(QString("%1 V").arg(canData.minCellVolt, 5, 'f', 3));
        ui->maxCellVolt->setText(QString("%1 V").arg(canData.maxCellVolt, 5, 'f', 3));
        ui->avgCellVolt->setText(QString("%1 V").arg(canData.avgCellVolt, 5, 'f', 3));
        float delta = canData.maxCellVolt - canData.minCellVolt;
        ui->deltaCellVolt->setText(QString("%1 V").arg(delta, 5, 'f', 3));
    } else {
        ui->minCellVolt->setText("Invalid");
        ui->maxCellVolt->setText("Invalid");
        ui->avgCellVolt->setText("Invalid");
        ui->deltaCellVolt->setText("Invalid");
    }

    if (canData.socValid) {
        ui->minSoc->setText(QString("%1 %").arg(canData.minSoc));
        ui->maxSoc->setText(QString("%1 %").arg(canData.maxSoc));
    } else {
        ui->minSoc->setText("Invalid");
        ui->maxSoc->setText("Invalid");
    }

    if (canData.tempValid) {
        ui->minTemp->setText(QString("%1 °C").arg(canData.minTemp, 4, 'f', 1));
        ui->maxTemp->setText(QString("%1 °C").arg(canData.maxTemp, 4, 'f', 1));
        ui->avgTemp->setText(QString("%1 °C").arg(canData.avgTemp, 4, 'f', 1));
    } else {
        ui->minTemp->setText("Invalid");
        ui->maxTemp->setText("Invalid");
        ui->avgTemp->setText("Invalid");
    }

    if (canData.batteryVoltageValid) {
        ui->batteryVoltage->setText(QString("%1 V").arg(canData.batteryVoltage, 6, 'f', 1));
    } else {
        ui->batteryVoltage->setText("Invalid");
    }

    if (canData.dcLinkVoltageValid) {
        ui->dcLinkVoltage->setText(QString("%1 V").arg(canData.dcLinkVoltage, 6, 'f', 1));
    } else {
        ui->dcLinkVoltage->setText("Invalid");
    }

    if (canData.currentValid) {
        ui->current->setText(QString("%1 A").arg(canData.current, 6, 'f', 2));
    } else {
        ui->current->setText("Invalid");
    }

    if (canData.isolationResistanceValid) {
        ui->isoRes->setText(QString("%1 kΩ").arg(canData.isolationResistance, 6, 'f', 2));
    } else {
        ui->isoRes->setText("Invalid");
    }

    ui->tsState->setText(ts_state_to_string(canData.tsState));

    if (canData.errorCode & ERROR_IMD_FAULT) {
        ui->imdStatus->setText("Error");
    } else {
        ui->imdStatus->setText("OK");
    }

    if (canData.errorCode & ERROR_IMD_POWERSTAGE_DISABLED) {
        ui->imdSC->setText("Error");
    } else {
        ui->imdSC->setText("OK");
    }

    if (canData.errorCode & ERROR_AMS_FAULT) {
        ui->amsStatus->setText("Error");
    } else {
        ui->amsStatus->setText("OK");
    }

    if (canData.errorCode & ERROR_AMS_POWERSTAGE_DISABLED) {
        ui->amsSC->setText("Error");
    } else {
        ui->amsSC->setText("OK");
    }

    if (canData.errorCode & ERROR_SDC_OPEN) {
        ui->scStatus->setText("Error");
    } else {
        ui->scStatus->setText("OK");
    }

    if (linkAvailable) {
        ui->linkDisplay->setStyleSheet("background-color: rgb(0, 255, 0);");
    } else {
        ui->linkDisplay->setStyleSheet("background-color: rgb(255, 0, 0);");
    }
    linkAvailable = false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    (void) event;
    if (can) {
        can->disconnect_device();
    }
}

