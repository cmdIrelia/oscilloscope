#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QFile>
#include <QTextStream>
#include "qcustomplot.h"
#include <QTimer>
#include "readini.h"

namespace Ui {
class MainWindow;
}

class PID{
public:
    PID(){
        p=0;i=0;d=0;
    }
    PID(qint16 _p,qint16 _i,qint16 _d){
        p=_p;i=_i;d=_d;
    }
    void setPara(qint16 _p,qint16 _i,qint16 _d){
        p=_p;i=_i;d=_d;
        qDebug()<<"set pid para:"<<p<<" "<<i<<" "<<d;
    }
    void setP(qint16 _p){p=_p;}
    void setI(qint16 _i){i=_i;}
    void setD(qint16 _d){d=_d;}

protected:
    int p,i,d;

};

class ControlParameter : public PID{
protected:

    bool controlObject_Q;
    quint16 sv_manual,sv_auto;
    bool manual;
    bool closeLoop;
//  PID pid_s;
    QByteArray datagram_command;
public:
    enum STATE{
        IDLE=1,
        STOP,
        RUN,
        EMERENCY_ZERO_OUTPUT,
        EMERENCY_THROAT_OPEN
    };
    enum MODE{
        AUTO=false,
        MANUAL=true
    };
    enum CTRL_OBJECT{
        Q=true,
        P=false
    };

    ControlParameter(qint16 _p,qint16 _i,qint16 _d,bool _controlObject_Q,quint16 _sv,bool _closeLoop){
        PID::setPara(_p,_i,_d);
        controlObject_Q=_controlObject_Q;
        setSv(_sv,true);
        closeLoop=_closeLoop;
        datagram_command.fill(20,char(0));
    }
    void setPara(int _p,int _i,int _d,bool _controlObject_Q){
        PID::setPara(_p,_i,_d);
        controlObject_Q=_controlObject_Q;
    }
    //void setPara(int _p,int _i,int _d){pid_s.setPara(_p,_i,_d);}
    void setSv(quint16 _sv, bool isManual){
        if(isManual==MANUAL) sv_manual=_sv;
        else sv_auto=_sv;
    }
    void setManual(bool _manual){manual=_manual;}
    void setObject(bool _controlObject_Q){controlObject_Q=_controlObject_Q;}
    void setCloseLoop(bool _closeLoop){closeLoop=_closeLoop;}
    quint16 sv(){
        if(manual==MANUAL) return sv_manual;
        else return sv_auto;
    }
    bool isCloseLoop(){return closeLoop;}
    bool isManual(){return manual;}
    bool isControlQ(){return controlObject_Q;}

    QByteArray datagram(){
        int pos=0;
        datagram_command[pos++]=0x99;   //帧头                             0
        datagram_command[pos++]=0x02;   //帧类型                           1
        datagram_command[pos++]=RUN;   //状态控制                           2
        datagram_command[pos++]=controlObject_Q;    //对象                  3
        datagram_command[pos++]=((quint8)(sv()>>8&0xff));   //目标高八位     4
        datagram_command[pos++]=((quint8)(sv()&0xff));      //目标低八位     5
        datagram_command[pos++]=((quint8)(p>>8&0xff));//P高八位             6
        datagram_command[pos++]=((quint8)(p&0xff));   //P低八位             7
        datagram_command[pos++]=((quint8)(i>>8&0xff));//I高八位             8
        datagram_command[pos++]=((quint8)(i&0xff));   //I低八位             9
        datagram_command[pos++]=((quint8)(d>>8&0xff));//D高八位             10
        datagram_command[pos++]=((quint8)(d&0xff));  //D低八位              11
        datagram_command[pos++]=isCloseLoop();         //开闭环               12
        return datagram_command;
    }
private:

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const int PORT, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QUdpSocket *udp_skt;
    int UDP_PORT;

    long rec_cnt;   //接收计数
    QFile *file;
    QTextStream* out;

    QByteArray datagram[2];
    bool selector;

    QTimer *timer;
    QTimer *timer_sendcommand;

    const int CHANNEL_NUM=4;
    const int  DRAW_WIDTH=1000;
    QVector<double> x;
    QVector<double>y[4]; // initialize with entries 0..100

    readIni *iniFile;

    void open_document();
    void QPB_graph_Init();

    bool should_draw=false;
    void draw_customplot(QCustomPlot *, QByteArray *draw_data);
    void Timer_Init();

    ControlParameter *ctrlpara;//(0,0,0,true,0);
    QByteArray datagram_command;

    int draw_presclaer=0; //画图分频


private slots:
    void processPendingDatagram();

    void timerUpdate();
    void timerUpdate_sendcommand();
    void on_pushButton_timerctrl_clicked();
    void on_radioButton_10V_clicked();
    void on_radioButton_5V_clicked();
    void on_pushButton_set_range_clicked();
    void on_lineEdit_setrange_l_returnPressed();
    void on_lineEdit_setrange_h_returnPressed();

    void on_horizontalScrollBar_manualCtrl_valueChanged(int value);
    void on_pushButton_pid_confirm_clicked();

    void sendCommand();
    void on_radioButton_Manual_toggled(bool checked);
    void on_radioButton_Q_toggled(bool checked);
    void on_checkBox_closeLoop_toggled(bool checked);
};

#endif // MAINWINDOW_H
