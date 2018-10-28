#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include <string>
using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_OpenSerialButton_clicked();

    void ReadData();

    void on_SendButton_clicked();

    void calData();
    string bufToString(QByteArray buf);

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    // for example: 10591300
    string bufStr;
    // 10 59 13
    int previous[3] = {0}, current[3] = {0}, difference[3] = {0};
    // 13.5910
    double previousDisplacement, currentDisplacement, differenceDisplacement;
    string displayString;
    bool dataUpdated;
    bool headerPrinted = false;
    bool dataStarted = false;
    int sizeRead = 0;
    // 存储 4 个 16 进制数
    QByteArray size4Buf = QByteArray();
};

#endif // MAINWINDOW_H