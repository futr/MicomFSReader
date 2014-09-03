#include "logicaldrivedialog.h"
#include "ui_logicaldrivedialog.h"

LogicalDriveDialog::LogicalDriveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogicalDriveDialog)
{
    ui->setupUi(this);

    // Fix size
    setFixedSize( sizeHint() );

    // Update drive list
    updateLogicalDriveList();
}

LogicalDriveDialog::~LogicalDriveDialog()
{
    delete ui;
}

void LogicalDriveDialog::updateLogicalDriveList()
{
    // List up logical drive
    ui->driveComboBox->clear();

#ifdef __WIN32__
    DWORD drives;
    HANDLE handle;
    DISK_GEOMETRY_EX dgex;
    DWORD dw;

    // Get Logical Drive bits
    drives = GetLogicalDrives();

    // 見つかったロジカルドライブの種類を調べremovableだけ残す
    for ( int i = 0; i < 32; i++ ) {
        if ( drives & ( 1 << i ) ) {
            // ith drive is available
            WCHAR wbuf[1024];

            // Clear array
            for ( int j = 0; j < 1024; j++ ) {
                wbuf[j] = L'\0';
            }

            QString( "\\\\.\\" + QString().sprintf( "%c", i + 65 ) + ":" ).toWCharArray( wbuf );

            handle = CreateFileW( wbuf,
                                  GENERIC_READ,
                                  0,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );

            // Check error
            if ( handle == INVALID_HANDLE_VALUE ) {
                drives = drives & ~( 1 << i );

                continue;
            }

            // デバイス情報取得
            DeviceIoControl( handle,
                             IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                             NULL,
                             0,
                             (LPVOID) &dgex,
                             (DWORD) sizeof( dgex ),
                             (LPDWORD) &dw,
                             NULL );

            // リムーバブルか調べる
            if ( dgex.Geometry.MediaType != RemovableMedia ) {
                // 違う
                drives = drives & ~( 1 << i );
            }

            CloseHandle( handle );
        }
    }

    // UI設定
    for ( int i = 0; i < 32; i++ ) {
        if ( drives & ( 1 << i ) ) {
            // ith drive is available
            ui->driveComboBox->addItem( QString().sprintf( "%c", i + 65 ) );
        }
    }
#endif
}

QString LogicalDriveDialog::getSelectedLogicalDriveName()
{
    return ui->driveComboBox->currentText();
}

void LogicalDriveDialog::on_okButton_clicked()
{
    // OK
    if ( ui->driveComboBox->currentIndex() < 0 ) {
        reject();

        return;
    }

    accept();
}
