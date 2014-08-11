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
    QString name = QFileDialog::getOpenFileName( this, "Select file" );

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
    deviceOpened = false;

    // Open device
    if ( !micomfs_open_device( &fs, ui->fileNameEdit->text().toUtf8().data(), MicomFSDeviceFile ) ) {
        QMessageBox::critical( this, "Error", "Can't open device" );

        return;
    }

    // Open File System
    if ( !micomfs_init_fs( &fs ) ) {
        QMessageBox::critical( this, "Error", "Can't open FileSystem" );

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
    ui->fsInfoListWidget->addItem( QString( "Max entry count : " + QString::number( fs.entry_count ) ) );
    ui->fsInfoListWidget->addItem( QString( "Used entries : " + QString::number( fs.used_entry_count ) ) );
    ui->fsInfoListWidget->addItem( QString( "Sector size : " + QString::number( fs.sector_size ) ) );
    ui->fsInfoListWidget->addItem( QString( "Sector count : " + QString::number( fs.sector_count ) ) );

    // Update the file list
    micomfs_get_file_list( &fs, &fileList, &fileCount );

    // Setup Tree Widget
    ui->fileListWidget->clear();
    ui->fileListWidget->setSelectionMode( QAbstractItemView::ContiguousSelection );
    ui->fileListWidget->setColumnCount( 2 );
    ui->fileListWidget->setHeaderLabels( QStringList() << "Name" << "Size[Bytes]" );

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
    int i;
    uint32_t j;

    if ( items.count() == 0 ) {
        return;
    }

    // Get file name
    name = QFileDialog::getExistingDirectory( this, "Save directory" );

    if ( name == "" ) {
        return;
    }

    // Create progress
    ProgressDialog *progress = new ProgressDialog();

    // Save all selected files
    for ( i = 0; i < items.count(); i++ ) {
        // Save file
        QFile file;
        MicomFSFile *fp;
        char buf[512];

        // Get file pointer
        fp = (MicomFSFile *)items[i]->data( 2, Qt::UserRole ).value<void *>();

        // Open file
        file.setFileName( name + "/" + fp->name );
        file.open( QIODevice::WriteOnly );

        // Write file
        progress->setWindowModality( Qt::ApplicationModal );
        progress->show();
        progress->setProgressPos( 0 );
        progress->setProgressMax( fp->sector_count );

        // Start file access
        micomfs_start_fread( fp, 0 );

        // Write loop
        for ( j = 0; j < fp->sector_count; j++ ) {
            // Process message
            QApplication::processEvents();

            // read sector
            micomfs_seq_fread( fp, buf, 512 );

            // write sector to output file
            file.write( buf, 512 );

            // Update UI
            progress->setProgressPos( j );

            // 閉じてたらキャンセル
            if ( !progress->isVisible() ) {
                break;
            }
        }

        // Close file
        micomfs_fclose( fp );

        // close file
        file.close();
    }

    delete progress;
}

void Widget::on_openDriveButton_clicked()
{
    // Open Window's logical drive
    LogicalDriveDialog *dialog;

    deviceOpened = false;

    dialog = new LogicalDriveDialog();

    // Select
    if ( dialog->exec() == QDialog::Accepted ) {
#ifdef __WIN32__
        HANDLE handle;
        VOLUME_DISK_EXTENTS diskExtents;
        DWORD dwSize;
        WCHAR wbuf[1024];
        int ret;

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
            QMessageBox::critical( this, "Error", "Can't open selected drive" );
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
            QMessageBox::critical( this, "Error", "Can't get physical drive name" );
            delete dialog;

            return;
        }

        // Close
        CloseHandle( handle );

        // Clear array
        for ( int i = 0; i < 1024; i++ ) {
            wbuf[i] = L'\0';
        }

        // Create physical drive name
        QString( "\\\\.\\PHYSICALDRIVE" + QString().sprintf( "%lu", diskExtents.Extents[0].DiskNumber ) ).toWCharArray( wbuf );

        // Open device
        if ( !micomfs_open_device( &fs, (char *)wbuf, MicomFSDeviceWinDrive ) ) {
            QMessageBox::critical( this, "Error", "Can't open device" );

            delete dialog;

            return;
        }

        // Open Filesystem
        if ( !micomfs_init_fs( &fs ) ) {
            QMessageBox::critical( this, "Error", "Can't open FileSystem" );

            micomfs_close_device( &fs );

            delete dialog;

            return;
        }

        QMessageBox::critical( this, "Error", QString::number( fs.dev_sector_count ) );
        QMessageBox::critical( this, "Error", QString::number( fs.dev_sector_size ) );
        QMessageBox::critical( this, "Error", QString::number( fs.entry_count ) );

        // Clear variable
        fileList = NULL;

        // Update
        updateFileList();
#endif
    }

    deviceOpened = true;

    delete dialog;
}
