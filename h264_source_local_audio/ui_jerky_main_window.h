/********************************************************************************
** Form generated from reading UI file 'jerky_main.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_JERKY_MAIN_WINDOW_H
#define UI_JERKY_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *startStreamBtn;
    QPushButton *pushStreamBtn;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *openUrl;
    QLineEdit *pushUrl;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(436, 219);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        startStreamBtn = new QPushButton(centralwidget);
        startStreamBtn->setObjectName(QStringLiteral("startStreamBtn"));
        startStreamBtn->setGeometry(QRect(110, 130, 75, 23));
        pushStreamBtn = new QPushButton(centralwidget);
        pushStreamBtn->setObjectName(QStringLiteral("pushStreamBtn"));
        pushStreamBtn->setGeometry(QRect(260, 130, 75, 23));
        label = new QLabel(centralwidget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(60, 43, 54, 12));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(60, 80, 54, 12));
        openUrl = new QLineEdit(centralwidget);
        openUrl->setObjectName(QStringLiteral("openUrl"));
        openUrl->setGeometry(QRect(130, 40, 261, 20));
        openUrl->setCursor(QCursor(Qt::ArrowCursor));
        pushUrl = new QLineEdit(centralwidget);
        pushUrl->setObjectName(QStringLiteral("pushUrl"));
        pushUrl->setGeometry(QRect(130, 76, 261, 20));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 436, 23));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);
        QObject::connect(startStreamBtn, SIGNAL(clicked()), MainWindow, SLOT(startStream()));
        QObject::connect(pushStreamBtn, SIGNAL(clicked()), MainWindow, SLOT(pushStream()));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "Jerky", 0));
        startStreamBtn->setText(QApplication::translate("MainWindow", "\346\211\223\345\274\200\346\265\201", 0));
        pushStreamBtn->setText(QApplication::translate("MainWindow", "\346\216\250\351\200\201\346\265\201", 0));
        label->setText(QApplication::translate("MainWindow", "\346\213\211\346\265\201\345\234\260\345\235\200\357\274\232", 0));
        label_2->setText(QApplication::translate("MainWindow", "\346\216\250\346\265\201\345\234\260\345\235\200\357\274\232", 0));
        openUrl->setText(QApplication::translate("MainWindow", "rtmp://192.168.1.158/live/123456", 0));
        pushUrl->setText(QApplication::translate("MainWindow", "rtmp://192.168.1.158/live/654321", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_JERKY_MAIN_WINDOW_H
