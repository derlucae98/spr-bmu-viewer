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


    ::memset(&bmsInfo, 0, sizeof(bms_info_t));
    ::memset(&balanceStatus, 0, sizeof(balanceStatus));

    updateTimer = new QTimer();
    updateTimer->setInterval(150);
    QObject::connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateCellVoltagePeriodic);


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

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::new_frame(QCanBusFrame frame)
{
    static uint16_t fullUpdate = 0;

    switch (frame.frameId()) {
    case ID_BMS_INFO_1:
        decompose_bms_1(frame.payload());
        fullUpdate |= (1 << 0);
        break;
    case ID_BMS_INFO_2:
        decompose_bms_2(frame.payload());
        fullUpdate |= (1 << 1);
        break;
    case ID_BMS_INFO_3:
        decompose_bms_3(frame.payload());
        fullUpdate |= (1 << 2);
        break;
    case ID_CELL_VOLT_1:
        decomposeCellVoltage((quint8)frame.payload().at(0) >> 4, 0, frame.payload());
        fullUpdate |= (1 << 3);
        break;
    case ID_CELL_VOLT_2:
        decomposeCellVoltage((quint8)frame.payload().at(0) >> 4, 3, frame.payload());
        fullUpdate |= (1 << 4);
        break;
    case ID_CELL_VOLT_3:
        decomposeCellVoltage((quint8)frame.payload().at(0) >> 4, 6, frame.payload());
        fullUpdate |= (1 << 5);
        break;
    case ID_CELL_VOLT_4:
        decomposeCellVoltage((quint8)frame.payload().at(0) >> 4, 9, frame.payload());
        fullUpdate |= (1 << 6);
        break;
    case ID_CELL_TEMP_1:
        decomposeCellTemperatures((quint8)frame.payload().at(0) >> 4, 0, frame.payload());
        fullUpdate |= (1 << 7);
        break;
    case ID_CELL_TEMP_2:
        decomposeCellTemperatures((quint8)frame.payload().at(0) >> 4, 5, frame.payload());
        fullUpdate |= (1 << 8);
        break;
    case ID_CELL_TEMP_3:
        decomposeCellTemperatures((quint8)frame.payload().at(0) >> 4, 10, frame.payload());
        fullUpdate |= (1 << 9);
        break;
    case ID_UID:
        decomposeUid((quint8)frame.payload().at(0) >> 4, frame.payload());
        fullUpdate |= (1 << 10);
        break;
    case ID_DIAG_RESPONSE:
        handle_diag_response(frame);
        break;
    case 0x00E:
        //Balancing activity
        decompose_balance(frame);
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

QString MainWindow::error_to_string(error_code_t error)
{
    switch (error) {
    case ERROR_NO_ERROR:
        return "Error cleared.";
    case ERROR_SYSTEM_NOT_HEALTHY:
        return "System not healthy!";
    case ERROR_CONTACTOR_IMPLAUSIBLE:
        return "Contactor state implausible!";
    case ERROR_PRE_CHARGE_TOO_SHORT:
        return "Pre-charge too short!";
    case ERROR_PRE_CHARGE_TIMEOUT:
        return "Pre-charge timed out!";
    }
    return "Undefined error.";
}

void MainWindow::poll_balance_status()
{
    ::memset(balanceStatus, 0, sizeof(balanceStatus));
    QCanBusFrame frame;
    frame.setFrameId(ID_DIAG_REQUEST);
    QByteArray payload;
    payload.append(0x3);
    payload.append((quint8)0);
    payload.append((quint8)0);
    frame.setPayload(payload);
    can->send_frame(frame);
}

void MainWindow::handle_diag_response(QCanBusFrame &frame)
{
    switch (frame.payload().at(0)) {
    case 0x3: //Get balancing
        for (size_t i = 0; i < 8; i++) {
            balanceStatus[0][i] |= (frame.payload().at(3) >> (7 - i)) & 0x01;
        }
        for (size_t i = 0; i < 4; i++) {
            balanceStatus[0][i + 8] |= (frame.payload().at(4) >> (7 - i)) & 0x01;
        }
        update_ui_balancing();
        break;
    }
}

void MainWindow::update_ui_balancing()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1); // Voltages
    for (quint16 stack = 0; stack < 12; stack++) {
        for (quint16 cell = 0; cell < 12; cell++) {
            if (balanceStatus[stack][cell]) {
                volts->child(stack)->setBackground(cell+2, Qt::darkBlue);
                volts->child(stack)->setForeground(cell+2, Qt::white);
            } else {
                volts->child(stack)->setBackground(cell+2, Qt::transparent);
                volts->child(stack)->setForeground(cell+2, Qt::white);
            }
        }
    }
}

void MainWindow::global_balancing_enable(bool enable)
{
    QCanBusFrame frame;
    QByteArray payload;
    frame.setFrameId(ID_DIAG_REQUEST);
    payload.append(0x04);
    payload.append((quint8) 0x00);
    if (enable) {
        payload.append(0x01);
    } else {
        payload.append((quint8) 0x00);
    }
    frame.setPayload(payload);
    can->send_frame(frame);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (can) {
        can->disconnect_device();
    }
}

void MainWindow::decomposeCellVoltage(quint8 stack, quint8 cellOffset, QByteArray payload)
{
    QVector<quint16> voltages(3);
    voltages[0] = ((quint8)payload.at(1) << 5) | ((quint8)payload.at(2) >> 3);
    voltages[1] = ((quint8)payload.at(3) << 5) | ((quint8)payload.at(4) >> 3);
    voltages[2] = ((quint8)payload.at(5) << 5) | ((quint8)payload.at(6) >> 3);

    cellVoltages[stack][cellOffset] = voltages.at(0);
    cellVoltages[stack][cellOffset+1] = voltages.at(1);
    cellVoltages[stack][cellOffset+2] = voltages.at(2);

    if (cellOffset == 0) {
        cellVoltageValidity[stack][0] = ((quint8)payload.at(0) & 0x3);
    }
    cellVoltageValidity[stack][cellOffset + 1] = ((quint8)payload.at(2) & 0x3);
    cellVoltageValidity[stack][cellOffset + 2] = ((quint8)payload.at(4) & 0x3);
    cellVoltageValidity[stack][cellOffset + 3] = ((quint8)payload.at(6) & 0x3);

}

void MainWindow::decomposeCellTemperatures(quint8 stack, quint8 offset, QByteArray payload)
{
    temperatures[stack][offset + 0] = ((((quint16)payload.at(0) & 0xF) << 6) | (quint8)payload.at(1) >> 2) * 0.1f;
    temperatures[stack][offset + 1] = (((quint8)payload.at(2) << 2) | ((quint8)payload.at(3) >> 6)) * 0.1f;
    temperatures[stack][offset + 2] = ((((quint16)payload.at(3) & 0xF) << 6) | ((quint8)payload.at(4) >> 2)) * 0.1f;
    temperatures[stack][offset + 3] = (((quint8)payload.at(5) << 2) | ((quint8)payload.at(6) >> 6)) * 0.1f;

    temperatureValidity[stack][offset + 0] = ((quint8)payload.at(1) & 0x3);
    temperatureValidity[stack][offset + 1] = (((quint8)payload.at(3) >> 4) & 0x3);
    temperatureValidity[stack][offset + 2] = ((quint8)payload.at(4) & 0x3);
    temperatureValidity[stack][offset + 3] = (((quint8)payload.at(6) >> 4) & 0x3);

    if (offset == 10) {
        return;
    }
    temperatures[stack][offset + 4] = ((((quint8)payload.at(6) & 0xF) << 6) | ((quint8)payload.at(7) >> 2)) * 0.1f;

    temperatureValidity[stack][offset + 4] = ((quint8)payload.at(7) & 0x3);
}

void MainWindow::decomposeUid(quint8 stack, QByteArray payload)
{
    uid[stack] = (quint8)payload.at(1) << 24 | (quint8)payload.at(2) << 16 | (quint8)payload.at(3) << 8 | (quint8)payload.at(4);
}

void MainWindow::decompose_bms_1(QByteArray payload)
{
    bmsInfo.minCellVolt = (float)(((quint16)(payload[0] & 0xFF) << 5) | (quint16)(payload[1] & 0xFF) >> 3) * 0.001f;
    bmsInfo.minCellVoltValid = (payload[1] >> 2) & 0x01;
    bmsInfo.maxCellVolt =(float)((quint16)((payload[1] & 0x03) << 11) | ((quint16)(payload[2] & 0xFF) << 3) | (quint8)payload[3] >> 5) * 0.001f;
    bmsInfo.maxCellVoltValid = (payload[3] >> 4) & 0x01;
    bmsInfo.avgCellVolt = (float)((quint16)((payload[3] & 0x0F) << 9) | (quint16)((payload[4] & 0xFF) << 1) | (quint16)((payload[5] >> 7) & 0x01)) * 0.001f;
    bmsInfo.avgCellVoltValid = (payload[5] >> 6) & 0x01;
    bmsInfo.minSoc = (float)(((quint16)(payload[5] & 0x3F) << 4) | (quint16)((payload[6] >> 4) & 0x0F)) * 0.1f;
    bmsInfo.minSocValid = (payload[6] >> 3) & 0x01;
    bmsInfo.maxSoc = (float)(((quint16)(payload[6] & 0x07) << 7) | (quint16)((payload[7] >> 1) & 0x7F)) * 0.1f;
    bmsInfo.maxSocValid = (payload[7] & 0x01);

}

void MainWindow::decompose_bms_2(QByteArray payload)
{
    bmsInfo.batteryVoltage = (float)(((quint16)(payload[0] & 0xFF) << 5) | (quint16)((payload[1] >> 3) & 0x1F)) * 0.1f;
    bmsInfo.batteryVoltageValid = (payload[1] >> 2) & 0x01;
    bmsInfo.dcLinkVoltage = (float)(((quint16)(payload[2] & 0xFF) << 5) | (quint16)((payload[3] >> 3) & 0x1F)) * 0.1f;
    bmsInfo.dcLinkVoltageValid = (payload[3] >> 2) & 0x01;
    bmsInfo.current = (float)(((qint16)(payload[4]) << 8) | (quint8)(payload[5])) * 0.00625f;
    bmsInfo.currentValid = (payload[6] >> 7) & 0x01;
}

void MainWindow::decompose_bms_3(QByteArray payload)
{
    bmsInfo.isoRes = (float)(((quint16)(payload[0] & 0xFF) << 7) | (quint16)((payload[1] >> 1) & 0x7F)) * 0.1f;
    bmsInfo.isoResValid = payload[1] & 0x01;
    bmsInfo.shutdownStatus = (payload[2] >> 7) & 0x01;
    bmsInfo.tsState = static_cast<ts_state_t>((payload[2] >> 5) & 0x03);
    bmsInfo.amsScStatus = (payload[2] >> 4) & 0x01;
    bmsInfo.amsStatus = (payload[2] >> 3) & 0x01;
    bmsInfo.imdScStatus = (payload[2] >> 2) & 0x01;
    bmsInfo.imdStatus = (payload[2] >> 1) & 0x01;
    bmsInfo.error = static_cast<error_code_t>(payload[3] >> 1);
    bmsInfo.minTemp = (float)(((quint16)(payload[3] & 0x01) << 9) | (quint16)((payload[4] & 0xFF) << 1) | (quint16)((payload[5] >> 7) & 0x01)) * 0.1f;
    bmsInfo.minTempValid = (payload[5] >> 6) & 0x01;
    bmsInfo.maxTemp = (float)(((quint16)(payload[5] & 0x3F) << 4) | (quint16)((payload[6] >> 4) & 0x0F)) * 0.1f;
    bmsInfo.maxTempValid = (payload[6] >> 3) & 0x01;
    bmsInfo.avgTemp = (float)(((quint16)(payload[6] & 0x07) << 7) | (quint16)((payload[7] >> 1) & 0x7F)) * 0.1f;
    bmsInfo.avgTempValid = payload[7] & 0x01;
}

void MainWindow::decompose_balance(QCanBusFrame &frame)
{
    quint8 index = (quint8)frame.payload().at(0);

    balanceStatus[index][4]  = (frame.payload().at(1) >> 0) & 0x01;
    balanceStatus[index][5]  = (frame.payload().at(1) >> 1) & 0x01;
    balanceStatus[index][6]  = (frame.payload().at(1) >> 2) & 0x01;
    balanceStatus[index][7]  = (frame.payload().at(1) >> 3) & 0x01;
    balanceStatus[index][8]  = (frame.payload().at(1) >> 4) & 0x01;
    balanceStatus[index][9]  = (frame.payload().at(1) >> 5) & 0x01;
    balanceStatus[index][10]  = (frame.payload().at(1) >> 6) & 0x01;
    balanceStatus[index][11]  = (frame.payload().at(1) >> 7) & 0x01;
    balanceStatus[index][0]  = (frame.payload().at(2) >> 4) & 0x01;
    balanceStatus[index][1]  = (frame.payload().at(2) >> 5) & 0x01;
    balanceStatus[index][2] = (frame.payload().at(2) >> 6) & 0x01;
    balanceStatus[index][3] = (frame.payload().at(2) >> 7) & 0x01;
    update_ui_balancing();

//    quint64 gates = 0ULL;
//    gates |= ((quint64)(frame.payload().at(1) & 0xFF) << 40) | ((quint64)(frame.payload().at(2) & 0xFF) << 32) | ((quint64)(frame.payload().at(3) & 0xFF) << 24)
//            | ((quint64)(frame.payload().at(4) & 0xFF) << 16) | ((quint64)(frame.payload().at(5) & 0xFF) << 8) | ((quint64)(frame.payload().at(6) & 0xFF));

//    for (size_t i = 0; i < 12; i++) {

//        balanceStatus[3 * frame.payload().at(0)][]
//    }

//    switch (frame.payload().at(0)) {
//    case 0:
//        for (size_t i = 0; i < 6; i++) { //Message/byte
//            for (size_t j = 0; j < 8; j++) { //bit
//                balanceStatus[0][]
//            }
//        }

//        break;
//    case 1:
//        ::mempcpy(&balanceStatus[4][0], &gates, 6);
//        break;
//    case 2:
//        ::mempcpy(&balanceStatus[8][0], &gates, 6);
//        break;
//    }
//    qDebug() << balanceStatus[0][2];
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


void MainWindow::setUID(QVector<quint32> uid)
{
    QTreeWidgetItem *item = ui->parameters->topLevelItem(0);
    for (quint16 stack = 0; stack < item->childCount(); stack++) {
        item->child(stack)->setText(1, QString::number(uid.at(stack), 16));
    }
}


void MainWindow::updateCellVoltagePeriodic()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1); // Voltages
    QTreeWidgetItem *temps = ui->parameters->topLevelItem(3); //Temperatures
    QTreeWidgetItem *uids = ui->parameters->topLevelItem(0); //UIDs
    QTreeWidgetItem *cellVoltValid = ui->parameters->topLevelItem(2); //Open Wires

    for (quint16 stack = 0; stack < 12; stack++) {
        for (quint16 cell = 0; cell < 12; cell++) {
            volts->child(stack)->setText(cell+2, QString::number(cellVoltages[stack][cell]*0.001f, 'f', 3));
            cellVoltValid->child(stack)->setText(cell+2, returnValidity(cellVoltageValidity[stack][cell+1]));
        }
        cellVoltValid->child(stack)->setText(1, returnValidity(cellVoltageValidity[stack][0]));

        for (quint16 tempsens = 0; tempsens < 14; tempsens++) {
            if (temperatureValidity[stack][tempsens] == NOERROR){
                temps->child(stack)->setText(tempsens+1, QString::number(temperatures[stack][tempsens], 'f', 1));
            } else {
                temps->child(stack)->setText(tempsens+1, returnValidity(temperatureValidity[stack][tempsens]));
            }
        }

        uids->child(stack)->setText(1, QString::number(uid[stack], 16).toUpper());
    }

    if (bmsInfo.minCellVoltValid) {
        ui->minCellVolt->setText(QString("%1 V").arg(bmsInfo.minCellVolt, 5, 'f', 3));
    } else {
        ui->minCellVolt->setText("Invalid");
    }

    if (bmsInfo.maxCellVoltValid) {
        ui->maxCellVolt->setText(QString("%1 V").arg(bmsInfo.maxCellVolt, 5, 'f', 3));
    } else {
        ui->maxCellVolt->setText("Invalid");
    }

    if (bmsInfo.avgCellVoltValid) {
        ui->avgCellVolt->setText(QString("%1 V").arg(bmsInfo.avgCellVolt, 5, 'f', 3));
    } else {
        ui->avgCellVolt->setText("Invalid");
    }

    if (bmsInfo.minCellVoltValid && bmsInfo.maxCellVoltValid) {
        float delta = bmsInfo.maxCellVolt - bmsInfo.minCellVolt;
        ui->deltaCellVolt->setText(QString("%1 V").arg(delta, 5, 'f', 3));
    } else {
        ui->deltaCellVolt->setText("Invalid");
    }

    if (bmsInfo.minSocValid) {
        ui->minSoc->setText(QString("%1 %").arg(bmsInfo.minSoc, 6, 'f', 1));
    } else {
        ui->minSoc->setText("Invalid");
    }

    if (bmsInfo.maxSocValid) {
        ui->maxSoc->setText(QString("%1 %").arg(bmsInfo.maxSoc, 6, 'f', 1));
    } else {
        ui->maxSoc->setText("Invalid");
    }

    if (bmsInfo.minTempValid) {
        ui->minTemp->setText(QString("%1 °C").arg(bmsInfo.minTemp, 4, 'f', 1));
    } else {
        ui->minTemp->setText("Invalid");
    }

    if (bmsInfo.maxTempValid) {
        ui->maxTemp->setText(QString("%1 °C").arg(bmsInfo.maxTemp, 4, 'f', 1));
    } else {
        ui->maxTemp->setText("Invalid");
    }

    if (bmsInfo.avgTempValid) {
        ui->avgTemp->setText(QString("%1 °C").arg(bmsInfo.avgTemp, 4, 'f', 1));
    } else {
        ui->avgTemp->setText("Invalid");
    }

    if (bmsInfo.batteryVoltageValid) {
        ui->batteryVoltage->setText(QString("%1 V").arg(bmsInfo.batteryVoltage, 6, 'f', 1));
    } else {
        ui->batteryVoltage->setText("Invalid");
    }

    if (bmsInfo.dcLinkVoltageValid) {
        ui->dcLinkVoltage->setText(QString("%1 V").arg(bmsInfo.dcLinkVoltage, 6, 'f', 1));
    } else {
        ui->dcLinkVoltage->setText("Invalid");
    }

    if (bmsInfo.currentValid) {
        ui->current->setText(QString("%1 A").arg(bmsInfo.current, 6, 'f', 2));
    } else {
        ui->current->setText("Invalid");
    }

    if (bmsInfo.isoResValid) {
        ui->isoRes->setText(QString("%1 kΩ").arg(bmsInfo.isoRes, 6, 'f', 2));
    } else {
        ui->isoRes->setText("Invalid");
    }

    ui->tsState->setText(ts_state_to_string(bmsInfo.tsState));

    if (bmsInfo.imdStatus) {
        ui->imdStatus->setText("OK");
    } else {
        ui->imdStatus->setText("Error");
    }

    if (bmsInfo.imdScStatus) {
        ui->imdSC->setText("OK");
    } else {
        ui->imdSC->setText("Error");
    }

    if (bmsInfo.amsStatus) {
        ui->amsStatus->setText("OK");
    } else {
        ui->amsStatus->setText("Error");
    }

    if (bmsInfo.amsScStatus) {
        ui->amsSC->setText("OK");
    } else {
        ui->amsSC->setText("Error");
    }

    if (bmsInfo.shutdownStatus) {
        ui->scStatus->setText("OK");
    } else {
        ui->scStatus->setText("Error");
    }

    static error_code_t lastError = ERROR_NO_ERROR;


    QDateTime dateTime = QDateTime::currentDateTime();
    if (bmsInfo.error != lastError) {
        if (bmsInfo.error == ERROR_NO_ERROR) {
            ui->errorLog->append("[" + dateTime.date().toString("dd.MM.yyyy") + " " + dateTime.time().toString("hh:mm:ss") + "] [INFO]: " + error_to_string(bmsInfo.error));
        } else {
            ui->errorLog->append("[" + dateTime.date().toString("dd.MM.yyyy") + " " + dateTime.time().toString("hh:mm:ss") + "] [ERROR]: " + error_to_string(bmsInfo.error));
        }
    }
    lastError = bmsInfo.error;


    if (linkAvailable) {
        ui->linkDisplay->setStyleSheet("background-color: rgb(0, 255, 0);");
    } else {
        ui->linkDisplay->setStyleSheet("background-color: rgb(255, 0, 0);");
    }
    linkAvailable = false;


    ::memset(cellVoltages, 0, 144);
    ::memset(temperatures, 0, 12*14);

    //poll_balance_status();

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


void MainWindow::on_diagButton_clicked()
{
    DiagDialog *diagDialog = new DiagDialog();
    QObject::connect(diagDialog, &DiagDialog::balancing_enable, this, &MainWindow::global_balancing_enable);
    diagDialog->setAttribute(Qt::WA_DeleteOnClose);
    diagDialog->exec();
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


void MainWindow::on_actionDiagnostic_triggered()
{
    on_diagButton_clicked();
}

