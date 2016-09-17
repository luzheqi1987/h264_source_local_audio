/********************************************************************************
** Form generated from reading UI file 'h264_source_local_audio.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_H264_SOURCE_LOCAL_AUDIO_H
#define UI_H264_SOURCE_LOCAL_AUDIO_H

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
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_h264_source_local_audioClass
{
public:
    QWidget *centralWidget;
    QLineEdit *openUrl;
    QLabel *label;
    QLineEdit *openUrl_2;
    QLabel *label_2;
    QPushButton *startStreamBtn;
    QPushButton *pushButton;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *h264_source_local_audioClass)
    {
        if (h264_source_local_audioClass->objectName().isEmpty())
            h264_source_local_audioClass->setObjectName(QStringLiteral("h264_source_local_audioClass"));
        h264_source_local_audioClass->resize(600, 400);
        centralWidget = new QWidget(h264_source_local_audioClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        openUrl = new QLineEdit(centralWidget);
        openUrl->setObjectName(QStringLiteral("openUrl"));
        openUrl->setGeometry(QRect(167, 96, 321, 20));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(100, 100, 54, 12));
        openUrl_2 = new QLineEdit(centralWidget);
        openUrl_2->setObjectName(QStringLiteral("openUrl_2"));
        openUrl_2->setGeometry(QRect(168, 147, 321, 20));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(100, 150, 54, 12));
        startStreamBtn = new QPushButton(centralWidget);
        startStreamBtn->setObjectName(QStringLiteral("startStreamBtn"));
        startStreamBtn->setGeometry(QRect(180, 210, 75, 23));
        pushButton = new QPushButton(centralWidget);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(320, 210, 75, 23));
        h264_source_local_audioClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(h264_source_local_audioClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 23));
        h264_source_local_audioClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(h264_source_local_audioClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        h264_source_local_audioClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        h264_source_local_audioClass->insertToolBarBreak(mainToolBar);
        statusBar = new QStatusBar(h264_source_local_audioClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        h264_source_local_audioClass->setStatusBar(statusBar);

        retranslateUi(h264_source_local_audioClass);

        QMetaObject::connectSlotsByName(h264_source_local_audioClass);
    } // setupUi

    void retranslateUi(QMainWindow *h264_source_local_audioClass)
    {
        h264_source_local_audioClass->setWindowTitle(QApplication::translate("h264_source_local_audioClass", "h264_source_local_audio", 0));
        openUrl->setText(QApplication::translate("h264_source_local_audioClass", "rtmp://gs.live.rgbvr.com/live/123456", 0));
        label->setText(QApplication::translate("h264_source_local_audioClass", "\346\213\211\346\265\201\345\234\260\345\235\200\357\274\232", 0));
        openUrl_2->setText(QApplication::translate("h264_source_local_audioClass", "rtmp://gs.push.rgbvr.com/live/654321", 0));
        label_2->setText(QApplication::translate("h264_source_local_audioClass", "\346\216\250\346\265\201\345\234\260\345\235\200\357\274\232", 0));
        startStreamBtn->setText(QApplication::translate("h264_source_local_audioClass", "\345\274\200\345\247\213\346\213\211\346\265\201", 0));
        pushButton->setText(QApplication::translate("h264_source_local_audioClass", "\345\274\200\345\247\213\346\216\250\346\265\201", 0));
    } // retranslateUi

};

namespace Ui {
    class h264_source_local_audioClass: public Ui_h264_source_local_audioClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_H264_SOURCE_LOCAL_AUDIO_H
