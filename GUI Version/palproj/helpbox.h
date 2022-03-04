#ifndef HELPBOX_H
#define HELPBOX_H

#include <QDialog>

namespace Ui {
class helpBox;
}

class helpBox : public QDialog
{
    Q_OBJECT

public:
    explicit helpBox(QWidget *parent = nullptr);
    ~helpBox();

private slots:
    void on_pushButton_clicked();

private:
    Ui::helpBox *ui;
};

#endif // HELPBOX_H
