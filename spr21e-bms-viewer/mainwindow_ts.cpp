#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::init_ts()
{
    ::memset(&tsBatteryData, 0, sizeof(TS_Accu::ts_battery_data_t));

    tsAccu = new TS_Accu(this);
    QObject::connect(tsAccu, &TS_Accu::new_data, this, &MainWindow::update_ui_ts);
    QObject::connect(tsAccu, &TS_Accu::link_availability_changed, this, &MainWindow::ts_link_available);
    QObject::connect(tsAccu, &TS_Accu::ts_state_changed, this, &MainWindow::ts_state_changed);
    tsLinkAvailable = false;
    ui->reqTsActive->setEnabled(false);
    ui->btnConfig->setEnabled(false);
    ui->tsTakeControl->setEnabled(false);
    tsNotifyOnErrors = false;
    ui->tsToggleNotification->setStyleSheet("image: url(:/img/res/no-bell.svg);");
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


void MainWindow::on_btnConfig_clicked()
{
    if (tsAccu) {
        tsAccu->open_config_dialog();
    }
}


void MainWindow::on_btnShowErrors_clicked()
{
    show_error_message();
}

void MainWindow::ui_ts_invalidate_all()
{
    ui->minCellVolt->setText("---   V");
    ui->maxCellVolt->setText("---   V");
    ui->avgCellVolt->setText("---   V");
    ui->deltaCellVolt->setText("---   V");
    ui->minSoc->setText("---    %");
    ui->maxSoc->setText("---    %");
    ui->minTemp->setText("---  °C");
    ui->maxTemp->setText("---  °C");
    ui->avgTemp->setText("---  °C");
    ui->batteryVoltage->setText("---    V");
    ui->dcLinkVoltage->setText("---    V");
    ui->current->setText("---    A");
    ui->isoRes->setText("---    kΩ");
    ui->tsState->setText("---");
    ui->imdStatus->setText("---");
    ui->imdStatus->setStyleSheet("");
    ui->amsStatus->setText("---");
    ui->amsStatus->setStyleSheet("");
    ui->scStatus->setText("---");
    ui->scStatus->setStyleSheet("");

    QTreeWidgetItem *volts = ui->tsParameters->topLevelItem(1);
    QTreeWidgetItem *openWire = ui->tsParameters->topLevelItem(2); //Open Wires
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < TS_Accu::MAX_NUM_OF_CELLS; cell++) {
            volts->child(stack)->setText(cell+2, "---");
            openWire->child(stack)->setText(cell+2, "---");
        }
        openWire->child(stack)->setText(1, "---");
    }

    QTreeWidgetItem *temps = ui->tsParameters->topLevelItem(3); //Temperatures
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 tempsens = 0; tempsens < TS_Accu::MAX_NUM_OF_TEMPSENS; tempsens++) {
            temps->child(stack)->setText(tempsens + 1, "---");
        }
    }

    ui->tsAccuStatus->setStyleSheet("");
    ui->tsSoc->setStyleSheet("");
    ui->tsSoc->setToolTip("");
    ui->tsTemperature->setStyleSheet("");
    ui->tsTemperature->setToolTip("");
    ui->tsIndicator->setStyleSheet("");
    ui->tsIndicator->setToolTip("");
}

void MainWindow::ts_link_available(bool available)
{
    if (available) {
        ui->tsConnectionStatus->setStyleSheet("image: url(:/img/res/hex-check.svg);");
        ui->tsConnectionStatus->setToolTip("Connected");
        ui->reqTsActive->setEnabled(false);
        ui->btnConfig->setEnabled(true);
        ui->tsTakeControl->setEnabled(true);
        ui->cbAlertOnErr->setEnabled(true);
        ui->btnShowErrors->setEnabled(true);
    } else {
        ui->tsConnectionStatus->setStyleSheet("image: url(:/img/res/hex-warning.svg);");
        ui->tsConnectionStatus->setToolTip("Disconnected");
        ui_ts_invalidate_all();
        ui->reqTsActive->setEnabled(false);
        ui->btnConfig->setEnabled(false);
        ui->tsTakeControl->setEnabled(false);
        ui->cbAlertOnErr->setEnabled(false);
        ui->btnShowErrors->setEnabled(false);
    }
}

void MainWindow::ts_state_changed(TS_Accu::ts_state_t state, TS_Accu::contactor_error_t error)
{
    QString errorString;
    if (state == TS_Accu::TS_STATE_ERROR) {
        QStringList errors = TS_Accu::contactor_error_to_string(error);
        for (const auto &i : errors) {
            errorString.push_back(i);
            errorString.push_back("\n");
        }
        ui->btnShowErrors->setEnabled(true);
        ui->btnShowErrors->setText("Show errors!");
        if (tsNotifyOnErrors) {
            /*  What an ugly workaround...
             *  calling show_error_message(); directly locks the whole system for whatever reason.
             *  Deferring it to a timer timeout slot solves the problem.
             *  Fix me I guess?
             */
            QTimer *worker = new QTimer();
            worker->setSingleShot(true);
            QObject::connect(worker, &QTimer::timeout, this, [=]{
                show_error_message();
                worker->deleteLater();
            });
            worker->start();
        }
    } else {
        ui->btnShowErrors->setEnabled(false);
        ui->btnShowErrors->setText("No errors!");
    }
    this->tsErrorString = errorString;
}

void MainWindow::update_ui_ts(TS_Accu::ts_battery_data_t data)
{
    tsBatteryData = data;

    update_ui_ts_voltage();
    update_ui_ts_temperature();
    update_ui_ts_stats();
    update_ui_ts_balancing();
    update_ui_ts_uid();

    get_error_reason(data.errorCode);
}

void MainWindow::update_ui_ts_uid()
{
    QTreeWidgetItem *uid = ui->tsParameters->topLevelItem(0);
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        uid->child(stack)->setText(1, QString::number(tsBatteryData.UID[stack], 16).toUpper());
    }
}

void MainWindow::update_ui_ts_voltage()
{
    QTreeWidgetItem *volts = ui->tsParameters->topLevelItem(1);
    QTreeWidgetItem *openWire = ui->tsParameters->topLevelItem(2); //Open Wires
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 cell = 0; cell < TS_Accu::MAX_NUM_OF_CELLS; cell++) {
            volts->child(stack)->setText(cell+2, QString::number(tsBatteryData.cellVoltage[stack][cell], 'f', 4));
            openWire->child(stack)->setText(cell+2, TS_Accu::sensor_status_to_string(tsBatteryData.cellVoltageStatus[stack][cell+1]));
        }
        openWire->child(stack)->setText(1, TS_Accu::sensor_status_to_string(tsBatteryData.cellVoltageStatus[stack][0]));
    }
    ::memset(tsBatteryData.cellVoltage, 0, TS_Accu::MAX_NUM_OF_SLAVES * TS_Accu::MAX_NUM_OF_CELLS);
}

void MainWindow::update_ui_ts_temperature()
{
    QTreeWidgetItem *temps = ui->tsParameters->topLevelItem(3); //Temperatures
    for (quint16 stack = 0; stack < TS_Accu::MAX_NUM_OF_SLAVES; stack++) {
        for (quint16 tempsens = 0; tempsens < TS_Accu::MAX_NUM_OF_TEMPSENS; tempsens++) {
            if (tsBatteryData.temperatureStatus[stack][tempsens] == TS_Accu::NOERROR){
                temps->child(stack)->setText(tempsens + 1, QString::number(tsBatteryData.temperature[stack][tempsens], 'f', 1));
            } else {
                temps->child(stack)->setText(tempsens + 1, TS_Accu::sensor_status_to_string(tsBatteryData.temperatureStatus[stack][tempsens]));
            }
        }
    }
    ::memset(tsBatteryData.temperature, 0, TS_Accu::MAX_NUM_OF_SLAVES * TS_Accu::MAX_NUM_OF_TEMPSENS);
}

void MainWindow::update_ui_ts_stats()
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
        ui->minSoc->setText(QString("    %1 %").arg(tsBatteryData.minSoc));
        ui->maxSoc->setText(QString("    %1 %").arg(tsBatteryData.maxSoc));

        if (tsBatteryData.minSoc <= 10) {
            ui->tsSoc->setStyleSheet("image: url(:/img/res/battery-empty.svg);");
        } else if (tsBatteryData.minSoc > 10 && tsBatteryData.minSoc <= 40) {
            ui->tsSoc->setStyleSheet("image: url(:/img/res/battery-almost-empty.svg);");
        } else if (tsBatteryData.minSoc > 40 && tsBatteryData.minSoc <= 70) {
            ui->tsSoc->setStyleSheet("image: url(:/img/res/battery-almost-full.svg);");
        } else {
            ui->tsSoc->setStyleSheet("image: url(:/img/res/battery-full.svg);");
        }
        ui->tsSoc->setToolTip(QString("%1 %").arg(tsBatteryData.minSoc));

    } else {
        ui->minSoc->setText("Invalid");
        ui->maxSoc->setText("Invalid");
        ui->tsSoc->setStyleSheet("");
        ui->tsSoc->setToolTip("");
    }

    if (tsBatteryData.tempValid) {
        ui->minTemp->setText(QString("%1 °C").arg(tsBatteryData.minTemp, 4, 'f', 1));
        ui->maxTemp->setText(QString("%1 °C").arg(tsBatteryData.maxTemp, 4, 'f', 1));
        ui->avgTemp->setText(QString("%1 °C").arg(tsBatteryData.avgTemp, 4, 'f', 1));

        if (tsBatteryData.maxTemp <= 25) {
            ui->tsTemperature->setStyleSheet("image: url(:/img/res/temp-cold.svg);");
        } else if (tsBatteryData.maxTemp > 25 && tsBatteryData.maxTemp <= 50) {
            ui->tsTemperature->setStyleSheet("image: url(:/img/res/temp-mid.svg);");
        } else {
            ui->tsTemperature->setStyleSheet("image: url(:/img/res/temp-hot.svg);");
        }
        ui->tsTemperature->setToolTip(QString("%1 °C").arg(tsBatteryData.maxTemp, 4, 'f', 1));
    } else {
        ui->minTemp->setText("Invalid");
        ui->maxTemp->setText("Invalid");
        ui->avgTemp->setText("Invalid");
        ui->tsTemperature->setStyleSheet("");
        ui->tsTemperature->setToolTip("");
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

    ui->tsState->setText(TS_Accu::ts_state_to_string(tsBatteryData.tsState));

    if (tsBatteryData.tsState == TS_Accu::TS_STATE_PRE_CHARGING || tsBatteryData.tsState == TS_Accu::TS_STATE_OPERATE) {
        ui->tsIndicator->setStyleSheet("image: url(:/img/res/emergency-on.svg);");
        ui->tsIndicator->setToolTip("TS on");
    } else {
        ui->tsIndicator->setStyleSheet("image: url(:/img/res/emergency-off.svg);");
        ui->tsIndicator->setToolTip("TS off");
    }

    if ((tsBatteryData.errorCode & TS_Accu::ERROR_IMD_FAULT) && (tsBatteryData.errorCode & TS_Accu::ERROR_IMD_POWERSTAGE_DISABLED)) {
        ui->imdStatus->setText("Error");
        ui->imdStatus->setStyleSheet("color: rgb(255, 0, 0);");
    } else if (((tsBatteryData.errorCode & TS_Accu::ERROR_IMD_FAULT) == 0) && (tsBatteryData.errorCode & TS_Accu::ERROR_IMD_POWERSTAGE_DISABLED)) {
        ui->imdStatus->setText("Ready. Reset?");
        ui->imdStatus->setStyleSheet("color: rgb(255, 127, 0);");
    } else {
        ui->imdStatus->setText("OK");
        ui->imdStatus->setStyleSheet("color: rgb(0, 127, 0);");
    }

    /* Status indicator:
     * No errors, SDC closed: Check mark
     * No errors but SDC open: exclamation with check mark
     * Any error: exclamation mark
     * A click on this symbol shows the error dialog which lists the errors
     */

    if (tsBatteryData.errorCode == TS_Accu::ERROR_NO_ERROR) {
        ui->tsAccuStatus->setStyleSheet("border-image: url(:/img/res/circle-check.svg);");
    } else if (tsBatteryData.errorCode == TS_Accu::ERROR_AMS_FAULT || tsBatteryData.errorCode == TS_Accu::ERROR_IMD_FAULT) {
        ui->tsAccuStatus->setStyleSheet("border-image: url(:/img/res/circle-warning.svg);");
    } else if (tsBatteryData.errorCode == TS_Accu::ERROR_SDC_OPEN) {
        ui->tsAccuStatus->setStyleSheet("border-image: url(:/img/res/circle-info-ok.svg);");
    }

    if ((tsBatteryData.errorCode & TS_Accu::ERROR_AMS_FAULT) && (tsBatteryData.errorCode & TS_Accu::ERROR_AMS_POWERSTAGE_DISABLED)) {
        ui->amsStatus->setText("Error");
        ui->amsStatus->setStyleSheet("color: rgb(255, 0, 0);");
    } else if (((tsBatteryData.errorCode & TS_Accu::ERROR_AMS_FAULT) == 0) && (tsBatteryData.errorCode & TS_Accu::ERROR_AMS_POWERSTAGE_DISABLED)) {
        ui->amsStatus->setText("Ready. Reset?");
        ui->amsStatus->setStyleSheet("color: rgb(255, 127, 0);");
    } else {
        ui->amsStatus->setStyleSheet("color: rgb(0, 127, 0);");
        ui->amsStatus->setText("OK");
    }

    if (tsBatteryData.errorCode & TS_Accu::ERROR_SDC_OPEN) {
        ui->scStatus->setText("Error");
        ui->scStatus->setStyleSheet("color: rgb(255, 0, 0);");
    } else {
        ui->scStatus->setText("OK");
        ui->scStatus->setStyleSheet("color: rgb(0, 127, 0);");
    }
}

void MainWindow::update_ui_ts_balancing()
{
    QTreeWidgetItem *volts = ui->tsParameters->topLevelItem(1); // Voltages
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

void MainWindow::get_error_reason(TS_Accu::contactor_error_t error)
{
    static quint32 errOld = static_cast<quint32>(error);
    quint32 err = static_cast<quint32>(error);

    for (quint32 i = 0; i < 32; i++) {
        if ((1 << i) == TS_Accu::ERROR_AMS_FAULT || (1 << i) == TS_Accu::ERROR_AMS_POWERSTAGE_DISABLED || (1 << i) == TS_Accu::ERROR_IMD_POWERSTAGE_DISABLED) {
            // Exclude 0x2 "ERROR_AMS_FAULT", "0x200 "ERROR_AMS_POWERSTAGE_DISABLED" and 0x400 ERROR_IMD_POWERSTAGE_DISABLED from error log
            // disabled powerstages are a consequence of AMS or IMD faults
            // If any AMS fault appears, the AMS error bit is set. So there is no point in printing it if you also print the reason
            continue;
        }

        if (((errOld & (1 << i)) == 0) && (err & (1 << i))) {
            // Error arrive
            if (err & 0xFE7F) {
                // Check if any bits other than SDC_OPEN or PRECHARGE_TIMEOUT are set
                if ((1 << i) == TS_Accu::ERROR_SDC_OPEN) {
                    // Suppress SDC open error if an AMS or IMD fault occurred. Open SDC is a consequence of AMS and IMD faults
                    // If, however, the SDC id open while AMS and IMD are fine, print the error
                    continue;
                }
            }
            append_error(TS_Accu::contactor_error_to_string(static_cast<TS_Accu::contactor_error_t>(1 << i)).constFirst() + ".", SEVERITY_ERROR);

        } else if ((errOld & (1 << i)) && ((err & (1 << i)) == 0)) {
            // Error leave
            append_error(TS_Accu::contactor_error_to_string(static_cast<TS_Accu::contactor_error_t>(1 << i)).constFirst() + " cleared.", SEVERITY_INFO);
        }
    }

    if ((errOld != TS_Accu::ERROR_NO_ERROR) && (err == TS_Accu::ERROR_NO_ERROR)) {
        append_error("System ready!", SEVERITY_INFO_GREEN);
    }

    errOld = err;
}

void MainWindow::on_tsToggleNotification_clicked()
{
    if (tsNotifyOnErrors) {
        tsNotifyOnErrors = false;
        ui->tsToggleNotification->setStyleSheet("image: url(:/img/res/no-bell.svg);");
    } else {
        tsNotifyOnErrors = true;
        ui->tsToggleNotification->setStyleSheet("image: url(:/img/res/bell.svg);");
    }
}

void MainWindow::on_tsAccuStatus_clicked()
{
    if (tsBatteryData.errorCode != TS_Accu::ERROR_NO_ERROR) {
        show_error_message();
    }
}

void MainWindow::show_error_message()
{
    // Show error dialog on button click (or automatically if checkbox is checked)
    // Close dialog if error cleares
    errorDialog = new ErrorDialog(tsBatteryData.errorCode);

    auto connectionStateChanged = QObject::connect(tsAccu, &TS_Accu::ts_state_changed, this, [=](TS_Accu::ts_state_t state){
        if (state != TS_Accu::TS_STATE_ERROR) {
            if (errorDialog) {
                errorDialog->close();
            }
        }
    });

    auto connectionNewData = QObject::connect(tsAccu, &TS_Accu::new_data, this, [=](TS_Accu::ts_battery_data_t data) {
        if (errorDialog) {
            errorDialog->updateErrors(data.errorCode);
        }
    });

    errorDialog->setAttribute(Qt::WA_DeleteOnClose);
    errorDialog->exec();
    QObject::disconnect(connectionStateChanged);
    QObject::disconnect(connectionNewData);

}
