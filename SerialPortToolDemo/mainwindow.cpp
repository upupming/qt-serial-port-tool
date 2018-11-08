#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qmessagebox.h"
#include<math.h>
#include<QPalette>
#include<chrono>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include"xlsxdocument.h"
#include <queue>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerUpdate()));
    timer->start(1000);

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
void MainWindow::timerUpdate(void)
{
    QDateTime time = QDateTime::currentDateTime();
    QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");
    ui->timer->setText(str);
    value = time.toString("yyyy-MM-dd");
}

void MainWindow::on_OpenSerialButton_clicked()
{
    if(ui->OpenSerialButton->text() == tr("打开串口"))
    {
        // Set color
        ui->pushButton->setStyleSheet("QPushButton{background-image: url(:/new/prefix1/red.png);}");
        beginTimeInited = false;
        timeSinceBegin = 0;
        for(int i=0; i<10; i++) {
            priority_queue<double> emptyMaxHeap;
            priority_queue<double, std::vector<double>, std::greater<double>> emptyMinHeap;
            swap(displacementMaxHeaps[i], emptyMaxHeap);
            swap(displacementMinHeaps[i], emptyMinHeap);
        }

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

double MainWindow::getMaxMinDisplacementDiff() {
    if(timeSinceBegin < 9) {
        // No enough data yet, return 0
        return 0;
    } else {
        // At time = 9, we return displacementMaxHeaps[0] - displacementMinHeaps[0]
        // At time = 19, we return displacementMaxHeaps[0] - displacementMinHeaps[0], too
        // It is also to say that timeSinceBegin must >=9
        cout << timeSinceBegin << "   " << (timeSinceBegin-9)%10 << endl;
        return displacementMaxHeaps[(timeSinceBegin-9)%10].top() - displacementMinHeaps[(timeSinceBegin-9)%10].top();
    }
}

void MainWindow::calData() {
    dataUpdated = false;

    current[0] = stoi(bufStr.substr(0, 2));
    current[1] = stoi(bufStr.substr(2, 2));
    current[2] = stoi(bufStr.substr(4, 2));
    currentDisplacement = current[2] + current[1]/100.0 + current[0]/10000.0;

    // ######### Add to heaps for future query
    // time = 5, save to displacementMaxHeaps[0] - displacementMaxHeaps[5]
    // time = 0, save to displacementMaxHeaps[0] only
    // time = 1, save to displacementMaxHeaps[0] and displacementMaxHeaps[1]
    // time = 10, save to displacementMaxHeaps[1] - displacementMaxHeaps[10]
    // time = 11, save to displacementMaxHeaps[2] - displacementMaxHeaps[11]
    if(timeSinceBegin >= 10) {
        for(int j = timeSinceBegin - 9; j<=timeSinceBegin; j++) {
            displacementMaxHeaps[j%10].push(currentDisplacement);
            displacementMinHeaps[j%10].push(currentDisplacement);
        }
    } else {
        for(int j=0; j<=timeSinceBegin; j++) {
            displacementMaxHeaps[j].push(currentDisplacement);
            displacementMinHeaps[j].push(currentDisplacement);
        }
    }

    displayString = to_string(currentDisplacement) + '\t' + "Not calculated" + '\t' + to_string(timeSinceBegin) + '\t';

    // Calculate Max-Min diff only per second
    QTime now = QTime::currentTime();
    if(!beginTimeInited) {
        beginTime = now;
        timeSinceBegin = 0;
        beginTimeInited = true;
    } else {
        int msPassed = beginTime.msecsTo(now);
        cout << msPassed << endl;
        if(msPassed / 1000 > timeSinceBegin) {
            timeSinceBegin = msPassed / 1000;
            differenceDisplacement = getMaxMinDisplacementDiff();
            displayString = to_string(currentDisplacement) + '\t' + to_string(differenceDisplacement) + "\t\t" + to_string(timeSinceBegin) + '\t';

            if(timeSinceBegin >= 10) {
                // Clean heaps
                priority_queue<double> emptyMaxHeap;
                priority_queue<double, std::vector<double>, std::greater<double>> emptyMinHeap;
                swap(displacementMaxHeaps[timeSinceBegin%10], emptyMaxHeap);
                swap(displacementMinHeaps[timeSinceBegin%10], emptyMinHeap);

                // Change color
                if(differenceDisplacement < 0.01) {
                    ui->pushButton->setStyleSheet("QPushButton{background-image: url(:/new/prefix1/green.png);}");
                } else {
                    ui->pushButton->setStyleSheet("QPushButton{background-image: url(:/new/prefix1/red.png);}");
                }
            }
        }
    }



    dataUpdated = true;
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
            if(bufToString(buf) == "aa") {
                cout << "Starting to accumulate buf..." << endl;
                dataStarted = true;
                sizeRead = 0;
            }
        }
        else
        {
            size4Buf += buf;
            sizeRead += buf.size();

            if(sizeRead == 4) {
                cout << "4 bytes [" << bufToString(size4Buf) << "] have been accumulated, starting to calculate data..." << endl;
                calData();
                if(dataUpdated) {
                    cout << "data updated: [" << current[0] << ", " << current[1] << ", " << current[2] << "]"<< endl;

                    QString str = ui->textEdit->toPlainText();

                    if(!headerPrinted) {
                        str = QString::fromStdString(string("Current") + "\t"  + "Difference" + "\t"  + "timeSinceBegin"+ "\t" + "\n") + str;
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

void MainWindow::on_ExportButton_clicked()
{

    QXlsx::Document xlsx("data.xlsx");
    NUM1= ui->lineEdit->text();
    NUM2= ui->lineEdit_2->text();
       //![0]
       //![1]
       xlsx.write("A1", " 日期 ");
       xlsx.write("B1", "设备型号");
       xlsx.write("C1", " 设备序列号 ");
       xlsx.write("D1", true);
       xlsx.write("E1", "位移数据");
       xlsx.write("F1", "差值数据");
       xlsx.write(i,1, value);
      // xlsx.write("B2", "WT-X");
     //  xlsx.write("C2", " 001 ");
       xlsx.write(i,2, NUM1);
       xlsx.write(i,3, NUM2);
       xlsx.write(i,4, true);
       xlsx.write(i,5, currentDisplacement);
       xlsx.write(i,6, differenceDisplacement);
       //![1]
       //![2]

       xlsx.save();
       i++;
       //![2]

}
