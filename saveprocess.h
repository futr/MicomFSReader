#ifndef SAVEPROCESS_H
#define SAVEPROCESS_H

#include <QWidget>

namespace Ui {
class SaveProcess;
}

class SaveProcess : public QWidget
{
    Q_OBJECT

public:
    explicit SaveProcess(QWidget *parent = 0);
    ~SaveProcess();

    void setProgressPos( int pos );
    void setProgressMax( int max );

private slots:
    void on_cancelButton_clicked();

private:
    Ui::SaveProcess *ui;
};

#endif // SAVEPROCESS_H
