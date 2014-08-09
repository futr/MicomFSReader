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

    // Get Logical Drive bits
    drives = GetLogicalDrives();

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
