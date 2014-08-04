#include "saveprocess.h"
#include "ui_saveprocess.h"

SaveProcess::SaveProcess(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SaveProcess)
{
    ui->setupUi(this);
}

SaveProcess::~SaveProcess()
{
    delete ui;
}

void SaveProcess::setProgressPos(int pos)
{
    ui->progressBar->setValue( pos );
}

void SaveProcess::setProgressMax(int max)
{
    ui->progressBar->setMaximum( max );
}

void SaveProcess::on_cancelButton_clicked()
{
    this->close();
}
