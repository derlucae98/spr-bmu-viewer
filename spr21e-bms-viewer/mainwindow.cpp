#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);




    ::memset(&canDataLv, 0, sizeof(can_data_LV_t));

    if (geteuid()) {
        QMessageBox mb;
        mb.setText("Some parts of this software require root privileges! Click OK and enter your password or run as root.");
        mb.exec();
    }

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
    });
    QObject::connect(can, &Can::device_down, this, [=] {
        interfaceUp = false;
        ui->infoFrame->setEnabled(false);
        ui->parameters->setEnabled(false);
        ui->btnConnectPcan->setText("Connect");
        ui->cbSelectPCAN->setEnabled(true);
    });
    QObject::connect(can, &Can::new_frame, this, &MainWindow::new_frame);
    QObject::connect(can, &Can::available_devices, this, [=] (QStringList names) {
        ui->cbSelectPCAN->addItems(names);
        this->show();});
    can->init();

    ::memset(&tsBatteryData, 0, sizeof(TS_Accu::ts_battery_data_t));
    tsAccu = new TS_Accu(this);
    QObject::connect(tsAccu, &TS_Accu::new_data, this, &MainWindow::update_ui_ts);
    QObject::connect(can, &Can::new_frame, tsAccu, &TS_Accu::can_frame);
    QObject::connect(tsAccu, &TS_Accu::can_send, can, &Can::send_frame);
    QObject::connect(tsAccu, &TS_Accu::link_availability_changed, this, &MainWindow::ts_link_available);

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

    ui->reqTsActive->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::new_frame(QCanBusFrame frame)
{
    switch (frame.frameId()) {
    case CAN_ID_LV_ACCU_STATS_1:
        decompose_lv_stats_1(frame.payload());
        break;
    case CAN_ID_LV_ACCU_STATS_2:
        decompose_lv_stats_2(frame.payload());
        break;
    case CAN_ID_LV_ACCU_STATE:
        decompose_lv_state(frame.payload());
        break;
    case CAN_ID_LV_ACCU_CELL_VOLT_03:
        decompose_lv_cell_volt_03(frame.payload());
        break;
    case CAN_ID_LV_ACCU_CELL_VOLT_45:
        decompose_lv_cell_volt_45(frame.payload());
        break;
    case CAN_ID_LV_ACCU_TEMP_03:
        decompose_lv_temp_03(frame.payload());
        break;
    case CAN_ID_LV_ACCU_TEMP_47:
        decompose_lv_temp_47(frame.payload());
    }
}

void MainWindow::ts_link_available(bool available)
{
    if (available) {
        ui->linkDisplay->setStyleSheet("background-color: rgb(0, 255, 0);");
    } else {
        ui->linkDisplay->setStyleSheet("background-color: rgb(255, 0, 0);");
    }
}

void MainWindow::update_ui()
{

}

QString MainWindow::error_to_string(TS_Accu::contactor_error_t error)
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


QString MainWindow::ts_state_to_string(TS_Accu::ts_state_t state)
{
    switch (state) {
    case TS_Accu::TS_STATE_STANDBY:
        return "Standby";
    case TS_Accu::TS_STATE_PRE_CHARGING:
        return "Pre-charging";
    case TS_Accu::TS_STATE_OPERATE:
        return "Operate";
    case TS_Accu::TS_STATE_ERROR:
        return "Error";
    }
}

QString MainWindow::returnValidity(quint8 val)
{
    switch (val) {
    case TS_Accu::NOERROR:
        return "OK";
    case TS_Accu::PECERROR:
        return "PEC error";
    case TS_Accu::VALUEOUTOFRANGE:
        return "Value out of range";
    case TS_Accu::OPENCELLWIRE:
        return "Open wire";
    default:
        return "Unknown error";
    }
}

void MainWindow::update_ui_ts(TS_Accu::ts_battery_data_t data)
{
    tsBatteryData = data;
    update_ui_voltage();
    update_ui_temperature();
    update_ui_stats();
    update_ui_balancing();
    update_ui_uid();
//    update_ui_lv();
}

void MainWindow::update_ui_lv()
{
    ui->minCellVolt_LV->setText(QString("%1 V").arg(canDataLv.minCellVolt, 5, 'f', 3));
    ui->maxCellVolt_LV->setText(QString("%1 V").arg(canDataLv.maxCellVolt, 5, 'f', 3));
    ui->avgCellVolt_LV->setText(QString("%1 V").arg(canDataLv.avgCellVolt, 5, 'f', 3));
    ui->minTemp_LV->setText(QString("%1 °C").arg(canDataLv.minTemp, 4, 'f', 1));
    ui->maxTemp_LV->setText(QString("%1 °C").arg(canDataLv.maxTemp, 4, 'f', 1));
    ui->avgTemp_LV->setText(QString("%1 °C").arg(canDataLv.avgTemp, 4, 'f', 1));

    float delta = canDataLv.maxCellVolt - canDataLv.minCellVolt;
    ui->deltaCellVolt_LV->setText(QString("%1 V").arg(delta, 5, 'f', 3));

    QTreeWidgetItem *volts = ui->parameters_LV->topLevelItem(0);
    for (quint16 cell = 0; cell < MAX_NUM_OF_LV_CELLS; cell++) {
        volts->child(0)->setText(cell+2, QString::number(canDataLv.cellVoltage[cell], 'f', 4));
        //openWire->child(stack)->setText(cell+2, returnValidity(canData.cellVoltageStatus[stack][cell+1]));
    }
    //openWire->child(stack)->setText(1, returnValidity(canData.cellVoltageStatus[stack][0]));

    ::memset(canDataLv.cellVoltage, 0, MAX_NUM_OF_LV_CELLS * sizeof(float));

    QTreeWidgetItem *temps = ui->parameters_LV->topLevelItem(2); //Temperatures
    for (quint16 tempsens = 0; tempsens < MAX_NUM_OF_LV_TEMPSENS; tempsens++) {
        temps->child(0)->setText(tempsens + 1, QString::number(canDataLv.temperature[tempsens], 'f', 1));
    }
    ::memset(canDataLv.temperature, 0, MAX_NUM_OF_LV_TEMPSENS);
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

void MainWindow::on_clearErrorLog_clicked()
{
    ui->errorLog->clear();
}

void MainWindow::on_actionAbout_SPR_BMS_viewer_triggered()
{
    AboutDialog *aboutDialog = new AboutDialog();
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void MainWindow::on_actionConfig_triggered()
{
    if (tsAccu) {
        tsAccu->open_config_dialog();
    }
}



void MainWindow::decompose_lv_stats_1(QByteArray data)
{
    canDataLv.avgCellVolt = (((quint8)data.at(0) << 8) | (quint8)data.at(1)) * 0.0001f;
    canDataLv.maxCellVolt = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canDataLv.minCellVolt = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;

}

void MainWindow::decompose_lv_stats_2(QByteArray data)
{
    canDataLv.maxTemp = (((quint8)data.at(0) << 8) | (quint8)(data.at(1))) * 0.1f;
    canDataLv.minTemp = (((quint8)data.at(2) << 8) | (quint8)(data.at(3))) * 0.1f;
    //canDataLv.avgTemp = (((quint8)data.at(6) << 8) | (quint8)(data.at(7))) * 0.1f;
}

void MainWindow::decompose_lv_state(QByteArray data)
{

}

void MainWindow::decompose_lv_cell_volt_03(QByteArray data)
{
    canDataLv.cellVoltage[0] = (((quint8)data.at(0) << 8) | (quint8)data.at(1)) * 0.0001f;
    canDataLv.cellVoltage[1] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    canDataLv.cellVoltage[2] = (((quint8)data.at(4) << 8) | (quint8)data.at(5)) * 0.0001f;
    canDataLv.cellVoltage[3] = (((quint8)data.at(6) << 8) | (quint8)data.at(7)) * 0.0001f;
}

void MainWindow::decompose_lv_cell_volt_45(QByteArray data)
{
    canDataLv.cellVoltage[4] = (((quint8)data.at(0) << 8) | (quint8)data.at(1)) * 0.0001f;
    canDataLv.cellVoltage[5] = (((quint8)data.at(2) << 8) | (quint8)data.at(3)) * 0.0001f;
    //todo : SOC
    //todo: Strom
}

void MainWindow::decompose_lv_temp_03(QByteArray data)
{

}

void MainWindow::decompose_lv_temp_47(QByteArray data)
{

}

void MainWindow::update_ui_balancing()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1); // Voltages
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < TS_Accu::MAX_NUM_OF_CELLS; cell++) {
            if (tsBatteryData.balance[stack][cell]) {
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
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        uid->child(stack)->setText(1, QString::number(tsBatteryData.UID[stack], 16).toUpper());
    }
}

void MainWindow::update_ui_voltage()
{
    QTreeWidgetItem *volts = ui->parameters->topLevelItem(1);
    QTreeWidgetItem *openWire = ui->parameters->topLevelItem(2); //Open Wires
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < TS_Accu::MAX_NUM_OF_CELLS; cell++) {
            volts->child(stack)->setText(cell+2, QString::number(tsBatteryData.cellVoltage[stack][cell], 'f', 4));
            openWire->child(stack)->setText(cell+2, returnValidity(tsBatteryData.cellVoltageStatus[stack][cell+1]));
        }
        openWire->child(stack)->setText(1, returnValidity(tsBatteryData.cellVoltageStatus[stack][0]));
    }
    ::memset(tsBatteryData.cellVoltage, 0, TS_Accu::MAX_NUM_OF_SLAVES * TS_Accu::MAX_NUM_OF_CELLS);
}

void MainWindow::update_ui_temperature()
{
    QTreeWidgetItem *temps = ui->parameters->topLevelItem(3); //Temperatures
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 tempsens = 0; tempsens < TS_Accu::MAX_NUM_OF_TEMPSENS; tempsens++) {
            if (tsBatteryData.temperatureStatus[stack][tempsens] == TS_Accu::NOERROR){
                temps->child(stack)->setText(tempsens + 1, QString::number(tsBatteryData.temperature[stack][tempsens], 'f', 1));
            } else {
                temps->child(stack)->setText(tempsens + 1, returnValidity(tsBatteryData.temperatureStatus[stack][tempsens]));
            }
        }
    }
    ::memset(tsBatteryData.temperature, 0, TS_Accu::MAX_NUM_OF_SLAVES * TS_Accu::MAX_NUM_OF_TEMPSENS);
}

void MainWindow::update_ui_stats()
{
    if (tsBatteryData.voltageValid) {
        ui->minCellVolt->setText(QString("%1 V").arg(tsBatteryData.minCellVolt, 5, 'f', 3));
        ui->maxCellVolt->setText(QString("%1 V").arg(tsBatteryData.maxCellVolt, 5, 'f', 3));
        ui->avgCellVolt->setText(QString("%1 V").arg(tsBatteryData.avgCellVolt, 5, 'f', 3));
        float delta = tsBatteryData.maxCellVolt - tsBatteryData.minCellVolt;
        ui->deltaCellVolt->setText(QString("%1 V").arg(delta, 5, 'f', 3));
    } else {
        ui->minCellVolt->setText("Invalid");
        ui->maxCellVolt->setText("Invalid");
        ui->avgCellVolt->setText("Invalid");
        ui->deltaCellVolt->setText("Invalid");
    }

    if (tsBatteryData.socValid) {
        ui->minSoc->setText(QString("%1 %").arg(tsBatteryData.minSoc));
        ui->maxSoc->setText(QString("%1 %").arg(tsBatteryData.maxSoc));
    } else {
        ui->minSoc->setText("Invalid");
        ui->maxSoc->setText("Invalid");
    }

    if (tsBatteryData.tempValid) {
        ui->minTemp->setText(QString("%1 °C").arg(tsBatteryData.minTemp, 4, 'f', 1));
        ui->maxTemp->setText(QString("%1 °C").arg(tsBatteryData.maxTemp, 4, 'f', 1));
        ui->avgTemp->setText(QString("%1 °C").arg(tsBatteryData.avgTemp, 4, 'f', 1));
    } else {
        ui->minTemp->setText("Invalid");
        ui->maxTemp->setText("Invalid");
        ui->avgTemp->setText("Invalid");
    }

    if (tsBatteryData.batteryVoltageValid) {
        ui->batteryVoltage->setText(QString("%1 V").arg(tsBatteryData.batteryVoltage, 6, 'f', 1));
    } else {
        ui->batteryVoltage->setText("Invalid");
    }

    if (tsBatteryData.dcLinkVoltageValid) {
        ui->dcLinkVoltage->setText(QString("%1 V").arg(tsBatteryData.dcLinkVoltage, 6, 'f', 1));
    } else {
        ui->dcLinkVoltage->setText("Invalid");
    }

    if (tsBatteryData.currentValid) {
        ui->current->setText(QString("%1 A").arg(tsBatteryData.current, 6, 'f', 2));
    } else {
        ui->current->setText("Invalid");
    }

    if (tsBatteryData.isolationResistanceValid) {
        ui->isoRes->setText(QString("%1 kΩ").arg(tsBatteryData.isolationResistance, 6, 'f', 2));
    } else {
        ui->isoRes->setText("Invalid");
    }

    ui->tsState->setText(ts_state_to_string(tsBatteryData.tsState));

    if (tsBatteryData.errorCode & TS_Accu::ERROR_IMD_FAULT) {
        ui->imdStatus->setText("Error");
    } else {
        ui->imdStatus->setText("OK");
    }

    if (tsBatteryData.errorCode & TS_Accu::ERROR_IMD_POWERSTAGE_DISABLED) {
        ui->imdSC->setText("Error");
    } else {
        ui->imdSC->setText("OK");
    }

    if (tsBatteryData.errorCode & TS_Accu::ERROR_AMS_FAULT) {
        ui->amsStatus->setText("Error");
    } else {
        ui->amsStatus->setText("OK");
    }

    if (tsBatteryData.errorCode & TS_Accu::ERROR_AMS_POWERSTAGE_DISABLED) {
        ui->amsSC->setText("Error");
    } else {
        ui->amsSC->setText("OK");
    }

    if (tsBatteryData.errorCode & TS_Accu::ERROR_SDC_OPEN) {
        ui->scStatus->setText("Error");
    } else {
        ui->scStatus->setText("OK");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    (void) event;
    if (can) {
        can->disconnect_device();
    }
}


void MainWindow::on_tsTakeControl_stateChanged(int arg1)
{
    if (tsAccu) {
        tsAccu->ts_take_control((bool)arg1);
    }
    if (arg1) {
        ui->reqTsActive->setEnabled(true);
    } else {
        ui->reqTsActive->setChecked(false);
        ui->reqTsActive->setEnabled(false);
    }
}

void MainWindow::on_reqTsActive_stateChanged(int arg1)
{
    if (tsAccu) {
        tsAccu->ts_activate((bool)arg1);
    }
}

