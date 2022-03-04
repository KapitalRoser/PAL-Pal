#include <QDialog>
//#include "processCore.h"

namespace Ui {
class configure;
}

class configure : public QDialog
{
    Q_OBJECT

public:
    explicit configure(QWidget *parent = nullptr);
    void writeReqsToFile();
    void readExisting();
    ~configure();

private slots:
    void on_buttonBox_accepted();
    void natureButtonClicked(bool checked);
    void hpButtonClicked(bool checked);
    void on_buttonBox_rejected();

    void on_pushButton_clicked();

private:
    Ui::configure *ui;
};
