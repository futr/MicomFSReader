#include "writefileworker.h"

WriteFileWorker::WriteFileWorker(QObject *parent) :
    QObject(parent)
{
}

bool WriteFileWorker::error()
{
    // Error is occurred
    return m_error;
}

void WriteFileWorker::setParameter( QFile *writeFile, MicomFSFile *readFile, int intervalInMS )
{
    // Set the parameter
    this->interval  = intervalInMS;
    this->readFile  = readFile;
    this->writeFile = writeFile;
}

void WriteFileWorker::doSaveFile( void )
{
    // Save readFile to writeFile
    uint32_t i;
    QTimer intervalTimer;
    char buf[512];

    // init
    beforePos = 0;
    stopFlag  = false;
    m_error   = false;

    // Start interval timer
    connect( &intervalTimer, SIGNAL(timeout()), this, SLOT(updateProgress()) );
    intervalTimer.setInterval( interval );
    intervalTimer.start();

    // Save all sectors
    for ( i = 0; i < readFile->sector_count; i++ ) {
        // Checl the flag
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
    }

    intervalTimer.stop();

    // emit error
    if ( m_error ) {
        emit errorOccurred();
    }

    emit finished();
}

void WriteFileWorker::stopSave()
{
    // Stop saving
    stopFlag = true;
}

void WriteFileWorker::updateProgress( void )
{
    bytesPerSec = ( readFile->sector_count - beforePos ) * 512 / ( interval / 1000.0 );

    beforePos = readFile->current_sector;

    emit progress( readFile->current_sector, readFile->sector_count, bytesPerSec, readFile->name );
}
