#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qmath.h>
#include <QMainWindow>
#include <QResizeEvent>
#include <QString>

#include <QPointF>
#include <QRectF>

#include <QElapsedTimer>

#include <QTime>
#include <QTimer>

#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>



#include <QMessageBox>

#include <QPainter>
#include <QColor>
#include "qpaintbox.h"


#define BOTTOM  0
//#define HEIGHT  ui->widget->height
//#define WIDTH   ui->widget->width


const float gravity = -9.81;

typedef struct {
    uint8_t diameter;
    QColor pathColor;
    float posX;
    float posY;
    float vel;
    float velX;
    float velY;
    float alpha;
} _sBallData;

typedef union{
    uint8_t     u8[4];
    int8_t      i8[4];
    uint16_t    u16[2];
    int16_t     i16[2];
    uint32_t    u32;
    int32_t     i32;
    float       f;
}_work;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTimer();

    //cambiamos el nombre porque necesito el evento del qpaintbox
    void onMousePressEvent(QMouseEvent *event);
    void onMouseReleaseEvent(QMouseEvent *event);
    void onMouseMoveEvent(QMouseEvent *event);

    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event);

    void on_startButton_clicked();
    void on_cleanButton_clicked();
    void on_openButton_clicked();
    void on_sendButton_clicked();

    void OnRxChar();
private:
    Ui::MainWindow *ui;
    QPaintBox *QPaintBox1;
    QSerialPort *QSerialPort1;

    QTimer *timer;
    QPen pen;
    QBrush brush;

//    QMouseEvent mouseData;

    float myTime;
    int32_t oldWidth, oldHeight;

    _sBallData ballData;
    _sBallData initBallData;
    _sBallData lastBallData;

    uint8_t rxBuf[256], header, nBytes, cks, index, tmoRX;

    void SendCMD(uint8_t *buf, uint8_t length);

    void DecodeCmd(uint8_t *rxBuf);


    typedef enum{
        ALIVE=0xF0,
        FIRMWARE=0xF1,

        LEDS=0x10,

        PULSADORES=0x12,

        DEGREES = 0x13,
        VELOCITY = 0x14,
        POSITION=0x15,

        LAUNCHSTATE = 0x16,

        ACK = 0x17,
        WALLBOUNCE=0x18,

        ACKNOWLEDGE=0x0D,
        /*
        UNKNOWNCOMANND=0xFF
*/
    }_eIDCommand;

    //variables booleanas
    bool onPath = false;
    bool onExe = false;
    bool firExe = true;
    bool isMoved = false;
    bool mouseIsPressed;
   // const char firmware[] = "EX100923.01";


};
#endif // MAINWINDOW_H
