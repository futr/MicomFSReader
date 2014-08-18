#ifndef WRITEFILEWORKER_H
#define WRITEFILEWORKER_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include "micomfs.h"

class WriteFileWorker : public QObject
{
    Q_OBJECT
public:
    explicit WriteFileWorker(QObject *parent = 0);

private:
    QFile *writeFile;
    MicomFSFile *readFile;
    int bytesPerSec;
    int beforePos;
    int interval;
    bool stopFlag;
    bool m_error;

public:
    bool error( void );

signals:
    void errorOccurred( void );
    void finished( void );
    void progress( int pos, int max, int bytesPerSec, QString fileName );

public slots:
    void setParameter( QFile *writeFile, MicomFSFile *readFile, int intervalInMS );
    void doSaveFile( void );
    void stopSave( void );

private slots:
    void updateProgress( void );

};

#endif // WRITEFILEWORKER_H
