#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPort>
#include <QSerialPortInfo>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //查找可用的串口
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    reverse(ports.begin(), ports.end());
    foreach (const QSerialPortInfo &info, ports)
    {
        QSerialPort serial;
        serial.setPort(info);
        if(serial.open(QIODevice::ReadWrite))
        {
            ui->PortBox->addItem(serial.portName());
            serial.close();
        }
    }
    //设置波特率下拉菜单默认显示第0项
    ui->BaudBox->setCurrentIndex(0);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_OpenSerialButton_clicked()
{
    if(ui->OpenSerialButton->text() == tr("打开串口"))
    {
        serial = new QSerialPort;
        //设置串口名
        serial->setPortName(ui->PortBox->currentText());
        //打开串口
        serial->open(QIODevice::ReadWrite);
        //设置波特率
        serial->setBaudRate(ui->BaudBox->currentText().toInt());
        //设置数据位数
        switch (ui->BitBox->currentIndex())
        {
        case 8:
            serial->setDataBits(QSerialPort::Data8);
            break;
        default:
            break;
        }
        //设置校验位
        switch (ui->ParityBox->currentIndex())
        {
        case 0:
            serial->setParity(QSerialPort::NoParity);
            break;
        default:
            break;
        }
        //设置停止位
        switch (ui->BitBox->currentIndex())
        {
        case 1:
            serial->setStopBits(QSerialPort::OneStop);
            break;
        case 2:
            serial->setStopBits(QSerialPort::TwoStop);
        default:
            break;
        }
        //设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);

        //关闭设置菜单使能
        ui->PortBox->setEnabled(false);
        ui->BaudBox->setEnabled(false);
        ui->BitBox->setEnabled(false);
        ui->ParityBox->setEnabled(false);
        ui->StopBox->setEnabled(false);
        ui->OpenSerialButton->setText(tr("关闭串口"));
        //连接信号槽
        QObject::connect(serial,&QSerialPort::readyRead,this,&MainWindow::ReadData);
    }
    else
    {
        //关闭串口
        serial->clear();
        serial->close();
        serial->deleteLater();

        //恢复设置使能
        ui->PortBox->setEnabled(true);
        ui->BaudBox->setEnabled(true);
        ui->BitBox->setEnabled(true);
        ui->ParityBox->setEnabled(true);
        ui->StopBox->setEnabled(true);
        ui->OpenSerialButton->setText(tr("打开串口"));

    }

}

void MainWindow::calData() {
    dataUpdated = false;

    current[0] = stoi(bufStr.substr(0, 2));
    current[1] = stoi(bufStr.substr(2, 2));
    current[2] = stoi(bufStr.substr(4, 2));
    currentDisplacement = current[2] + current[1]/100.0 + current[0]/10000.0;

    // Cal difference
    if(previous[0] != 0 || previous[1] != 0 || previous[2] != 0) {
        difference[0] = current[0] - previous[0];
        difference[1] = current[1] - previous[1];
        difference[2] = current[2] - previous[2];
        differenceDisplacement = difference[2] + difference[1]/100.0 + difference[2]/10000.0;
    }

    previousDisplacement = previous[2] + previous[1]/100.0 + previous[0]/10000.0;
    previous[0] = current[0];
    previous[1] = current[1];
    previous[2] = current[2];

    dataUpdated = true;
    displayString = to_string(currentDisplacement) + '\t' + to_string(previousDisplacement) + '\t' + to_string(differenceDisplacement);
}

string MainWindow::bufToString(QByteArray buf) {
    bufStr = "";

    for (int i = 0; i < buf.size(); ++i) {
        stringstream ss;
        ss << setfill('0') << setw(2) << setbase(16) << (int)(buf.at(i));
        bufStr += ss.str();
    }

    return bufStr;
}

//读取接收到的信息
void MainWindow::ReadData()
{
    QByteArray buf;
    buf = serial->readAll();
    if(!buf.isEmpty())
    {
        if(!dataStarted) {
            // 假定数据起始 aa 到达时 buf.size() == 1
            if(bufToString(buf) == "aa") {
                cout << "Starting to accumulate buf..." << endl;
                dataStarted = true;
                sizeRead = 0;
            }
        } else {
            // 假定同一个 buf 里面不同时含有（两次测量的数据中的一部分）
            size4Buf += buf;
            sizeRead += buf.size();

            if(sizeRead == 4) {
                cout << "4 bytes [" << bufToString(size4Buf) << "] have been accumulated, starting to calculate data..." << endl;
                calData();
                if(dataUpdated) {
                    cout << "data updated: [" << current[0] << ", " << current[1] << ", " << current[2] << "]"<< endl;

                    QString str = ui->textEdit->toPlainText();

                    if(!headerPrinted) {
                        str = QString::fromStdString(string("Current") + "\t" + "Previous" + "\t" + "Difference" + "\n") + str;
                        headerPrinted = true;
                    }

                    str += QString::fromStdString(displayString + '\n');
                    ui->textEdit->clear();
                    ui->textEdit->append(str);
                }

                dataStarted = false;
                size4Buf = QByteArray();
            }
        }

        cout << "There are " << buf.size() << " data in this buf, they are: " << bufToString(buf) << endl;;

    }
    buf.clear();
}

//发送按钮槽函数
void MainWindow::on_SendButton_clicked()
{
    serial->write(ui->textEdit_2->toPlainText().toLatin1());
}