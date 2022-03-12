#include "mainwindow.h"

#include <QApplication>
#include <CoreFoundation/CFBundle.h>
int main(int argc, char *argv[])
{
//    CFBundleRef mainBundle = CFBundleGetMainBundle();
//     CFURLRef ppIcon = CFBundleCopyResourceURL(mainBundle,CFSTR("palPalIcon"), CFSTR("icns"),NULL);
//     std::string iconStr;
//     if (ppIcon != NULL){
//         qInfo() << "Found icon!";
//         iconStr = CFStringGetCStringPtr(CFURLGetString(ppIcon),kCFStringEncodingUTF8);
//         iconStr.erase(0,7);
//         qInfo() << "Icon path: " << QString::fromStdString(iconStr);
//     } else {
//         qInfo() << "Didn't find icon!";
//     }

    QApplication a(argc, argv);
    MainWindow w;
//    w.setWindowIcon(QIcon(QString::fromStdString(iconStr)));
    w.show();
    return a.exec();
}
