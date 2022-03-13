#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "configure.h"
#include "helpbox.h"
#include "switchbutton.h"
#include "searchlogic.h" //gets processCore from here.
#include "qmessagebox.h"
#include <QClipboard>
#include <CoreFoundation/CFBundle.h>

std::vector<searchResult>eevees;
//declare this directory as a member in mainwindow? would be nice not to have to derive it again
const std::string configPath = std::string(getenv("HOME")) + "/Library/Application Support/PAL-Pal";
const std::string configName = configPath + "/config.txt";

QString formatHex(u32 num){
    return QString::number(num,16).toUpper();
}

void MainWindow::populateInstructionPanel(const path &inPath){
    //battle Now Team data
    battleNowTeamInfo finalRoll = inPath.battleNow;
    QString path = ":/new/rollIcons/QBPokes/";
    QString fileExtension = ".png";
    ui->labelPlayerLeadName->setText(QString::fromStdString(finalRoll.playerLead));
    ui->labelPlayerOtherName->setText(QString::fromStdString(finalRoll.playerOther));
    ui->labelOppLeadName->setText(QString::fromStdString(finalRoll.oppLead));
    ui->labelOppOtherName->setText(QString::fromStdString(finalRoll.oppOther));
    ui->labelPlayerLeadHp->setText(QString::number(finalRoll.playerLeadHp));
    ui->labelPlayerOtherHp->setText(QString::number(finalRoll.playerOtherHp));
    ui->labelOppLeadHp->setText(QString::number(finalRoll.oppLeadHp));
    ui->labelOppOtherHp->setText(QString::number(finalRoll.oppOtherHp));
    ui->picPlayerLead->setPixmap(QPixmap(path + QString::fromStdString(finalRoll.playerLead) + fileExtension));
    ui->picPlayerOther->setPixmap(QPixmap(path + QString::fromStdString(finalRoll.playerOther) + fileExtension));
    ui->picOppLead->setPixmap(QPixmap(path + QString::fromStdString(finalRoll.oppLead) + fileExtension));
    ui->picOppOther->setPixmap(QPixmap(path + QString::fromStdString(finalRoll.oppOther) + fileExtension));
    if (inPath.instructions[reroll] == 0){
        ui->labelPlayerLeadName->setText("?");
        ui->labelPlayerOtherName->setText("?");
        ui->labelOppLeadName->setText("?");
        ui->labelOppOtherName->setText("?");
        ui->picPlayerLead->setPixmap(QPixmap(path + "None" + fileExtension));
        ui->picPlayerOther->setPixmap(QPixmap(path + "None" + fileExtension));
        ui->picOppLead->setPixmap(QPixmap(path + "None" + fileExtension));
        ui->picOppOther->setPixmap(QPixmap(path + "None" + fileExtension));
    }
    std::vector<int>instr = inPath.instructions;
    ui->labelRerolls->setText(QString::number(instr[0]) + " Rerolls");
    ui->labelMemcards->setText(QString::number(instr[1])  + " Memory card reloads");
    ui->labelOptionsSaves->setText(QString::number(instr[2]) + " Options saves");
    ui->labelNameScreens->setText(QString::number(instr[3]) + " Name screen backouts");
    //Show reroll seed? nah, just need inputSeed to debug, and titleSeed helps.
    //ui->labelRerollSeed->setText(formatHex(inPath.battleNow.postSeed));
}
void MainWindow::populateEeveeFrame(const searchResult &input){
    PokemonProperties primary = input.candidate;
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(5) << primary.trainerId;
    //ui->labelEeveeSeed->setText(formatHex(input.listingSeed)); //ListingSeed
    ui->labelTitleSeed->setText(formatHex(input.titleSeed));
    ui->butCopyTitle->setText("Copy");
    ui->labelTid->setText((QString::fromStdString(ss.str())));
    ui->labelHPIV->setText(QString::number(primary.hpIV));
    ui->labelATKIV->setText(QString::number(primary.atkIV));
    ui->labelDEFIV->setText(QString::number(primary.defIV));
    ui->labelSpAIV->setText(QString::number(primary.spAtkIV));
    ui->labelSpDIV->setText(QString::number(primary.spDefIV));
    ui->labelSPEIV->setText(QString::number(primary.speedIV));
    ui->labelHPP->setText(QString::number(primary.hiddenPowerPower));
    ui->labelHPT->setText(QString::fromStdString(hpTypes[primary.hiddenPowerTypeIndex]));
    ui->labelNature->setText(QString::fromStdString(naturesList[primary.natureIndex])); //add blue and red for boosted stats.
    if (primary.genderIndex){
        ui->labelGender->setText("F");
    } else { //WEIRD, loading these characters causes a 1s lag the first time this function is run. Maybe replace with pixmap
        ui->labelGender->setText("M");
    }
    if (primary.isShiny){
        ui->labelShiny->setVisible(true);
    } else {
        ui->labelShiny->setVisible(false);
    }
    //nature colors
    if (primary.natureIndex % 6 != 0){
        int plusStat = (primary.natureIndex / 5) + 1;
        int minusStat = (primary.natureIndex % 5) + 1;
        QString blackColor = "color: black;";
        QString redColor = "color: red;";
        QString blueColor = "color: blue;";
        ui->labelATKIV->setStyleSheet(blackColor);
        ui->labelDEFIV->setStyleSheet(blackColor);
        ui->labelSpAIV->setStyleSheet(blackColor);
        ui->labelSpDIV->setStyleSheet(blackColor);
        ui->labelSPEIV->setStyleSheet(blackColor);
           switch (plusStat)
           {
           case 1:
             ui->labelATKIV->setStyleSheet(redColor);
             break;
           case 2:
             ui->labelDEFIV->setStyleSheet(redColor);
             break;
           case 3:
             ui->labelSPEIV->setStyleSheet(redColor);
             break;
           case 4:
             ui->labelSpAIV->setStyleSheet(redColor);
             break;
           case 5:
             ui->labelSpDIV->setStyleSheet(redColor);
             break;
           }

           switch (minusStat)
           {
           case 1:
             ui->labelATKIV->setStyleSheet(blueColor);
             break;
           case 2:
             ui->labelDEFIV->setStyleSheet(blueColor);
             break;
           case 3:
             ui->labelSPEIV->setStyleSheet(blueColor);
             break;
           case 4:
             ui->labelSpAIV->setStyleSheet(blueColor);
             break;
           case 5:
             ui->labelSpDIV->setStyleSheet(blueColor);
             break;
           }

    }

}
void MainWindow::populateLabels(const searchResult &input){
    MainWindow::populateEeveeFrame(input);
    MainWindow::populateInstructionPanel(input.optimalPath);
}
void MainWindow::depopulateLabels(){
    ui->buttonSearch->setEnabled(false);
    QString boxStyle = "QGroupBox{border:1px solid gray; border-radius: 5px; margin-top: 6px;} QGroupBox::title{subcontrol-origin: margin;left: 7px; padding: 0px 5px 0px 5px;}";
    ui->instructionBox->setStyleSheet(boxStyle);
    ui->instructionBox->setEnabled(false);
    QString frameStyle = "QFrame{border:3px solid dimgray; border-radius: 5px; margin-top: 6px;} QLabel{border:none;}";
    ui->pokeGridFrame->setStyleSheet("QFrame{border:2px solid dimgray; border-radius: 5px; margin-top: 6px;} QLabel{border:none;}");
    ui->EeveeFrame->setStyleSheet(frameStyle);
    ui->labelTitleSeed->setText("?");
    ui->labelTid->setText("?");
    ui->labelHPIV->setText("?");
    ui->labelATKIV->setText("?");
    ui->labelDEFIV->setText("?");
    ui->labelSpAIV->setText("?");
    ui->labelSpDIV->setText("?");
    ui->labelSPEIV->setText("?");
    ui->labelHPP->setText("?");
    ui->labelHPT->setText("?");
    ui->labelNature->setText("?"); //add blue and red for boosted stats.
    ui->labelGender->setText("?");
    ui->labelPlayerLeadName->setText("?");
    ui->labelPlayerOtherName->setText("?");
    ui->labelOppLeadName->setText("?");
    ui->labelOppOtherName->setText("?");
    ui->labelPlayerLeadHp->setText("?");
    ui->labelPlayerOtherHp->setText("?");
    ui->labelOppLeadHp->setText("?");
    ui->labelOppOtherHp->setText("?");
    ui->labelShiny->setVisible(false);
    QString path = ":/new/rollIcons/QBPokes/";
    QString fileExtension = ".png";
    ui->picPlayerLead->setPixmap(QPixmap(path + "None" + fileExtension));
    ui->picPlayerOther->setPixmap(QPixmap(path + "None" + fileExtension));
    ui->picOppLead->setPixmap(QPixmap(path + "None" + fileExtension));
    ui->picOppOther->setPixmap(QPixmap(path + "None" + fileExtension));
    ui->labelRerolls->setText("?");
    ui->labelMemcards->setText("?");
    ui->labelOptionsSaves->setText("?");
    ui->labelNameScreens->setText("?");
    //ui->labelRerollSeed->setText("?");
    ui->seedEntry->setText("");
    ui->customCTT->setText("?");
    ui->customTargetDisplay->setText("0x???");
    ui->labelErrorText->setText("");
    ui->butCopyTitle->setText("Copy");
    ui->butCopyTitle->setEnabled(false);

    QString blackColor = "color: black;";
    ui->labelATKIV->setStyleSheet(blackColor);
    ui->labelDEFIV->setStyleSheet(blackColor);
    ui->labelSpAIV->setStyleSheet(blackColor);
    ui->labelSpDIV->setStyleSheet(blackColor);
    ui->labelSPEIV->setStyleSheet(blackColor);
}
void MainWindow::checkFile(){
    //check for valid config, else lock it up.
    //right now only # of lines is checked for, regardless of content.
    //If people break this somehow then i'll implement more rigourous validation.
    int lineCount = 0;
    std::ifstream existing(configName);
    std::string empt;
    if (existing.fail()){
    //label "No configuration set!
        qInfo() << "Creating folder at: " << QString::fromStdString(configPath) << "\n";
        qInfo() << "Home Directory: " << getenv("HOME");
        if (mkdir(configPath.c_str(),0777) == -1){
            qInfo() << "ERROR, FILE FOLDER EXISTS";
        } else {
            qInfo() << "FILE FOLDER SUCCESS!";
        }
    }
    while(!existing.fail()){
        lineCount++;
        existing >> empt;
        //ui->labelTitleSeed->setText(QString::number(lineCount));
    }
    if (lineCount >= 12){
         //minimum # of lines, assuming 1 nature and one hp
        //NOTE THAT NOT FILTERING FOR NATURE/HP IS EQUIVALENT TO FILTERING FOR ALL OF THEM
        ui->seedEntry->setEnabled(true);
        ui->butPasteInput->setEnabled(true);
        ui->EeveeFrame->setEnabled(true);
        ui->titleSeedGrid->setEnabled(true);
        ui->seedInputContainer->setEnabled(true);
        ui->ConfigStatusLabel->setText("Configuration set!");
    } else {
        ui->ConfigStatusLabel->setText("Configuration not set!");
        depopulateLabels();
        ui->EeveeFrame->setEnabled(false);
        ui->titleSeedGrid->setEnabled(false);
        ui->seedInputContainer->setEnabled(false);
        ui->seedEntry->setEnabled(false);
        ui->buttonSearch->setEnabled(false);
        ui->buttonReset->setEnabled(false);
        ui->buttonNext->setEnabled(false);
        ui->buttonPrevious->setEnabled(false);
        ui->butPasteInput->setEnabled(false);
    }
    //ui->labelTitleSeed->setText(QString::number(lineCount);
    existing.clear();
    existing.seekg(0);
    existing.close();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    CFBundleRef mainBundle = CFBundleGetMainBundle();
//    CFURLRef ppIcon = CFBundleCopyResourceURL(mainBundle,CFSTR("palPalIcon"), CFSTR("icns"),NULL);
//    std::string iconStr;
//    if (ppIcon != NULL){
//        qInfo() << "Found icon!";
//        iconStr = CFStringGetCStringPtr(CFURLGetString(ppIcon),kCFStringEncodingUTF8);
//        iconStr.erase(0,7);
//        qInfo() << "Icon path: " << QString::fromStdString(iconStr);
//    } else {
//        qInfo() << "Didn't find icon!";
//    }
//    this->setWindowIcon(QString::fromStdString(iconStr));
    depopulateLabels();
    checkFile();

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_buttonSearch_clicked()
{
    eevees.clear();
    const u32 referenceSeed = ui->seedEntry->text().toUInt(nullptr,16); //call it input cleansing.
    searchResult searchData = searchForEevee(referenceSeed,referenceSeed,true);
    populateLabels(searchData);
    eevees.push_back(searchData);
    ui->buttonPrevious->setEnabled(false);
    ui->instructionBox->setEnabled(true);
    ui->butCopyTitle->setEnabled(true);
    ui->buttonNext->setEnabled(true);
    ui->buttonReset->setEnabled(true);
}

void MainWindow::on_openConfigureMenu_clicked()
{
    configure mConfigure;
    mConfigure.setModal(true);
    mConfigure.exec();
    checkFile();
}

void MainWindow::on_buttonReset_clicked()
{
    depopulateLabels();
    eevees.clear();
    ui->buttonNext->setEnabled(false);
    ui->buttonPrevious->setEnabled(false);
}

void MainWindow::on_buttonNext_clicked()
{
    //add current eevee to some kind of vector, so that it can be restored via previous after next is complete.
    const u32 referenceSeed = ui->seedEntry->text().toUInt(nullptr,16);
    u32 currentSeed = eevees.back().listingSeed;
    searchResult searchData = searchForEevee(referenceSeed,currentSeed,false);
    populateLabels(searchData);
    eevees.push_back(searchData);
    if (ui->buttonPrevious->isEnabled() == false){
        ui->buttonPrevious->setEnabled(true);
        //not perfect.
    }
}

void MainWindow::on_buttonPrevious_clicked()
{
    if (eevees.size() >= 2){
        eevees.pop_back();//decrement
        populateLabels(eevees.back());
        if (eevees.size() == 1){
            ui->buttonPrevious->setEnabled(false);
        }
    } else {
        //This shouldn't ever happen, but it corrects itself if there's a bug.
        ui->buttonPrevious->setEnabled(false);

    }
}

void MainWindow::on_buttonHelp_clicked()
{
//Message box is cool but oh so manual. No real resizing, will need to go the whole 9 yards.
    helpBox mHelp;
    mHelp.setModal(false);
    mHelp.exec();
}


void MainWindow::on_butCustomReset_clicked()
{
    depopulateLabels();
    ui->butCustomReset->setEnabled(false);
    ui->butCustomSearch->setEnabled(false);
    eevees.clear();
    ui->buttonNext->setEnabled(false);
    ui->buttonPrevious->setEnabled(false);
    ui->seedEntry_2->setText("");
    ui->targetEntry->setText("");
}

void MainWindow::on_butCustomSearch_clicked()
{
    u32 customInSeed = ui->seedEntry_2->text().toUInt(nullptr,16);
    u32 targetSeed = ui->targetEntry->text().toUInt(nullptr,16);
    ui->customTargetDisplay->setText("0x" + formatHex(targetSeed));
    ui->instructionBox->setEnabled(true);
    int ctt = findGap(customInSeed,targetSeed,true);

    if (ctt == -1){
    ui->labelErrorText->setText("Error, seeds are reversed!");
    } else {
        if (CTTAcceptable(customInSeed,ctt)){
            ui->customCTT->setText(QString::number(ctt) + " advances away.");
            //Findpath.
            path result = findPath(customInSeed,targetSeed);
            populateInstructionPanel(result);
         } else {
            depopulateLabels();
            ui->labelErrorText->setText(

            "Unfortunately, no path was found to this seed using the tool's methods. Path to nearest seed shown."
            );
        }
        ui->butCustomReset->setEnabled(true);

    }

}

void MainWindow::on_seedEntry_returnPressed()
{
    on_buttonSearch_clicked();
}

//input validation
void MainWindow::on_seedEntry_textEdited(const QString &arg1)
{
    ui->buttonNext->setEnabled(false);
    bool validInput;
    arg1.toUInt(&validInput,16);
    ui->buttonSearch->setEnabled(validInput);
}

void MainWindow::on_seedEntry_2_textEdited(const QString &arg1)
{
    bool validInput;
    bool validTarget;
    arg1.toUInt(&validInput,16);
    ui->targetEntry->text().toUInt(&validTarget,16);
    ui->butCustomSearch->setEnabled(validInput&validTarget);
}

void MainWindow::on_targetEntry_textEdited()
{
    on_seedEntry_2_textEdited(ui->seedEntry_2->text());
}

void MainWindow::on_butPasteInput_clicked()
{
    ui->seedEntry->setText("");
    ui->seedEntry->paste();
}

void MainWindow::on_butCopyTitle_clicked()
{
    QClipboard *clip = QApplication::clipboard();
    clip->setText(ui->labelTitleSeed->text());
    ui->butCopyTitle->setText("Copied!");
}

void MainWindow::on_butCustomCurrentPaste_clicked()
{
    ui->seedEntry_2->setText("");
    ui->seedEntry_2->paste();
}

void MainWindow::on_butCustomTargetPaste_clicked()
{
    ui->targetEntry->setText("");
    ui->targetEntry->paste();
}
