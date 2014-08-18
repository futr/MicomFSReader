#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = 0);
    ~ProgressDialog();

    void setProgressMax( int max );
    void setLabelCaption( QString &caption );

private:
    Ui::ProgressDialog *ui;

public slots:
    void setProgressPos( int pos , int max, int bytesPerSec, QString fileName );
};

#endif // PROGRESSDIALOG_H
