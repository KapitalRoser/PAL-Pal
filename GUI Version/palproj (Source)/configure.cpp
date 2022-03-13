#include "configure.h"
#include "ui_configure.h"
#include "processCore.h"
#include "searchlogic.h"
#include "unistd.h"

PokemonRequirements inputReqs;

const std::string CONFIG_FILEPATH = "/Users/Carter/Documents/PAL-Pal.app/Contents/Resources/config.txt";
const std::string configPath = std::string(getenv("HOME")) + "/Library/Application Support/PAL-Pal";
const std::string configName = configPath + "/config.txt";
//std::string configPath = strcat(getenv("HOME"),"/Documents/PAL-Pal/config.txt");
std::vector<unsigned int> naturesChosen;
std::vector<unsigned int> hiddenPowersChosen;

//Add select all and clear all buttons to Natures and HiddenPower Types.
//Add +/n/- nature iv boxes, frlg style.
//Add warning box to prevent user for searching for an impossible nature/iv/hidden power combo (ex. 31iv spa. hp fighting, 30 is ok)
//Add textEdit to allow the copy/pasting of a code for easier setup.

void configure::writeReqsToFile(){
    //assumes formatted
    //write reqs to file
    std::ofstream configW (configName);
    const std::string version = "r1.0";
    configW << version << "\n";

    //IVs -- only neutral for now.
    configW << ui->inHP->value() << "\n";
    configW << ui->inNAtk->value() << "\n";
    configW << ui->inNDef->value() << "\n";
    configW << ui->inNSpA->value() << "\n";
    configW << ui->inNSpD->value() << "\n";
    configW << ui->inNSpe->value() << "\n";

    //Natures
    if (naturesChosen.empty() || naturesChosen.size() == 25){
        configW << "25\n";
    } else {
        configW << naturesChosen.size() << "\n";
        for(unsigned int i = 0; i < naturesChosen.size();i++){
            configW << naturesChosen.at(i) << "\n";
        }
    }

    //Same for Hidden Power
    if (hiddenPowersChosen.empty() || hiddenPowersChosen.size() == 16){
        configW << "16\n";
    } else {
        configW << hiddenPowersChosen.size() << "\n";
        for (unsigned int i = 0; i < hiddenPowersChosen.size();i++) {
            configW << hiddenPowersChosen.at(i) << "\n";
        }
    }

    //Hidden Power Strength
    configW << ui->spinHidenPower->value() << "\n";

    //radio button questions
    //There's probably a better way with the buttonBox/groupBox stuff but I can't figure it out.
    //Gender first
    int genderStatus = 0;
    if(ui->radioMale->isChecked()){
        genderStatus = 0;
    }
    if (ui->radioFemale->isChecked()){
        genderStatus = 1;
    }
    if (ui->radioAnyGender->isChecked()){
        genderStatus = 2;
    }
    configW << genderStatus << "\n";

    //Then Shiny last
    int shinyStatus = 0;
    if (ui->radioNotShiny->isChecked()){
        shinyStatus = 0;
    }
    if (ui->radioShiny->isChecked()){
        shinyStatus = 1;
    }
    if (ui->radioAnyShiny->isChecked()){
        shinyStatus = 2;
    }
    configW << shinyStatus;
    //no forceEven, since that isn't necessary anymore.
    configW.close();
}
void configure::readExisting(){
    PokemonRequirements fileReqs;
    fileReqs.validNatures.fill(0);
    fileReqs.validHPTypes.fill(0);
    int lineCount = 0;
    bool valid = 0;
    std::string empt;
    std::ifstream existing(configName);

    //This verification step just loosely checks if the file was corrupted in some way
    //Like if it didn't write all the lines, or the user deleted lines or something.
    //Honestly not thrilled with this. May need to redesign or reconsider it's existence entirely.
    //All I want to do is prevent crashes.
    if(existing.fail()){
        qInfo() << "Config corrupted or not found!";
    } else {
        qInfo() << "config file found!";
    }
    while(!existing.fail()){
        lineCount++;
        existing >> empt;
    }
    if (lineCount>=14 && lineCount <= 52){
        valid = true;

    } else {
        valid = false;
    }
    existing.clear();
    existing.seekg(0);
    readReqsConfig(fileReqs,existing);
    existing.close();

    if (valid){
       ui->inHP->setValue(fileReqs.hpIV);
       ui->inNAtk->setValue(fileReqs.atkIV);
       ui->inNDef->setValue(fileReqs.defIV);
       ui->inNSpA->setValue(fileReqs.spAtkIV);
       ui->inNSpD->setValue(fileReqs.spDefIV);
       ui->inNSpe->setValue(fileReqs.speedIV);

       //natures
       for (int i = 0;i<25;i++) {
           if (fileReqs.validNatures[i]){
               configure::findChild<QPushButton *>("butN" + QString::number(i))->setChecked(true);
               naturesChosen.push_back(i);
           }
       }
       std::sort(naturesChosen.begin(),naturesChosen.end());
       //hidden powers
       for (int i = 0;i<16;i++) {
           if (fileReqs.validHPTypes[i]){
               configure::findChild<QPushButton *>("butH" + QString::number(i))->setChecked(true);
               hiddenPowersChosen.push_back(i);
           }
       }
       std::sort(hiddenPowersChosen.begin(),hiddenPowersChosen.end());
       ui->spinHidenPower->setValue(fileReqs.hiddenPowerPower);

       if (fileReqs.genderIndex == 0){
           ui->radioMale->setChecked(true);
       } else if (fileReqs.genderIndex == 1){
           ui->radioFemale->setChecked(true);
       } else {
           ui->radioAnyGender->setChecked(true);
       }

       if (fileReqs.isShiny == 0){
           ui->radioNotShiny->setChecked(true);
       } else if (fileReqs.isShiny == 1){
           ui->radioShiny->setChecked(true);
       } else {
           ui->radioAnyShiny->setChecked(true);
       }
    }
}
configure::configure(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::configure)
{
    ui->setupUi(this);
    naturesChosen.clear();
    hiddenPowersChosen.clear();
//    ui->pushButton->setText(QString::fromStdString(CONFIG_FILEPATH));
    //First Check for existing data
    readExisting();
    QString boxStyle = "QGroupBox{border:1px solid gray; border-radius: 5px; margin-top: 6px;} QGroupBox::title{subcontrol-origin: margin;left: 7px; padding: 0px 5px 0px 5px;}";
    ui->IVBox->setStyleSheet(boxStyle);
    ui->natureBox->setStyleSheet(boxStyle);
    ui->HPBox->setStyleSheet(boxStyle);
    ui->genderBox->setStyleSheet(boxStyle);
    ui->shinyGroupBox->setStyleSheet(boxStyle);
    //ui->pushButton->setStyleSheet("QPushButton{border: 1px solid gray; border-radius: 5px; background-color: #e3e3e3; padding: 2px 20px 2px 20px;font-size: 10pt;color: black;} "
    //"QPushButton:hover{background-color: Azure; border: 1px solid DodgerBlue;}"); //pretty close but still missing some animations and stuff.

    //natureStuff
    QPushButton *NatureButtons[25];
    for (int i = 0; i<25; i++){
        QString butName = "butN" + QString::number(i);
        NatureButtons[i] = configure::findChild<QPushButton *>(butName);
        NatureButtons[i]->setCheckable(true);//This works
        connect(NatureButtons[i], SIGNAL(toggled(bool)),this, SLOT(natureButtonClicked(bool)));
    }

    QPushButton*HiddenPowerButtons[16];
    for (int i = 0; i<16; i++){
        QString butName = "butH" + QString::number(i);
        HiddenPowerButtons[i] = configure::findChild<QPushButton *>(butName);
        HiddenPowerButtons[i]->setCheckable(true);//This works
        connect(HiddenPowerButtons[i], SIGNAL(toggled(bool)),this, SLOT(hpButtonClicked(bool)));
    }
}

configure::~configure()
{
    delete ui;
}

void configure::on_buttonBox_accepted()
{
    writeReqsToFile();
    configure::accept();
}

void configure::natureButtonClicked(bool checked){
    //Do the same for Hidden power.
    QPushButton *button = (QPushButton *)sender();
    QString butNature = button->objectName();
    butNature = butNature.remove(0,4);
    if (checked){
        naturesChosen.push_back(butNature.toUInt());
        std::sort(naturesChosen.begin(),naturesChosen.end());
    } else {
        std::vector<unsigned int>::iterator iter = std::find(naturesChosen.begin(),naturesChosen.end(),butNature.toUInt());
        int idx = iter-naturesChosen.begin();
        naturesChosen.erase(naturesChosen.begin()+idx);
    }

//    //Debug print
//    QString debugOut;
//    for(unsigned int i = 0; i < naturesChosen.size();i++){
//        debugOut+=QString::number(naturesChosen.at(i)) + " ";
//    }
//    ui->labelDebug->setText(debugOut);

}
void configure::hpButtonClicked(bool checked){
    //Do the same for Hidden power.
    QPushButton *button = (QPushButton *)sender();
    QString butHiddenPower = button->objectName();
    butHiddenPower = butHiddenPower.remove(0,4);
    if (checked){
        hiddenPowersChosen.push_back(butHiddenPower.toUInt());
        std::sort(hiddenPowersChosen.begin(),hiddenPowersChosen.end());
    } else {
        std::vector<unsigned int>::iterator iter = std::find(hiddenPowersChosen.begin(),hiddenPowersChosen.end(),butHiddenPower.toUInt());
        int idx = iter-hiddenPowersChosen.begin();
        hiddenPowersChosen.erase(hiddenPowersChosen.begin()+idx);
    }

//    //Debug print
//    QString debugOut;
//    for(unsigned int i = 0; i < hiddenPowersChosen.size();i++){
//        debugOut+=QString::number(hiddenPowersChosen.at(i)) + " ";
//    }
//    ui->labelDebug->setText(debugOut);

}

void configure::on_buttonBox_rejected()
{
    configure::reject();
}

void configure::on_pushButton_clicked()
{
    ui->inHP->setValue(0); //Could use an enum along with findChild() to put this in a loop
    ui->inNAtk->setValue(0);
    ui->inNDef->setValue(0);
    ui->inNSpA->setValue(0);
    ui->inNSpD->setValue(0);
    ui->inNSpe->setValue(0);
    for (int i = 0;i<25;i++) {
            configure::findChild<QPushButton *>("butN" + QString::number(i))->setChecked(false);
    }
    for (int i = 0;i < 16;i++) {
        configure::findChild<QPushButton *>("butH" + QString::number(i))->setChecked(false);
    }
    naturesChosen.clear();
    hiddenPowersChosen.clear();
    ui->radioAnyGender->setChecked(true);
    ui->radioAnyShiny->setChecked(true);
    ui->spinHidenPower->setValue(0);

}
