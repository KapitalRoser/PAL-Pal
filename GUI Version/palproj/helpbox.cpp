#include "helpbox.h"
#include "processCore.h"
#include "ui_helpbox.h"
#include <CoreFoundation/CFBundle.h> //where should this really go?


//fun MacOS stuff
CFBundleRef mainBundle = CFBundleGetMainBundle();
CFURLRef docURL = CFBundleCopyResourceURL(mainBundle,CFSTR("doc"), CFSTR("txt"),NULL);


helpBox::helpBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::helpBox)
{
    ui->setupUi(this);
    if (docURL != NULL){
        std::string docStr = CFStringGetCStringPtr(CFURLGetString(docURL),kCFStringEncodingUTF8); //If you notice this starts to fail, then look up better solutions.
        docStr.erase(0,7);
        std::ifstream helpFile(docStr);
        qInfo() << "HelpFile declared";
        std::string lineIn;
        while(!helpFile.fail()){
           std::getline(helpFile,lineIn);
           ui->bodyText->setStyleSheet("font-size: 8pt;");
           ui->bodyText->setText(ui->bodyText->text() + QString::fromStdString(lineIn) + "\n"); //+ \n?
        }
    }



    //$HOME/....
    //echo $HOME
    //shell script sets up environment variables using commands.
    //.bat file for windows, .sh for unix.
    //use the variables throughout the program
    // store doc and config there. Documents is probably better than applications because of perms.


}

helpBox::~helpBox()
{
    delete ui;
}

void helpBox::on_pushButton_clicked()
{
    helpBox::accept();
}
