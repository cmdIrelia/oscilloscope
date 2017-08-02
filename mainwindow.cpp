#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include <QRadioButton>
#include <vector>
#include <QSignalMapper>
#include <QDebug>

MainWindow::MainWindow(const int PORT, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    UDP_PORT=PORT;

    udp_skt=new QUdpSocket(this);
    udp_skt->bind(UDP_PORT,QUdpSocket::ShareAddress);
    connect(udp_skt,SIGNAL(readyRead()),this,SLOT(processPendingDatagram()));
    ui->label_port->setText(QString::number(UDP_PORT,10));

    rec_cnt=0;
    open_document();

    selector=false;

    QPB_graph_Init();
    ui->radioButton_5V->setChecked(true);
    Timer_Init();
    ui->label_timer_state->setText("running..");
    //ui->pushButton->setVisible(false);

    ui->radioButton_Manual->setChecked(true);
    ui->checkBox_closeLoop->setChecked(false);
    ctrlpara = new ControlParameter((qint16)(ui->doubleSpinBox_p->value()*100),(qint16)(ui->doubleSpinBox_i->value()*10000),(qint16)(ui->doubleSpinBox_d->value()*10000),ui->radioButton_Q->isChecked(),0,ui->checkBox_closeLoop->isChecked());
    ctrlpara->setManual(ControlParameter::MANUAL);
}

void MainWindow::QPB_graph_Init()
{
    float range[2]={0,20000};

    ui->customPlot->addGraph();
    ui->customPlot->xAxis->setRange(0,DRAW_WIDTH);
    //ui->customPlot->yAxis->setRange(-18000, 18000);
    //ui->customPlot->yAxis2->setRange(-5,+5);
    ui->customPlot->yAxis->setRange(range[0],range[1]);
    ui->customPlot->yAxis2->setRange(20*range[0]/65536,20*range[1]/65536);

    ui->customPlot->yAxis2->setVisible(true);

    ui->customPlot->yAxis->setLabel("adc_value");
    ui->customPlot->yAxis2->setLabel("voltage(V)");
    ui->customPlot->graph(0)->setPen(QPen(QColor(255,0,0)));

    ui->customPlot->addGraph();
    ui->customPlot->graph(1)->setPen(QPen(QColor(0,255,0)));

    ui->customPlot->addGraph();
    ui->customPlot->graph(2)->setPen(QPen(QColor(180,80,255)));

    ui->customPlot->addGraph();
    ui->customPlot->graph(3)->setPen(QPen(QColor(0,0,255)));


    int i;
    for(i=0;i<4;i++)
    {
        y[i].resize(DRAW_WIDTH);
    }
    x.resize(DRAW_WIDTH);
    for(i=0;i<DRAW_WIDTH;i++)
    {
        x[i]=i;
    }
}

void MainWindow::open_document()
{
    file = new QFile("D:/data");
    if(!file->open(QFile::WriteOnly|QFile::Text))
    {
        QMessageBox::warning(this,"warning","cannot open",QMessageBox::Yes);
        this->close();
    }
    out = new QTextStream(file);

    iniFile=new readIni("d:/data.txt");
}

void MainWindow::Timer_Init()
{
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),
            this,SLOT(timerUpdate()));
    timer->start(0.01);
    //timer->setInterval(0.01);

    timer_sendcommand = new QTimer(this);
    connect(timer_sendcommand, SIGNAL(timeout()),
            this,SLOT(timerUpdate_sendcommand()));
    timer_sendcommand->setInterval(100);
    timer_sendcommand->start();
}

void MainWindow::timerUpdate()
{
    if(selector==true)
    {
        draw_customplot(ui->customPlot,&datagram[1]);
    }
    else
    {
        draw_customplot(ui->customPlot,&datagram[0]);
    }
}

void MainWindow::sendCommand()
{
    QHostAddress ip_Tx=QHostAddress("192.168.0.2");
    int ip_txPort=UDP_PORT;
    udp_skt->writeDatagram(ctrlpara->datagram(),ip_Tx,ip_txPort);

//    int i;
//    char *p=ctrlpara->datagram().data();
//    char s[100]={0};
//    for(i=0;i<ctrlpara->datagram().length();i++){
//        sprintf(s+strlen(s),"i%d=%02x ",i,p[i]&0xff);
//    }
//    qDebug()<<s;
}

void MainWindow::timerUpdate_sendcommand()
{
    if(ctrlpara->isManual()==ControlParameter::AUTO){
        int rtn;
        if(iniFile->getNextNumber(&rtn)){
            ctrlpara->setSv(rtn,ControlParameter::AUTO);
            //qDebug()<<"auto:"<<rtn;
            qDebug("auto:dec=%d hex=%x",ctrlpara->sv(),ctrlpara->sv());
        }
        else{
            ctrlpara->setManual(ControlParameter::MANUAL);
            ctrlpara->setSv(0,ControlParameter::MANUAL);
            if(iniFile->reopen()){
                qDebug()<<"reopen done.";
            }
            ui->radioButton_Manual->setChecked(true);
            ui->horizontalScrollBar_manualCtrl->setValue(0);

        }
    }
    //qDebug("%s",ctrlpara->isManual()?"manual":"auto");
    sendCommand();
}

MainWindow::~MainWindow()
{
    file->close();
    delete ui;
}

void MainWindow::processPendingDatagram()
{
    QByteArray *read_buf;
    read_buf=(selector==true)?&datagram[0]:&datagram[1];     //write [0] when true

    while(udp_skt->hasPendingDatagrams())
    {
        read_buf->resize(udp_skt->pendingDatagramSize());
        udp_skt->readDatagram(read_buf->data(),read_buf->size());

        ui->label->setText("接收帧数：\n" + QString::number(rec_cnt++));

        selector=!selector;
        should_draw=true;
    }
}

void MainWindow::draw_customplot(QCustomPlot *customPlot, QByteArray *draw_data)
{
    if(draw_data->size()<1000)
    {
        //static int err_pkg_cnt=0;
        //qDebug()<<draw_data->size()<<"  cnt="<<err_pkg_cnt++;
        return ;
    }
    if(draw_presclaer>=9){
        draw_presclaer=0;
    }
    else{
        draw_presclaer++;
    }
    if(should_draw==false) return;
    else should_draw=false;

    //const int size = 1000;//draw_data->size();   996=(6channel*2byte)*(83data)
    int i,x_cord=0;
    QVector<double> y_local[CHANNEL_NUM]; // initialize with entries 0..100
    float channel[4];

    for(i=0;i<CHANNEL_NUM;i++)
    {
        //y_local[i].resize(size/CHANNEL_NUM/2);
        y_local[i].resize(1);
    }
    //for(i=0;i<size;i+=8)
    for(i=0;i<1;i+=8)
    {
        channel[0]=(qint32)((draw_data->at(i+0)&0xff)|((draw_data->at(i+1)<<8)&0xff00));    //流量计脉宽->流量(L/min)
        channel[1]=(qint32)((draw_data->at(i+2)&0xff)|((draw_data->at(i+3)<<8)&0xff00));    //sv
        channel[2]=(qint16)((draw_data->at(i+4)&0xff)|((draw_data->at(i+5)<<8)&0xff00));    //压力滤波
        channel[3]=(qint32)((draw_data->at(i+6)&0xff)|((draw_data->at(i+7)<<8)&0xff00));    //PID结果
        y_local[0][x_cord]=channel[0];
        y_local[1][x_cord]=channel[1];
        y_local[2][x_cord]=channel[2];
        y_local[3][x_cord]=channel[3];
        x_cord++;

        if(ui->checkBox_output->isChecked())
        {
            for(int j=0;j<4;j++)
            {
                (*out)<<QString::number(channel[j]*10.f/32768.f)<<"\t";
            }
            (*out)<<"\n";
            out->flush();
        }
    }

    for(i=0;i<CHANNEL_NUM;i++)
    {
        //y[i].remove(0,size/CHANNEL_NUM/2);
        y[i].remove(0,1);
        y[i].append(y_local[i]);
    }

    if(ui->checkBox->isChecked()){
        customPlot->graph(0)->setData(x, y[0]);
        customPlot->graph(0)->setVisible(true);
    }else{
        customPlot->graph(0)->setVisible(false);
    }
    if(ui->checkBox_2->isChecked()){
        customPlot->graph(1)->setData(x, y[1]);
        customPlot->graph(1)->setVisible(true);
    }else{
        customPlot->graph(1)->setVisible(false);
    }
    if(ui->checkBox_3->isChecked()){
        customPlot->graph(2)->setData(x, y[2]);
        customPlot->graph(2)->setVisible(true);
    }else{
        customPlot->graph(2)->setVisible(false);
    }
    if(ui->checkBox_4->isChecked()){
        customPlot->graph(3)->setData(x, y[3]);
        customPlot->graph(3)->setVisible(true);
    }else{
        customPlot->graph(3)->setVisible(false);
    }
    //customPlot->rescaleAxes();
    customPlot->replot();

    ui->label_2->setText("当前帧数据量(Bytes):\n" + QString::number(draw_data->size()));
    ui->label_Q->setText(QString::number(channel[0]));
    //ui->label_P->setText(QString::number(((channel[2]*10.f/32768.f-10)*1.11111f-2)*35/8));    //P=(V-2)*35/8
    ui->label_P->setText(QString::number((channel[2]*10.f/32768.f*1.1111f-2)*35/8) + "\n" + QString::number(channel[2]*10.f/32768.f*1.11111f) + 'v');
}

void MainWindow::on_radioButton_10V_clicked()
{
    ui->customPlot->yAxis->setRange(-32768, 32768);
    ui->customPlot->yAxis2->setRange(-10,+10);
}

void MainWindow::on_radioButton_5V_clicked()
{
    ui->customPlot->yAxis->setRange(-18000, 18000);
    ui->customPlot->yAxis2->setRange(-5,+5);
}

void MainWindow::on_pushButton_set_range_clicked()
{
    float range[2]={ui->lineEdit_setrange_l->text().toFloat(),ui->lineEdit_setrange_h->text().toFloat()};
    ui->customPlot->yAxis->setRange(range[0],range[1]);
    ui->customPlot->yAxis2->setRange(20*range[0]/65536,20*range[1]/65536);
}

void MainWindow::on_lineEdit_setrange_l_returnPressed()
{
    float range[2]={ui->lineEdit_setrange_l->text().toFloat(),ui->lineEdit_setrange_h->text().toFloat()};
    ui->customPlot->yAxis->setRange(range[0],range[1]);
    ui->customPlot->yAxis2->setRange(20*range[0]/65536,20*range[1]/65536);
    qDebug()<<"set range L";
}

void MainWindow::on_lineEdit_setrange_h_returnPressed()
{
    float range[2]={ui->lineEdit_setrange_l->text().toFloat(),ui->lineEdit_setrange_h->text().toFloat()};
    ui->customPlot->yAxis->setRange(range[0],range[1]);
    ui->customPlot->yAxis2->setRange(20*range[0]/65536,20*range[1]/65536);
    qDebug()<<"set range H";
}

void MainWindow::on_pushButton_timerctrl_clicked()
{
    if(timer->isActive()){
        timer->stop();
        ui->label_timer_state->setText("stopped.");
    }
    else{
        timer->start();
        ui->label_timer_state->setText("running..");
    }
}

void MainWindow::on_horizontalScrollBar_manualCtrl_valueChanged(int value)
{
    ui->label_manualCtrl->setText(tr("hex:%1 - dec:%2").arg(value,4,16,QChar('0')).arg(value,5,10,QChar('0')));
    ctrlpara->setSv(value,ControlParameter::MANUAL);
    if(!ctrlpara->isControlQ()){
        ui->label_P_sv->setText(tr("P manual sv:%1").arg((value*10.f/65536.f-2)*35/8,5));
    }
    ui->label_P_sv->setVisible(ctrlpara->isManual()&&(!ctrlpara->isControlQ())&&ctrlpara->isCloseLoop());
}

void MainWindow::on_radioButton_Manual_toggled(bool checked)
{
    ctrlpara->setManual(checked);
    ui->horizontalScrollBar_manualCtrl->setEnabled(checked);
    //qDebug()<<checked;
}

void MainWindow::on_pushButton_pid_confirm_clicked()
{
    ctrlpara->PID::setPara((qint16)(ui->doubleSpinBox_p->value()*100),(qint16)(ui->doubleSpinBox_i->value()*10000),(qint16)(ui->doubleSpinBox_d->value()*10000));
}

void MainWindow::on_radioButton_Q_toggled(bool checked)
{
    if(checked) ctrlpara->setObject(ControlParameter::Q);
    else  ctrlpara->setObject(ControlParameter::P);
}

void MainWindow::on_checkBox_closeLoop_toggled(bool checked)
{
    ctrlpara->setCloseLoop(checked);
}
