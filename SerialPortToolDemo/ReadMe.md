# SerialPortToolDemo

2017.12.6      
v1.0.0    
串口工具 Demo  
基于 Qt    

## 代码剖析

直接跳转到[乱码分析及解决方案](#数据处理)。

### 初始化

`main.cpp` 入口：

```cpp
MainWindow w;
w.show();
```

创建了一个 `MainWindow` 对象，并调用 `show` 方法。

看一看 `mainwindow.cpp`，没有定义  `show` 方法，看来是父类 `QMainWindow` 的方法，我们查一下官方文档 http://doc.qt.io/qt-5/qwindow.html#show：

```
Shows the window.

This is equivalent to calling showFullScreen(), showMaximized(), or showNormal(), depending on the platform's default behavior for the window type and flags.
```

只是单纯的创建一个窗口，窗口中的组件有哪些的具体看 `mainwindow.cpp` 的方法。

看到第 7 - 9 行：

```cpp
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
```

使用成员变量初始化（参见 [Class](http://www.cplusplus.com/doc/tutorial/classes/)），将 `mainwindow.h` 中第 28 - 30 行定义的私有变量 `ui` 和 `serial` 初始化为父级组件 `parent` 和一个新创建的 `ui::MainWindow`。

###查找可用串口并在 `ui` 中展示

14 行开始一个 [`foreach` 循环](http://doc.qt.io/archives/qt-4.8/containers.html#the-foreach-keyword)，将 `QSerialPortInfo::availablePorts()` 中的每个端口 `info` 进行创建 `QSerialPort` 并开启 `QIODevice::ReadWrite` 模式，成功之后在 `ui` 中的 `PortBox` 中加入一个端口名称的元素。

最后循环结束，设置波特率下拉菜单默认显示第 0 项。

### 点击开启串口按钮

后面有两个方法，`on_OpenSerialButton_clicked` 和 `ReadData`，前者调用后者。从名字上可以看出 `on_OpenSerialButton_clicked` 监听用户『点击开启串口按钮』的的事件并进行相应操作，具体分析见下。

第 36 行通过 `if` 判断点击的按钮内容，如果是 `打开串口`，那么就打开，否则就会关闭串口。下面只分析打开串口的代码。

从 38 行开始新建串口，设置串口名为 `ui` -> `PortBox` 中选中的文本，打开串口，设置波特率为 `ui` -> `BuadBox` 中提供的文本。

在 46 行开始，如果 `ui` -> `BitBox` 中选中的索引为 8，就会设置端口的数据位数为 `QSerialPort::Data8`，否则什么也不做。

在 55 行开始，如果 `ui` -> `ParityBox` 中选中的索引为 0，就会设置端口的校验位为 `QSerialPort::NoParity`，否则什么也不做。

在 64 行开始，如果 `ui` -> `BitBox` 中选中的索引为 1、2，就会设置端口的停止位为 `QSerialPort::OneStop`、`QSerialPort::TwoStop`，否则什么也不做。

在 75 行，设置控制流为 `QSerialPort::NoFlowControl`。

在 77 - 83 行，关闭设置菜单使能（禁用/变灰色）。然后将按钮的文本改为 `关闭串口`，并等待下一次点击，下一次点击时会运行 `else` 一段的关闭逻辑代码。

最后在 86 行，运行 [QObject::connect](http://doc.qt.io/qt-5/qobject.html#connect-5)

```cpp
QMetaObject::Connection 
QObject::connect(
    const QObject *sender, 
    PointerToMemberFunction signal, 
    const QObject *context, 
    Functor functor, 
    Qt::ConnectionType type = Qt::AutoConnection)
```

> Creates a connection of a given `type` from `signal` in `sender` object to `functor` to be placed in a specific event loop of `context`, and returns a handle to the connection.

意即创建一个从 `serial` 的 `&QSerialPort::readyRead` 信号到本窗口的连接，使用 `ReadData` 进行数据处理。具体分析见下一节。

### 数据处理

在 113 行调用 `serial->readAll()` 获取数据，看看[官方文档](http://doc.qt.io/qt-5/qiodevice.html#readAll)写到：

> Reads all remaining data from the device, and returns it as a byte array.

> This function has no way of reporting errors; returning an empty QByteArray can mean either that no data was currently available for reading, or that an error occurred.

<details><summary>关于 tr 函数</summary>
后面还有一个 `tr` 需要做一下说明：

> the tr function is the central part of the Qt internationalization system (allso called "i18n" by people who can't handle words with more than ten letters). That's a system for making your application available in multiple languages.
> If you want your application to have multiple language support, wrap every user-visible string in your code inside a tr() function. Then Qt will use the appropriate translated string in a different language environment. Of course Qt won't actually translate for you, you (or your translator guy) have to do that with QtLinguist.
</details>

可以看到 `buf` 实际是数据的字节（[QByteArray](http://doc.qt.io/qt-5/qbytearray.html)）表示，为了将字节转换为字符，我们可以人工查阅 ASCII 码表：

![ASCII](https://upload.wikimedia.org/wikipedia/commons/thumb/1/1b/ASCII-Table-wide.svg/875px-ASCII-Table-wide.svg.png)

当然是骗你的，我们不可能一个一个查的啦 :stuck_out_tongue_closed_eyes:

通过查阅[资料](https://stackoverflow.com/questions/14131127/qbytearray-to-qstring)发现，可以使用 [`QTextCodec`](http://doc.qt.io/qt-5/qtextcodec.html#codecForMib) 完成：

```cpp
QString strbuf = QTextCodec::codecForMib(1015)->toUnicode(buf);
// (1015 is UTF-16, 1014 UTF-16LE, 1013 UTF-16BE, 106 UTF-8)
```

修改一下第 114 到 122 行即可：

```cpp
if(!buf.isEmpty())
    {
        // 转换成字符串
        QString strbuf = QTextCodec::codecForMib(1015)->toUnicode(buf);  
        QString str = ui->textEdit->toPlainText();
        // 添加此字符串
        str+=tr(strbuf);
        ui->textEdit->clear();
        ui->textEdit->append(str);

    }
    // 这句貌似是多余的，删掉吧
    buf.clear();
```

## 总结

一下子读了整个项目，其实对于想快速熟悉一个项目来说完全没有必要，下次剖析代码时要是可以运行起来就好，也只需着重关注字符串类型的变量并打印出来观察其形式。