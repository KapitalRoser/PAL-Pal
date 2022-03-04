#include "helpbox.h"
#include "processCore.h"
#include "ui_helpbox.h"

helpBox::helpBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::helpBox)
{
    ui->setupUi(this);
    std::ifstream helpFile("doc.txt");
    std::string lineIn;
    while(!helpFile.fail()){
       std::getline(helpFile,lineIn);
       ui->bodyText->setStyleSheet("font-size: 8pt;");
       ui->bodyText->setText(ui->bodyText->text() + QString::fromStdString(lineIn) + "\n"); //+ \n?
    }
}

helpBox::~helpBox()
{
    delete ui;
}

void helpBox::on_pushButton_clicked()
{
    helpBox::accept();
}
