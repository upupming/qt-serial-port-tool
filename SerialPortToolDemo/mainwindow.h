#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QTimer>
#include <QDateTime>


#include <QMainWindow>
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include"xlsxdocument.h"
#include <queue>

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
    double getMaxMinDisplacementDiff();

    void on_ExportButton_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    // for example: 10591300
    string bufStr;
    // 10 59 13
    int previous[3] = {0}, current[3] = {0}, difference[3] = {0};
    // 13.5910
    double currentDisplacement, differenceDisplacement;

    // Save all displacements in 10s as max heap and min heap
    int periodSeconds = 10;
    // displacementMaxHeaps[0] saves displacements in 0 - 9
    // displacementMaxHeaps[1] saves displacements in 1 - 10
    // ...
    // displacementMaxHeaps[9] saves displacements in 9 - 18
    // displacementMaxHeaps[10] saves displacements in 10 - 19
    // ...
    // at time = 9, use displacementMaxHeaps[0] for displacement in 0 - 9
    // at time = 10, use displacementMaxHeaps[1] for displacement in 1 - 10
    // if time = 55, I have to use displacementMaxHeaps[46] for displacement in 46 - 55
    // so at time = 55, we don't need displacementMaxHeaps[0] - displacementMaxHeaps[45] any more, so we free them in this way:
    // at time n+10, free displacementMaxHeaps[n]
    // so we can use displacementMaxHeaps[n] again for displacementMaxHeaps[n+10]
    // That means displacementMaxHeaps[0] will be free when time = 10, and be used to store displacementMaxHeaps[10]
    // So we only need a array(displacementMaxHeaps) of size 10
    // And displacementMaxHeaps[0] saves displacements in 0 - 9; 10 - 19; ...
    priority_queue<double> displacementMaxHeaps[10];
    priority_queue<double, std::vector<double>, std::greater<double>> displacementMinHeaps[10];
    bool beginTimeInited = false;
    QTime beginTime;
    int timeSinceBegin = 0;

    string displayString;
    QString value;
    int NUM,sz,m,l;
    QString NUM1;
    QString NUM2;
    int i=2;
    bool dataUpdated;
    bool headerPrinted = false;
    bool dataStarted = false;
    int sizeRead = 0;
    QByteArray size4Buf = QByteArray();

public slots:
   void timerUpdate(void);
};

#endif // MAINWINDOW_H