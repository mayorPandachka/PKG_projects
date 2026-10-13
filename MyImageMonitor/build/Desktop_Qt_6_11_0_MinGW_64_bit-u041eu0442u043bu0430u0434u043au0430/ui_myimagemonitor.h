/********************************************************************************
** Form generated from reading UI file 'myimagemonitor.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MYIMAGEMONITOR_H
#define UI_MYIMAGEMONITOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MyImageMonitor
{
public:
    QWidget *centralwidget;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QPushButton *btnOpenFolder;
    QPushButton *btnAddFiles;
    QPushButton *btnExportCsv;
    QPushButton *btnClear;
    QSpacerItem *spacerItem;
    QTableWidget *tableWidget;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MyImageMonitor)
    {
        if (MyImageMonitor->objectName().isEmpty())
            MyImageMonitor->setObjectName("MyImageMonitor");
        MyImageMonitor->resize(1200, 700);
        centralwidget = new QWidget(MyImageMonitor);
        centralwidget->setObjectName("centralwidget");
        vboxLayout = new QVBoxLayout(centralwidget);
        vboxLayout->setObjectName("vboxLayout");
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName("hboxLayout");
        btnOpenFolder = new QPushButton(centralwidget);
        btnOpenFolder->setObjectName("btnOpenFolder");

        hboxLayout->addWidget(btnOpenFolder);

        btnAddFiles = new QPushButton(centralwidget);
        btnAddFiles->setObjectName("btnAddFiles");

        hboxLayout->addWidget(btnAddFiles);

        btnExportCsv = new QPushButton(centralwidget);
        btnExportCsv->setObjectName("btnExportCsv");

        hboxLayout->addWidget(btnExportCsv);

        btnClear = new QPushButton(centralwidget);
        btnClear->setObjectName("btnClear");

        hboxLayout->addWidget(btnClear);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addLayout(hboxLayout);

        tableWidget = new QTableWidget(centralwidget);
        tableWidget->setObjectName("tableWidget");

        vboxLayout->addWidget(tableWidget);

        MyImageMonitor->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MyImageMonitor);
        menubar->setObjectName("menubar");
        MyImageMonitor->setMenuBar(menubar);
        statusbar = new QStatusBar(MyImageMonitor);
        statusbar->setObjectName("statusbar");
        MyImageMonitor->setStatusBar(statusbar);

        retranslateUi(MyImageMonitor);

        QMetaObject::connectSlotsByName(MyImageMonitor);
    } // setupUi

    void retranslateUi(QMainWindow *MyImageMonitor)
    {
        MyImageMonitor->setWindowTitle(QCoreApplication::translate("MyImageMonitor", "Image Monitor", nullptr));
        btnOpenFolder->setText(QCoreApplication::translate("MyImageMonitor", "\320\236\321\202\320\272\321\200\321\213\321\202\321\214 \320\277\320\260\320\277\320\272\321\203...", nullptr));
        btnAddFiles->setText(QCoreApplication::translate("MyImageMonitor", "\320\224\320\276\320\261\320\260\320\262\320\270\321\202\321\214 \321\204\320\260\320\271\320\273\321\213...", nullptr));
        btnExportCsv->setText(QCoreApplication::translate("MyImageMonitor", "\320\255\320\272\321\201\320\277\320\276\321\200\321\202 CSV", nullptr));
        btnClear->setText(QCoreApplication::translate("MyImageMonitor", "\320\236\321\207\320\270\321\201\321\202\320\270\321\202\321\214", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MyImageMonitor: public Ui_MyImageMonitor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MYIMAGEMONITOR_H
