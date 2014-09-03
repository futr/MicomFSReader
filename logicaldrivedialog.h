#ifndef LOGICALDRIVEDIALOG_H
#define LOGICALDRIVEDIALOG_H

#include <QDialog>
#include <QMessageBox>

#ifdef __WIN32__
#include <windows.h>
#endif

namespace Ui {
class LogicalDriveDialog;
}

class LogicalDriveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogicalDriveDialog(QWidget *parent = 0);
    ~LogicalDriveDialog();

    void updateLogicalDriveList( void );
    QString getSelectedLogicalDriveName( void );

private slots:
    void on_okButton_clicked();

private:
    Ui::LogicalDriveDialog *ui;
};

#endif // LOGICALDRIVEDIALOG_H
