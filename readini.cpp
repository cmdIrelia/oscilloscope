#include "readini.h"
#include <QDebug>
#include <QByteArray>

readIni::readIni(QString path)
{
    file=new QFile(path);
    if(!file->open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"readIni: cannot open file!";
        return ;
    }
    filePath=path;
}

bool readIni::getNextLine(QByteArray *qba)
{
    *qba=file->readLine();
    qba->remove(qba->length()-1,1); //去掉末尾的换行符
    if(qba->isEmpty()){
        return 0;
    }
    else{
        return 1;
    }
}

bool readIni::getNextNumber(int *result)
{
    bool rtn;
    QByteArray rl;
    if(getNextLine(&rl)==true){
        *result=rl.toInt(&rtn);
        return rtn;
    }
    else{
        qDebug()<<"readIni: getNextNumber Failed.";
        *result=0;
        return false;
    }
}

bool readIni::reopen()
{
    file->close();
    delete(file);
    file=new QFile(filePath);
    if(!file->open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"readIni: cannot open file!";
        return false;
    }
    return true;
}

readIni::~readIni()
{
    file->close();
}
