#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSerialPort1 = new QSerialPort(this);

    ui->portComBox->installEventFilter(this);

    ui->commandComBox->addItem("ALIVE", 0xF0);
    ui->commandComBox->addItem("FIRMWARE", 0xF1);
    ui->commandComBox->addItem("LEDS", 0x10);
    ui->commandComBox->addItem("PULSADORES", 0x12);
    ui->commandComBox->addItem("GRADOS",0x13);
    ui->commandComBox->addItem("VELOCIDAD", 0x14);
    ui->commandComBox->addItem("POSICION",0x15);
    ui->commandComBox->addItem("LANZAMIENTO",0x16);
    ui->commandComBox->addItem("MODIFICACION",0x17);
    ui->commandComBox->addItem("REBOTES", 0x18);


    QPaintBox1 = new QPaintBox(0, 0, ui->widget);

    timer = new QTimer(this);
    timer -> start(10); //refrescamos cada 10 ms

    //mediante los connects, vinculamos los slots y las señales
    connect(timer, &QTimer::timeout,this,&MainWindow::onTimer);

    connect(QPaintBox1,&QPaintBox::OnMousePress,this,&MainWindow::onMousePressEvent);
    connect(QPaintBox1,&QPaintBox::OnMouseRelease,this,&MainWindow::onMouseReleaseEvent);
    connect(QPaintBox1,&QPaintBox::OnMouseMove,this,&MainWindow::onMouseMoveEvent);

    connect(QSerialPort1,&QSerialPort::readyRead, this,&MainWindow::OnRxChar);

    //inicializamos variables

    header = 0;
    myTime = 0;

    //ballData.posX = ballData.diameter; //inicializamos con el diametro debido a que despues realizo una resta con el alto
    //ballData.posY = ballData.diameter;
    ballData.diameter = 30;
    ballData.velX = 0;
    ballData.velY = 0;
    ballData.alpha = 0;

    //utilizado en las formulas
    initBallData.posX = 0;
    initBallData.posY = ui->widget->height() - ballData.diameter;
    initBallData.velX = 0;
    initBallData.velY = 0;

    //usado para pintar el camino
    lastBallData.posX = 0;
    lastBallData.posY = ui->widget->width() - ballData.diameter;
    lastBallData.velX = 0;
    lastBallData.velY = 0;

}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event){ //utilizado para mostrar los puestos disponibles
    if(watched == ui->portComBox) {
        if (event->type() == QEvent::MouseButtonPress) {
            ui->portComBox->clear();
            QSerialPortInfo SerialPortInfo1;

            for(int i=0;i<SerialPortInfo1.availablePorts().count();i++)
                ui->portComBox->addItem(SerialPortInfo1.availablePorts().at(i).portName());

            return QMainWindow::eventFilter(watched, event);
        }
        else {
            return false;
        }
    }
    else{
        return QMainWindow::eventFilter(watched, event);
    }
}

void MainWindow::OnRxChar(){
    int count;
    uint8_t *buf;
    QString strHex;

    count = QSerialPort1->bytesAvailable();
    if(count <= 0)
        return;

    buf = new uint8_t[count];
    QSerialPort1->read((char *)buf, count);

    //Datos recibidos
    strHex = "<-- 0x";
    for (int a=0; a<count; a++) {
        strHex = strHex + QString("%1").arg(buf[a], 2, 16, QChar('0')).toUpper();
    }

    ui->plainTextEdit->appendPlainText(strHex);

    for (int i=0; i<count; i++) {
        strHex = strHex + QString("%1").arg(buf[i], 2, 16, QChar('0')).toUpper();

        switch(header){
        case 0:
            if(buf[i] == 'U'){
                header = 1;
                tmoRX = 5;
            }
            break;
        case 1:
            if(buf[i] == 'N')
                header = 2;
            else{
                header = 0;
                i--;
            }
            break;
        case 2:
            if(buf[i] == 'E')
                header = 3;
            else{
                header = 0;
                i--;
            }
            break;
        case 3:
            if(buf[i] == 'R')
                header = 4;
            else{
                header = 0;
                i--;
            }
            break;
        case 4:
            nBytes = buf[i];
            header = 5;
            break;
        case 5:
            if(buf[i] == ':'){
                header = 6;
                index = 0;
                cks = 'U' ^ 'N' ^ 'E' ^ 'R' ^ ':' ^ nBytes;
            }
            else{
                header = 0;
                i--;
            }
            break;
        case 6:
            nBytes--;
            if(nBytes > 0){
                rxBuf[index++] = buf[i];
                cks ^= buf[i];
            }
            else{
                header = 0;
                if(cks == buf[i])
                    DecodeCmd(rxBuf);
                else
                    ui->plainTextEdit->appendPlainText("ERROR CHECKSUM");
            }
            break;
        }
    }


    delete [] buf;

}

void MainWindow::DecodeCmd(uint8_t *rxBuf){
    QString str;

    switch (rxBuf[0]) {
        case LEDS:
            str = "LD3: ";
            if(rxBuf[1] & 0x08)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - LD2: ";
            if(rxBuf[1] & 0x04)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - LD1: ";
            if(rxBuf[1] & 0x02)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - LD0: ";
            if(rxBuf[1] & 0x01)
                str = str + "HIGH";
            else
                str = str + "LOW";

            rxBuf[1] = 0x08;


            ui->plainTextEdit->appendPlainText(str);
        break;
        case PULSADORES:
            str = "SW3: ";
            if(rxBuf[1] & 0x08)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW2: ";
            if(rxBuf[1] & 0x04)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW1: ";
            if(rxBuf[1] & 0x02)
                str = str + "HIGH";
            else
                str = str + "LOW";
            str = str + " - SW0: ";
            if(rxBuf[1] & 0x01)
                str = str + "HIGH";
            else
                str = str + "LOW";
            ui->plainTextEdit->appendPlainText(str);
        break;

        case ALIVE:
            if(rxBuf[1] == ACKNOWLEDGE)
                ui->plainTextEdit->appendPlainText("I'M ALIVE");
        break;

        case FIRMWARE:
                ui->plainTextEdit->appendPlainText("CORRECT VERSION");

        break;

        case POSITION:




            ui->plainTextEdit->appendPlainText(str);
        break;

        case LAUNCHSTATE:
        if(!onExe){
            onExe = true;
            ui->startButton->setText("STOP");

            myTime = 0;
            initBallData.velX = lastBallData.velX = cos(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
            initBallData.velY = lastBallData.velY = sin(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
            initBallData.posX = ballData.posX = ui->xPosLine->text().toFloat();

            //dependiendo de la forma en que se asigne le valor (sea teclado o moviendola con mouse) decidimos que hacer
            if(!isMoved){
                initBallData.posY = ballData.posY = ui->yPosLine->text().toFloat() + ballData.diameter; //restamos al total para que el punto de inicio se abajo a la izquierda
            } else{
                isMoved = false;
                initBallData.posY = ballData.posY = ui->widget->height() - ui->yPosLine->text().toFloat();
            }

            ballData.alpha = ui->alphaLine->text().toFloat();
            ballData.vel = ui->speedLine->text().toFloat();

        } else{
            ui->startButton->setText("START");
            onExe = false;
            onPath=false;
            firExe = true;

            myTime = 0;
            lastBallData.posX = 0;
            lastBallData.posY = 0;
        }

        break;


/*
        case UNKNOWNCOMANND:
            ui->plainTextEdit->appendPlainText("NO CMD");
        break;
*/



    }
}

void MainWindow::SendCMD(uint8_t *buf, uint8_t length){
    uint8_t tx[24];
    uint8_t cks, i;
    QString strHex;
    _work w;

    if(!QSerialPort1->isOpen())
        return;

    w.i32 = -1000;

    tx[7] = w.u8[0];
    tx[8] = w.u8[1];
    tx[9] = w.u8[2];
    tx[10] = w.u8[3];


    tx[0] = 'U';
    tx[1] = 'N';
    tx[2] = 'E';
    tx[3] = 'R';
    tx[4] = length + 1;
    tx[5] = ':';

    //copiamos los datos
    memcpy(&tx[6], buf, length);

    //generamos el checksum
    cks = 0;
    for (i=0; i<(length+6); i++) {
        cks ^= tx[i];
    }

    tx[i] = cks;


    //datos transmitidos
    strHex = "--> 0x";
    for (int i=0; i<length+7; i++) {
        strHex = strHex + QString("%1").arg(tx[i], 2, 16, QChar('0')).toUpper();
    }



    ui->plainTextEdit->appendPlainText(strHex);

    QSerialPort1->write((char *)tx, length+7);
}


void MainWindow::onMousePressEvent(QMouseEvent *event){
    if((event->button() == Qt::RightButton) && (event->pos().x() < ballData.posX + ballData.diameter) && (event->pos().x() > ballData.posX) && (event->pos().y() > ballData.posY) && (event->pos().y() < ballData.posY + ballData.diameter)){
        mouseIsPressed = true;
        ui->widget->setCursor(Qt::ClosedHandCursor); //cambiamos la apariencia del mouse
    }
}

void MainWindow::onMouseReleaseEvent(QMouseEvent *event){
    if(event->button() == Qt::RightButton){
        mouseIsPressed = false;
        ui->widget->setCursor(Qt::OpenHandCursor); //cambiamos la apariencia del mouse
    }
}

void MainWindow::onMouseMoveEvent(QMouseEvent *event){

    if(mouseIsPressed && onExe==false){
        isMoved = true;

        //mostramos el numero
        ui->xPosLine->setText(QString::number(event->pos().x()));
        ui->yPosLine->setText(QString::number(event->pos().y()));

        ballData.posX = event->pos().x(); //igualamos los valores de ballData.posX y posY para que la pelota se mueva
        ballData.posY = event->pos().y();

        initBallData.posX = event->pos().x();
        initBallData.posY = event->pos().y();

    }
}


void MainWindow::resizeEvent(QResizeEvent *event){
    QPaintBox1->resize(ui->widget->width(),ui->widget->height());

    QPaintBox1->getCanvas()->fill(Qt::black);

    QPaintBox1->update();
}


void MainWindow::paintEvent(QPaintEvent *event){
    Q_UNUSED(event);

    static bool first = false;

    if(!first){
        first = true;

        QPaintBox1->resize(ui->widget->width(), ui->widget->height());

        QPaintBox1->getCanvas()->fill(Qt::black);

        QPaintBox1->update();
    }

}



void MainWindow::on_startButton_clicked()
{
    if(ui->startButton->text()=="START"){

        onExe=true;
        ui->startButton->setText("STOP");

        myTime = 0;
        initBallData.velX = lastBallData.velX = cos(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
        initBallData.velY = lastBallData.velY = sin(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
        initBallData.posX = ballData.posX = ui->xPosLine->text().toFloat();

        //dependiendo de la forma en que se asigne le valor (sea teclado o moviendola con mouse) decidimos que hacer
        if(!isMoved){
            initBallData.posY = ballData.posY = ui->yPosLine->text().toFloat() + ballData.diameter; //restamos al total para que el punto de inicio se abajo a la izquierda
        } else{
            isMoved = false;
            initBallData.posY = ballData.posY = ui->widget->height() - ui->yPosLine->text().toFloat();
        }

        ballData.alpha = ui->alphaLine->text().toFloat();
        ballData.vel = ui->speedLine->text().toFloat();

    } else{

        ui->startButton->setText("START");
        onExe=false;
        onPath=false;
        firExe = true;

        myTime = 0;
        lastBallData.posX = 0;
        lastBallData.posY = 0;

    }

    QPainter paint(QPaintBox1->getCanvas());
    //limpiamos la pantalla
    brush.setColor(Qt::black);
    paint.fillRect(0,0,ui->widget->width(),ui->widget->height(),brush);


    if((ui->xPosLine->text().toInt() > ui->widget->width()-ballData.diameter) || (ui->yPosLine->text().toInt() > ui->widget->height()-ballData.diameter) || (ui->xPosLine->text().toInt() < 0) || (ui->yPosLine->text().toInt() < 0))
        onExe = false;

}


void MainWindow::on_cleanButton_clicked()
{
    ui->plainTextEdit->clear();
}


void MainWindow::on_openButton_clicked()
{
    if(QSerialPort1->isOpen()){
        QSerialPort1->close();
        ui->openButton->setText("OPEN");
    }
    else{
        if(ui->portComBox->currentText() == "")
            return;

        QSerialPort1->setPortName(ui->portComBox->currentText());
        QSerialPort1->setBaudRate(115200);
        QSerialPort1->setParity(QSerialPort::NoParity);
        QSerialPort1->setDataBits(QSerialPort::Data8);
        QSerialPort1->setStopBits(QSerialPort::OneStop);
        QSerialPort1->setFlowControl(QSerialPort::NoFlowControl);

        if(QSerialPort1->open(QSerialPort::ReadWrite)){
            ui->openButton->setText("CLOSE");
        }
        else
            QMessageBox::information(this, "Serial PORT", "ERROR. Opening PORT");
    }
}


void MainWindow::on_sendButton_clicked()
{
    uint8_t cmd, buf[24];
    //    _work w;
    int n;
    //    bool ok;
    QString strHex;


    if(ui->commandComBox->currentText() == "")
        return;

    cmd = ui->commandComBox->currentData().toInt();
    //mostramos el comando en el plainText
    ui->plainTextEdit->appendPlainText("0x" + (QString("%1").arg(cmd, 2, 16, QChar('0'))).toUpper());


    //dependiendo de la opcion elegida actuamos
    n = 0;
    switch (cmd) {
        case 0xF0://ALIVE   PC=>MBED 0xF0 ;  MBED=>PC 0xF0 0x0D
            n = 1;
        break;
        case 0xF1:
            n = 1;
        break;
        case 0x10:
            n = 2;
        break;
        case 0x12:
            n = 1;
        break;
        case 0x13:
            n = 3;
        break;
        case 0x14:
            n = 3;
        break;
        case 0x15:
            n = 3;
        break;
        case 0x16:
            n = 1;
        break;
        case 0x17:
            n = 3;
        break;
        case 0x18:
            n = 1;
        break;
    }

    if(n){ //entra siempre y cuando el valor de n es distinto de cero
        buf[0] = cmd;
        SendCMD(buf, n);
    }
}


void MainWindow::onTimer(){

    QPainter paint(QPaintBox1->getCanvas());
    brush.setStyle(Qt::SolidPattern); //determinamos que brush sea solido

    //tenemos este if para inicializar al posicion primero, porque en el constructor no se tiene la configuracion del widget al momento de la asignacion
    if(firExe){

        firExe = false;
        ballData.posX = 0;
        ballData.posY = ui->widget->height() - ballData.diameter;

    }


    if(onExe){

        //si cambiamos la velocidad actualizamos la posicion inicial, usado para cambiar la velodidad en ejecucion
        if(ballData.vel != ui->speedLine->text().toFloat()){
            ballData.vel = ui->speedLine->text().toFloat();
            //hacemos que la posicion inicial sea la posicion de la pelotita
            initBallData.posX = ballData.posX;
            initBallData.posY = ui->widget->height() - ballData.posY;
            //inicializamos el tiempo en 0
            myTime = 0;

            //condiciones para los signos
            if(initBallData.velX >= 0)
                initBallData.velX = 1;
            else
                initBallData.velX = -1;

            if(initBallData.velY >= 0)
                initBallData.velY = 1;
            else
                initBallData.velY = -1;

            //asiganmos el valor ingresado
            initBallData.velX *= cos(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
            initBallData.velY *= sin(qDegreesToRadians(ui->angleLine->text().toFloat())) * ui->speedLine->text().toFloat();
        }

        myTime += 0.01;

        ballData.velY = (initBallData.velY + gravity * myTime);
        ballData.alpha = ui->alphaLine->text().toFloat(); //cambiamos el valor del alpha constantemente


        // colisión pared derecha
        if ((ballData.posX > ui->widget->width()-ballData.diameter) && (initBallData.velX > 0)) {

            //recalculamos los datos de inicio debido a que es un nuevo tiro
            initBallData.posX = ui->widget->width()-ballData.diameter; //asignamos el valor maximo posible a la derecha
            initBallData.posY = ui->widget ->height() - ballData.posY;

            //usamos solo una variable de velX debido a que no cambia su valor como pasa con velY
            initBallData.velX = -1 * (initBallData.velX - (initBallData.velX * ballData.alpha * 0.01)); // calculo con la perdida de energia incluida
            initBallData.velY = ballData.velY - (ballData.velY * ballData.alpha * 0.01);

            myTime=0;

            // Agrega aquí la llamada para enviar el comando
            uint8_t cmdToSend = 0x18; // Reemplaza "0xYY" con el comando que deseas enviar
            uint8_t dataToSend = 0x01; // Reemplaza "0xZZ" con los datos que deseas enviar
            uint8_t buf[] = {cmdToSend, dataToSend};
            SendCMD(buf, sizeof(buf)); // Envía el comando con los datos



        }


        // colisión pared izquierda
        if ((ballData.posX < 0) && (initBallData.velX < 0)) {

            initBallData.posX = 0; //asignamos el valor maximo posible a la derecha
            initBallData.posY = ui->widget->height() - ballData.posY;

            initBallData.velX = -1 * (initBallData.velX - (initBallData.velX * ballData.alpha * 0.01));
            initBallData.velY = ballData.velY - (ballData.velY * ballData.alpha * 0.01);

            myTime=0;

            // Agrega aquí la llamada para enviar el comando
            uint8_t cmdToSend = 0x18; // Reemplaza "0xYY" con el comando que deseas enviar
            uint8_t dataToSend = 0x02; // Reemplaza "0xZZ" con los datos que deseas enviar
            uint8_t buf[] = {cmdToSend, dataToSend};
            SendCMD(buf, sizeof(buf)); // Envía el comando con los datos
        }


        // colisión pared inferior
        if ((ballData.posY > ui->widget->height()-ballData.diameter) && (ballData.velY < 0)) {

            initBallData.posY = ballData.diameter; // solo restamos ballData.diameter porque realizamos la resta con la altura en el calculo de posx
            initBallData.posX = ballData.posX;

            initBallData.velX = initBallData.velX - (initBallData.velX * ballData.alpha * 0.01);
            initBallData.velY =  -1 * (ballData.velY - (ballData.velY * ballData.alpha * 0.01));

            myTime = 0;

            // Agrega aquí la llamada para enviar el comando
            uint8_t cmdToSend = 0x18; // Reemplaza "0xYY" con el comando que deseas enviar
            uint8_t dataToSend = 0x08; // Reemplaza "0xZZ" con los datos que deseas enviar
            uint8_t buf[] = {cmdToSend, dataToSend};
            SendCMD(buf, sizeof(buf)); // Envía el comando con los datos


        }


        // colisión pared superior
        if ((ballData.posY < 0) && (ballData.velY > 0)) {
            initBallData.posY = ui->widget->height(); //asignamos este valor porque despues se resta con el otro
            initBallData.posX = ballData.posX;

            initBallData.velX = initBallData.velX - (initBallData.velX * ballData.alpha * 0.01);
            initBallData.velY = -1 * (ballData.velY - (ballData.velY * ballData.alpha * 0.01));

            myTime = 0;

            // Agrega aquí la llamada para enviar el comando
            uint8_t cmdToSend = 0x18; // Reemplaza "0xYY" con el comando que deseas enviar
            uint8_t dataToSend = 0x04; // Reemplaza "0xZZ" con los datos que deseas enviar
            uint8_t buf[] = {cmdToSend, dataToSend};
            SendCMD(buf, sizeof(buf)); // Envía el comando con los datos
        }


        if((ballData.velY <= 0.25) && (ballData.velY >= -0.25) && (initBallData.velX <= 0.25) && (initBallData.velX >= -0.25) && ballData.posY > ui->widget->height()-ballData.diameter-1){
            ui->startButton->setText("START");
            onExe=false;
            myTime = 0;
        }

        //pintamos o no el recorrido dependiendo de la decision del usuario
        if(onPath){
            brush.setColor(Qt::green);
            pen.setColor(Qt::green);
        } else{
            brush.setColor(Qt::black);
            pen.setColor(Qt::black);
        }

        if(ui->pathCheckBox->isChecked()){
            onPath = true;
        } else{
            onPath = false;
        }

        paint.setPen(pen);
        paint.setBrush(brush);
        paint.resetTransform();
        paint.translate(ballData.posX,ballData.posY);
        paint.drawEllipse(0,0,ballData.diameter,ballData.diameter);
        paint.save();

        ballData.posX  = (initBallData.posX + initBallData.velX * myTime);
        ballData.posY = ui->widget->height() - (initBallData.posY + initBallData.velY * myTime + 0.5 * gravity * myTime*myTime);

        pen.setColor(Qt::white);
        paint.setPen(pen);
        brush.setColor(Qt::white);
        paint.setBrush(brush);

        paint.resetTransform();
        paint.translate(ballData.posX,ballData.posY);
        paint.drawEllipse(0,0,ballData.diameter,ballData.diameter);
        paint.save();

        //mostramos los datos

        //ui->speedLine->setText(QString::number(initBallData.velX / cos(qDegreesToRadians(ui->angleLine->text().toFloat()))));
        ui->xPosLine->setText(QString::number(ballData.posX));
        ui->yPosLine->setText(QString::number(ballData.posY));

    } else{

        // no pintamos solamente la posicion anterior, porque si a la pelota la movemos muy rapido, no da el tiempo para limpiar las posiciones y queda mal
        brush.setColor(Qt::black);

        QRect myRect(0,0,ui->widget->width(),ui->widget->height()); //definimos el area

        paint.fillRect(myRect,brush);

        //pintamos la pelotita
        pen.setColor(Qt::white);
        paint.setPen(pen);
        brush.setColor(Qt::white);
        paint.setBrush(brush);

        paint.resetTransform();
        paint.translate(ballData.posX,ballData.posY);
        paint.drawEllipse(0,0,ballData.diameter,ballData.diameter);
        paint.save();

    }

    QPaintBox1->update();

}


