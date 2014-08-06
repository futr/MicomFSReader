#include "progressdialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::setProgressPos(int pos)
{
    ui->progressBar->setValue( pos );
}

void ProgressDialog::setProgressMax(int max)
{
    ui->progressBar->setMaximum( max );
}
