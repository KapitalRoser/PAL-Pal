#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "searchlogic.h"
#include <sys/stat.h>
#include <sys/types.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void populateInstructionPanel(const path &input);
    void populateEeveeFrame(const searchResult &input);
    void populateLabels(const searchResult &input);
    void depopulateLabels();
    void checkFile();
    ~MainWindow();

private slots:

    void on_buttonSearch_clicked();

    void on_openConfigureMenu_clicked();

    void on_buttonReset_clicked();

    void on_buttonNext_clicked();

    void on_buttonPrevious_clicked();

    void on_buttonHelp_clicked();

    void on_butCustomReset_clicked();

    void on_butCustomSearch_clicked();

    void on_seedEntry_returnPressed();

    void on_seedEntry_textEdited(const QString &arg1);

    void on_seedEntry_2_textEdited(const QString &arg1);

    void on_targetEntry_textEdited();


    void on_butPasteInput_clicked();

    void on_butCopyTitle_clicked();

    void on_butCustomCurrentPaste_clicked();

    void on_butCustomTargetPaste_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
