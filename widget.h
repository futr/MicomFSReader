#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include "progressdialog.h"
#include "logicaldrivedialog.h"
#include "micomfs.h"
#include <QDebug>


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_selectButton_clicked();

    void on_closeButton_clicked();

    void on_openButton_clicked();

    void updateFileList();

    void on_saveButton_clicked();

    void on_openDriveButton_clicked();

private:
    Ui::Widget *ui;

    MicomFS fs;
    MicomFSFile *fileList;
    uint16_t fileCount;

    bool deviceOpened;
};

#endif // WIDGET_H
