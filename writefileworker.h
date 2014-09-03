#ifndef WRITEFILEWORKER_H
#define WRITEFILEWORKER_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include "micomfs.h"
#include <time.h>
#include <QElapsedTimer>
#include <unistd.h>

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
    int m_progress;
    bool m_running;

public:
    bool error( void );
    int getProgress( void );
    bool isRunning( void );

signals:
    void errorOccurred( void );
    void finished( void );
    void progress( int pos, int max, int bytesPerSec, QString name );

public slots:
    void setParameter( QFile *writeFile, MicomFSFile *readFile );
    void setup();
    void doSaveFile( void );
    void stopSave( void );

};

#endif // WRITEFILEWORKER_H
