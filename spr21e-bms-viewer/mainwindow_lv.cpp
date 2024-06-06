#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::init_lv()
{
    ::memset(&lvBatteryData, 0, sizeof(LV_Accu::lv_battery_data_t));
    lvAccu = new LV_Accu(this);
    QObject::connect(lvAccu, &LV_Accu::new_data, this, &MainWindow::update_ui_lv);
    QObject::connect(lvAccu, &LV_Accu::link_availability_changed, this, &MainWindow::lv_link_available);
    QObject::connect(lvAccu, &LV_Accu::lv_state_changed, this, &MainWindow::lv_state_changed);
    lvLinkAvailable = false;
    lvNotifyOnErrors = false;
    ui->lvToggleNotification->setStyleSheet("image: url(:/img/res/no-bell.svg);");
}

void MainWindow::lv_link_available(bool available)
{
    if (available) {
        ui->lvConnectionStatus->setStyleSheet("image: url(:/img/res/hex-check.svg);");
        ui->lvConnectionStatus->setToolTip("Connected");
    } else {
        ui->lvConnectionStatus->setStyleSheet("image: url(:/img/res/hex-warning.svg);");
        ui->lvConnectionStatus->setToolTip("Disconnected");
        ui_lv_invalidate_all();
    }
}

void MainWindow::ui_lv_invalidate_all()
{
    ui->minCellVolt_LV->setText("---   V");
    ui->maxCellVolt_LV->setText("---   V");
    ui->avgCellVolt_LV->setText("---   V");
    ui->deltaCellVolt_LV->setText("---   V");
    ui->minTemp_LV->setText("---  °C");
    ui->maxTemp_LV->setText("---  °C");
    ui->avgTemp_LV->setText("---  °C");
    ui->soc_LV->setText("---    %");
    ui->batteryVoltage_LV->setText("---    V");
    ui->current_LV->setText("---    A");
    ui->lvState->setText("---");

    QTreeWidgetItem *volts = ui->parameters_LV->topLevelItem(0);
    QTreeWidgetItem *openWire = ui->parameters_LV->topLevelItem(1);
    for (quint16 cell = 0; cell < LV_Accu::MAX_NUM_OF_LV_CELLS; cell++) {
        volts->child(0)->setText(cell+2, "---");
        openWire->child(0)->setText(cell+2, "---");
    }

    QTreeWidgetItem *temps = ui->parameters_LV->topLevelItem(2); //Temperatures
    for (quint16 tempsens = 0; tempsens < LV_Accu::MAX_NUM_OF_LV_TEMPSENS; tempsens++) {
        temps->child(0)->setText(tempsens + 1, "---");
    }

    ui->lvAccuStatus->setStyleSheet("");
    ui->lvSoc->setStyleSheet("");
    ui->lvSoc->setToolTip("");
    ui->lvTemperature->setStyleSheet("");
    ui->lvTemperature->setToolTip("");
}

void MainWindow::update_ui_lv(LV_Accu::lv_battery_data_t data)
{
    lvBatteryData = data;

    if (lvBatteryData.voltageValid) {
        ui->minCellVolt_LV->setText(QString("%1 V").arg(lvBatteryData.minCellVolt, 5, 'f', 3));
        ui->maxCellVolt_LV->setText(QString("%1 V").arg(lvBatteryData.maxCellVolt, 5, 'f', 3));
        ui->avgCellVolt_LV->setText(QString("%1 V").arg(lvBatteryData.avgCellVolt, 5, 'f', 3));
        float delta = lvBatteryData.maxCellVolt - lvBatteryData.minCellVolt;
        ui->deltaCellVolt_LV->setText(QString("%1 V").arg(delta, 5, 'f', 3));
    } else {
        ui->minCellVolt_LV->setText("Invalid");
        ui->maxCellVolt_LV->setText("Invalid");
        ui->avgCellVolt_LV->setText("Invalid");
        ui->deltaCellVolt_LV->setText("Invalid");
    }

    if (lvBatteryData.tempValid) {
        ui->minTemp_LV->setText(QString("%1 °C").arg(lvBatteryData.minTemp, 4, 'f', 1));
        ui->maxTemp_LV->setText(QString("%1 °C").arg(lvBatteryData.maxTemp, 4, 'f', 1));
        ui->avgTemp_LV->setText(QString("%1 °C").arg(lvBatteryData.avgTemp, 4, 'f', 1));

        if (lvBatteryData.maxTemp <= 25) {
            ui->lvTemperature->setStyleSheet("image: url(:/img/res/temp-cold.svg);");
        } else if (lvBatteryData.maxTemp > 25 && lvBatteryData.maxTemp <= 50) {
            ui->lvTemperature->setStyleSheet("image: url(:/img/res/temp-mid.svg);");
        } else {
            ui->lvTemperature->setStyleSheet("image: url(:/img/res/temp-hot.svg);");
        }
        ui->lvTemperature->setToolTip(QString("%1 °C").arg(lvBatteryData.maxTemp, 4, 'f', 1));

    } else {
        ui->minTemp_LV->setText("Invalid");
        ui->maxTemp_LV->setText("Invalid");
        ui->avgTemp_LV->setText("Invalid");
        ui->lvTemperature->setStyleSheet("");
        ui->lvTemperature->setToolTip("");
    }

    QTreeWidgetItem *volts = ui->parameters_LV->topLevelItem(0);
    QTreeWidgetItem *openWire = ui->parameters_LV->topLevelItem(1);
    for (quint16 cell = 0; cell < LV_Accu::MAX_NUM_OF_LV_CELLS; cell++) {
        volts->child(0)->setText(cell+2, QString::number(lvBatteryData.cellVoltage[cell], 'f', 4));
        openWire->child(0)->setText(cell+2, LV_Accu::sensor_status_to_string(lvBatteryData.cellVoltageStatus[cell+1]));
    }
    openWire->child(0)->setText(1, LV_Accu::sensor_status_to_string(lvBatteryData.cellVoltageStatus[0]));

    ::memset(lvBatteryData.cellVoltage, 0, LV_Accu::MAX_NUM_OF_LV_CELLS * sizeof(float));

    QTreeWidgetItem *temps = ui->parameters_LV->topLevelItem(2); //Temperatures
    for (quint16 tempsens = 0; tempsens < LV_Accu::MAX_NUM_OF_LV_TEMPSENS; tempsens++) {
        if (lvBatteryData.temperatureStatus[tempsens] == LV_Accu::NOERROR){
            temps->child(0)->setText(tempsens + 1, QString::number(lvBatteryData.temperature[tempsens], 'f', 1));
        } else {
            temps->child(0)->setText(tempsens + 1, LV_Accu::sensor_status_to_string(lvBatteryData.temperatureStatus[tempsens]));
        }
    }
    ::memset(lvBatteryData.temperature, 0, LV_Accu::MAX_NUM_OF_LV_TEMPSENS * sizeof(float));

    if (lvBatteryData.socValid) {
        ui->soc_LV->setText(QString("    %1 %").arg(lvBatteryData.soc));

        if (lvBatteryData.soc <= 10) {
            ui->lvSoc->setStyleSheet("image: url(:/img/res/battery-empty.svg);");
        } else if (lvBatteryData.soc > 10 && lvBatteryData.soc <= 40) {
            ui->lvSoc->setStyleSheet("image: url(:/img/res/battery-almost-empty.svg);");
        } else if (lvBatteryData.soc > 40 && lvBatteryData.soc <= 70) {
            ui->lvSoc->setStyleSheet("image: url(:/img/res/battery-almost-full.svg);");
        } else {
            ui->lvSoc->setStyleSheet("image: url(:/img/res/battery-full.svg);");
        }
        ui->lvSoc->setToolTip(QString("%1 %").arg(lvBatteryData.soc));

    } else {
        ui->soc_LV->setText("Invalid");
        ui->lvSoc->setStyleSheet("");
        ui->lvSoc->setToolTip("");
    }

    if (lvBatteryData.batteryVoltageValid) {
        ui->batteryVoltage_LV->setText(QString("%1 V").arg(lvBatteryData.batteryVoltage, 6, 'f', 1));
    } else {
        ui->batteryVoltage_LV->setText("Invalid");
    }

    if (lvBatteryData.currentValid) {
        ui->current_LV->setText(QString("%1 A").arg(lvBatteryData.current, 6, 'f', 2));
    } else {
        ui->current_LV->setText("Invalid");
    }

    ui->lvState->setText(LV_Accu::lv_state_to_string(lvBatteryData.state));

    if (lvBatteryData.errorCode == LV_Accu::ERROR_NO_ERROR) {
        ui->lvAccuStatus->setStyleSheet("border-image: url(:/img/res/circle-check.svg);");
    } else {
        ui->lvAccuStatus->setStyleSheet("border-image: url(:/img/res/circle-warning.svg);");
    }

}

void MainWindow::lv_state_changed(LV_Accu::lv_state_t state, LV_Accu::contactor_error_t)
{

}

void MainWindow::on_lvToggleNotification_clicked()
{
    if (lvNotifyOnErrors) {
        lvNotifyOnErrors = false;
        ui->lvToggleNotification->setStyleSheet("image: url(:/img/res/no-bell.svg);");
    } else {
        lvNotifyOnErrors = true;
        ui->lvToggleNotification->setStyleSheet("image: url(:/img/res/bell.svg);");
    }
}
