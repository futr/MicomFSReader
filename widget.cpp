#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    deviceOpened( false )
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    // Close device if device is opened
    if ( deviceOpened ) {
        micomfs_close_device( &fs );
    }

    delete ui;
}

void Widget::on_selectButton_clicked()
{
    // Select file
    QString name = QFileDialog::getOpenFileName( this, tr( "Select a file" ) );

    // Return if didn't open
    if ( name == "" ) {
        return;
    }

    // Set the file name
    ui->fileNameEdit->setText( name );

    // Open
    ui->openButton->click();
}

void Widget::on_closeButton_clicked()
{
    close();
}

void Widget::on_openButton_clicked()
{
    // Open file
    if ( deviceOpened ) {
        micomfs_close_device( &fs );

        deviceOpened = false;
    }

    ui->fileListWidget->clear();
    ui->fsInfoListWidget->clear();

    // Open device
    if ( !micomfs_open_device( &fs, ui->fileNameEdit->text().toUtf8().data(), MicomFSDeviceFile, MicomFSDeviceModeRead ) ) {
        QMessageBox::critical( this, tr( "Error" ), tr( "Can't open device" ) );

        return;
    }

    // Open File System
    if ( !micomfs_init_fs( &fs ) ) {
        QMessageBox::critical( this, tr( "Error" ), tr( "Can't open FileSystem" ) );

        micomfs_close_device( &fs );

        return;
    }

    // Clear variable
    fileList = NULL;

    // Update
    updateFileList();

    deviceOpened = true;
}

void Widget::updateFileList()
{
    // Update the File list
    int i;

    // Delete the previous file list
    if ( fileList != NULL ) {
        free( fileList );
    }

    // Show File system information
    ui->fsInfoListWidget->clear();
    ui->fsInfoListWidget->addItem( QString( tr( "Max entry count : " ) + QString::number( fs.entry_count ) ) );
    ui->fsInfoListWidget->addItem( QString( tr( "Used entries : " ) + QString::number( fs.used_entry_count ) ) );
    ui->fsInfoListWidget->addItem( QString( tr( "Sector size : " ) + QString::number( fs.sector_size ) ) );
    ui->fsInfoListWidget->addItem( QString( tr( "Sector count : " ) + QString::number( fs.sector_count ) ) );

    // Update the file list
    micomfs_get_file_list( &fs, &fileList, &fileCount );

    // Setup Tree Widget
    ui->fileListWidget->clear();
    ui->fileListWidget->setSelectionMode( QAbstractItemView::ContiguousSelection );
    ui->fileListWidget->setColumnCount( 2 );
    ui->fileListWidget->setHeaderLabels( QStringList() << tr( "Name" ) << tr( "Size[Bytes]" ) );

    // Show the file list into widget
    for ( i = 0; i < fileCount; i++ ) {
        QList<QTreeWidgetItem *> items;
        items << new QTreeWidgetItem( QStringList() << fileList[i].name << "" );
        items[0]->setData( 1, Qt::DisplayRole, fileList[i].sector_count * 512 );
        items[0]->setData( 2, Qt::UserRole, QVariant::fromValue( (void *)&fileList[i] ) );

        ui->fileListWidget->addTopLevelItems( items );
    }

    // resize
    for ( i = 0; i < 2; i++ ) {
        ui->fileListWidget->resizeColumnToContents( i );
    }
}

void Widget::on_saveButton_clicked()
{
    // Save selected file
    QString name;
    QList<QTreeWidgetItem *> items = ui->fileListWidget->selectedItems();
    bool yesToAll;
    int i;

    if ( !deviceOpened ) {
        return;
    }

    if ( items.count() == 0 ) {
        QMessageBox::information( this, tr( "Information" ), tr( "Files are not selected\nPlease select files" ) );

        return;
    }

    // Get file name
    name = QFileDialog::getExistingDirectory( this, tr( "Directory to save" ) );

    if ( name == "" ) {
        return;
    }

    yesToAll = false;

    // Save all selected files
    for ( i = 0; i < items.count(); i++ ) {
        // Save file
        QFile file;
        MicomFSFile *fp;
        QThread *saveFileThread;
        WriteFileWorker *saveFileWorker;
        QMessageBox warningBox;

        // Make warning box
        warningBox.setWindowTitle( tr( "Warning" ) );
        warningBox.setText( tr( "File write failed" ) );
        warningBox.setInformativeText( tr( "A problem has occurred on current operation." ) );
        warningBox.setIcon( QMessageBox::Warning );
        warningBox.setStandardButtons( QMessageBox::Ok );

        // Get file pointer
        fp = (MicomFSFile *)items[i]->data( 2, Qt::UserRole ).value<void *>();

        // Set file name
        file.setFileName( name + "/" + fp->name );

        // 上書き確認
        if ( file.exists() && yesToAll == false ) {
            QMessageBox::StandardButton btn = QMessageBox::question( this, tr( "Overwrite" ), QString( tr( "%1 is already existing.\nOverwrite it?" ) ).arg( file.fileName() ),  QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );

            if ( btn == QMessageBox::No ) {
                continue;
            } else if ( btn == QMessageBox::Cancel ) {
                break;
            } else if ( btn == QMessageBox::YesToAll ) {
                yesToAll = true;
            }
        }

        // Open
        file.open( QIODevice::WriteOnly );

        // Write file

        // Create progress
        ProgressDialog *progress = new ProgressDialog();
        progress->setProgressPos( 0, 0, 0, "Null" );
        progress->setProgressMax( fp->sector_count );

        // Start file access
        micomfs_start_fread( fp, 0 );

        // Start Save File Thread
        saveFileThread = new QThread( this );
        saveFileWorker = new WriteFileWorker();

        // Setup
        saveFileWorker->moveToThread( saveFileThread );
        saveFileWorker->setParameter( &file, fp );
        saveFileWorker->setup();

        connect( saveFileWorker, SIGNAL(finished()),    progress,       SLOT(accept()) );
        connect( progress,       SIGNAL(finished(int)), saveFileWorker, SLOT(stopSave()), Qt::DirectConnection );
        connect( saveFileThread, SIGNAL(started()),     saveFileWorker, SLOT(doSaveFile()) );
        connect( saveFileWorker, SIGNAL(progress(int,int,int,QString)), progress, SLOT(setProgressPos(int,int,int,QString)) );

        // Exec
        saveFileThread->start();

        // Showmodal progress
        progress->setWindowFlags( Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog );
        progress->exec();

        // Wait Save Thread
        saveFileThread->quit();
        saveFileThread->wait();

        // Check a error
        if ( saveFileWorker->error() ) {
            warningBox.exec();
        }

        // Delete
        delete saveFileWorker;
        delete saveFileThread;
        delete progress;

        // 閉じる
        micomfs_fclose( fp );

        // close file
        file.close();
    }
}

void Widget::on_openDriveButton_clicked()
{
    // Open Window's logical drive
    LogicalDriveDialog *dialog;

    dialog = new LogicalDriveDialog();

    // Select
    if ( dialog->exec() == QDialog::Accepted ) {
#ifdef __WIN32__
        HANDLE handle;
        VOLUME_DISK_EXTENTS diskExtents;
        DWORD dwSize;
        WCHAR wbuf[1024];
        int ret;

        // もしすでにデバイスが開かれていれば閉じる
        if ( deviceOpened ) {
            micomfs_close_device( &fs );

            deviceOpened = false;
        }

        // Clear array
        for ( int i = 0; i < 1024; i++ ) {
            wbuf[i] = L'\0';
        }

        // Open Logical drive
        QString( "\\\\.\\" + QString( dialog->getSelectedLogicalDriveName() ) + ":" ).toWCharArray( wbuf );

        handle = CreateFileW( wbuf,
                              GENERIC_READ,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL );

        // Check error
        if ( handle == INVALID_HANDLE_VALUE ) {
            QMessageBox::critical( this, tr( "Error" ), tr( "Can't open selected drive" ) );
            delete dialog;

            return;
        }

        // Get Physical drive name
        ret = DeviceIoControl( handle,
                               IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL,
                               0,
                               (LPVOID) &diskExtents,
                               (DWORD) sizeof(diskExtents),
                               (LPDWORD) &dwSize,
                               NULL );

        // Check error
        if ( ret == 0 ) {
            QMessageBox::critical( this, tr( "Error" ), tr( "Can't get physical drive name" ) );
            delete dialog;

            return;
        }

        // Close
        CloseHandle( handle );

        /*
        // Clear array
        for ( int i = 0; i < 1024; i++ ) {
            wbuf[i] = L'\0';
        }

        // Create physical drive name
        QString( "\\\\.\\PHYSICALDRIVE" + QString().sprintf( "%lu", diskExtents.Extents[0].DiskNumber ) ).toWCharArray( wbuf );
        */

        // Open device ( 権限が不要なので論理ドライブ名のままで実行 )
        if ( !micomfs_open_device( &fs, (char *)wbuf, MicomFSDeviceWinDrive, MicomFSDeviceModeRead ) ) {
            QMessageBox::critical( this, tr( "Error" ), tr( "Can't open device" ) );

            delete dialog;

            return;
        }

        // Open Filesystem
        if ( !micomfs_init_fs( &fs ) ) {
            QMessageBox::critical( this, tr( "Error" ), tr( "Can't open FileSystem" ) );

            micomfs_close_device( &fs );

            delete dialog;

            return;
        }

        // Clear variable
        fileList = NULL;

        // Update
        updateFileList();

        deviceOpened = true;
#endif
    }

    delete dialog;
}

void Widget::on_aboutButton_clicked()
{
    // About box
    QString verStr;

    verStr = verStr +
            "MicomFSReader\n" +
            "(c) 2014 Masato Takahashi\n" +
            "mas@to.email.ne.jp\n\n" +
            "MFSAPI : " + MICOMFS_API_VERSION_CODE;

    QMessageBox::about( this,
                        tr( "MicomFSReader" ),
                        verStr );
}
