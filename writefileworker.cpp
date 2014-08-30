#include "writefileworker.h"

WriteFileWorker::WriteFileWorker(QObject *parent) :
    QObject(parent),
    m_running( false )
{
}

bool WriteFileWorker::error()
{
    // Error is occurred
    return m_error;
}

int WriteFileWorker::getProgress()
{
    return m_progress;
}

bool WriteFileWorker::isRunning()
{
    // 走っているか
    return m_running;
}

void WriteFileWorker::setParameter( QFile *writeFile, MicomFSFile *readFile )
{
    // Set the parameter
    this->readFile  = readFile;
    this->writeFile = writeFile;
}

void WriteFileWorker::setup()
{
    // 初期化など
    m_running = true;
}

void WriteFileWorker::doSaveFile( void )
{
    // Save readFile to writeFile
    uint32_t i;
    char buf[512];

    // init
    m_progress = 0;
    stopFlag   = false;
    m_error    = false;
    m_running  = true;

    // Save all sectors
    for ( i = 0; i < readFile->sector_count; i++ ) {
        // Check the flag
        if ( stopFlag ) {
            break;
        }

        // read sector
        if ( !micomfs_seq_fread( readFile, buf, 512 ) ) {
            m_error = true;

            break;
        }

        // write sector to output file
        writeFile->write( buf, 512 );

        // Set progress
        m_progress = i;
    }

    // emit error
    if ( m_error ) {
        emit errorOccurred();
    }

    emit finished();

    m_running = false;
}

void WriteFileWorker::stopSave()
{
    // Stop saving
    stopFlag = true;
}
