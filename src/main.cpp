#include <QApplication>
#include <QColor>
#include <QStyleFactory>
#include "LoginWindow/LoginWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    LoginWindow loginWindow;
    loginWindow.show();

    return app.exec();
}
