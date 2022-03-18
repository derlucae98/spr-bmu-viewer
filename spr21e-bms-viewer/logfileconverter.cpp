#include "logfileconverter.h"
#include "ui_logfileconverter.h"

LogfileConverter::LogfileConverter(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LogfileConverter)
{
    ui->setupUi(this);
    ui->status->clear();
    ui->progress->setValue(0);
    ui->progress->setDisabled(true);
    ui->outputPath->setDisabled(true);
    ui->btnOutputFile->setDisabled(true);
    ui->btnConvert->setDisabled(true);

}

LogfileConverter::~LogfileConverter()
{
    delete ui;
}

LogfileConverter::logging_data_t LogfileConverter::decode_line(QByteArray &inputLine)
{
    QByteArray raw = QByteArray::fromBase64(inputLine, QByteArray::Base64Encoding);
    logging_data_t data;
    char *dataPtr = (char*) &data;
    for (int i = 0; i < raw.length(); i++) {
        *dataPtr++ = raw.at(i);
    }
    return data;
}

QString LogfileConverter::raw_to_csv(logging_data_t &raw)
{
    QString ret;
    ret.append(QString("%1-%2-%3 %4:%5:%6;").arg(raw.timestamp.year, 4, 10, QLatin1Char('0'))
                                           .arg(raw.timestamp.month, 2, 10, QLatin1Char('0'))
                                           .arg(raw.timestamp.day, 2, 10, QLatin1Char('0'))
                                           .arg(raw.timestamp.hour, 2, 10, QLatin1Char('0'))
                                           .arg(raw.timestamp.minute, 2, 10, QLatin1Char('0'))
                                           .arg(raw.timestamp.second, 2, 10, QLatin1Char('0')));

    for (size_t stack = 0; stack < 12; stack++) {
        for (size_t cell = 0; cell < 12; cell++) {
            ret.append(QString("%1;").arg((float)raw.cellVoltage[stack][cell] * 0.001f, 5, 'f', 3, QLatin1Char('0')));
        }
    }

    for (size_t stack = 0; stack < 12; stack++) {
        for (size_t tempsen = 0; tempsen < 14; tempsen++) {
            ret.append(QString("%1;").arg((float)raw.temperature[stack][tempsen] * 0.1f, 4, 'f', 1, QLatin1Char('0')));
        }
    }
    ret.append(QString("%1;").arg(raw.current, 6, 'f', 2, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.batteryVoltage, 5, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.dcLinkVoltage, 5, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.minCellVolt * 0.001f, 5, 'f', 3, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.maxCellVolt * 0.001f, 5, 'f', 3, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.avgCellVolt * 0.001f, 5, 'f', 3, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.minTemperature * 0.1f, 4, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.maxTemperature * 0.1f, 4, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.avgTemperature * 0.1f, 4, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1;").arg(raw.stateMachineState, 1, 10));
    ret.append(QString("%1;").arg(raw.stateMachineError, 1, 10));
    ret.append(QString("%1;").arg(raw.minSoc * 0.1f, 5, 'f', 1, QLatin1Char('0')));
    ret.append(QString("%1").arg(raw.maxSoc * 0.1f, 5, 'f', 1, QLatin1Char('0')));
    return ret;
}

QString LogfileConverter::get_header()
{
    QString header;
    header.append("Logfile created by Battery Management Unit of SPR22e - Scuderia Mensa\r\n");
    header.append("Timestamp;");
    for (size_t i = 0; i < 144; i++) {
        header.append(QString("Cell %1;").arg(i+1));
    }
    for (size_t i = 0; i < 168; i++) {
        header.append(QString("Temperature %1;").arg(i+1));
    }
    header.append("Current;");
    header.append("Battery voltage;");
    header.append("DC-Link voltage;");
    header.append("Min cell voltage;");
    header.append("Max cell voltage;");
    header.append("Avg cell voltage;");
    header.append("Min temperature;");
    header.append("Max temperature;");
    header.append("Avg temperature;");
    header.append("State machine state;");
    header.append("State machine error;");
    header.append("Min SOC;");
    header.append("Max SOC");
    return header;
}

void LogfileConverter::on_btnInputFile_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open logfile", QDir::homePath(), "Logfiles (*.log)");
    ui->inputPath->setText(fileName);
    if (!fileName.isEmpty()) {
        ui->outputPath->setDisabled(false);
        ui->btnOutputFile->setDisabled(false);
    }
}


void LogfileConverter::on_btnOutputFile_clicked()
{
    //Remove file extension
    QFileInfo fileInfo(ui->inputPath->text());
    QString path = QDir(fileInfo.absolutePath()).filePath(fileInfo.baseName());
    path.append(".csv");

    QString fileName = QFileDialog::getSaveFileName(this, "Save CSV file", path, "CSV files (*.csv)");

    ui->outputPath->setText(fileName);
    if (!fileName.isEmpty()) {
        ui->btnConvert->setDisabled(false);
    }
}


void LogfileConverter::on_btnConvert_clicked()
{
    ui->status->clear();
    ui->progress->setDisabled(false);

    //Open input file
    QFile inputFile(ui->inputPath->text());
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox mb;
        mb.setText("Cannot open input file!");
        mb.exec();
        return;
    }

    //Open output file
    QFile outputFile(ui->outputPath->text());
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox mb;
        mb.setText("Cannot open output file!");
        mb.exec();
        return;
    }

    qint64 size = inputFile.size();
    qint64 numberOfLines = size / 885; //885 characters per line
    quint64 currentLine = 0;
    ui->progress->setMaximum(numberOfLines);
    QTextStream stream(&outputFile);
    stream << get_header() << Qt::endl;
    while (!inputFile.atEnd()) {
        QByteArray line = inputFile.readLine();
        logging_data_t data = decode_line(line);
        QString csv = raw_to_csv(data);
        stream << csv << Qt::endl;
        ui->progress->setValue(++currentLine);
    }
    inputFile.close();
    outputFile.close();
    ui->status->setText("Done.");
}

