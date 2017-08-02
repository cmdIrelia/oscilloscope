#ifndef READINI_H
#define READINI_H

#include <QObject>
#include <QFile>

class readIni
{
public:
    readIni(QString);
    bool getNextLine(QByteArray *qba);
    ~readIni();
    bool getNextNumber(int *result);
    bool reopen();
private:
    QFile *file;
    QString filePath;
};

#endif // READINI_H
