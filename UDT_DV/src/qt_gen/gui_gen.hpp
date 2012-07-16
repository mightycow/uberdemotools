/********************************************************************************
** Form generated from reading UI file 'gui.ui'
**
** Created: Mon 16. Jul 18:28:56 2012
**      by: Qt User Interface Compiler version 4.8.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef GUI_GEN_H
#define GUI_GEN_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>
#include "paint_widget.h"

QT_BEGIN_NAMESPACE

class Ui_MyClassClass
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *pathLabel;
    QLineEdit *pathLineEdit;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QDoubleSpinBox *timeScaleDoubleSpinBox;
    QCheckBox *reverseCheckBox;
    QCheckBox *showClockCheckBox;
    QCheckBox *showScoresCheckBox;
    QCheckBox *showHudCheckBox;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout;
    QPushButton *playButton;
    QPushButton *stopButton;
    QSlider *progressSlider;
    QSpacerItem *verticalSpacer;
    PaintWidget *paintWidget;

    void setupUi(QMainWindow *MyClassClass)
    {
        if (MyClassClass->objectName().isEmpty())
            MyClassClass->setObjectName(QString::fromUtf8("MyClassClass"));
        MyClassClass->resize(812, 901);
        MyClassClass->setContextMenuPolicy(Qt::NoContextMenu);
        MyClassClass->setAcceptDrops(true);
        MyClassClass->setWindowOpacity(1);
        MyClassClass->setLayoutDirection(Qt::LeftToRight);
        centralWidget = new QWidget(MyClassClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setAutoFillBackground(false);
        gridLayout_2 = new QGridLayout(centralWidget);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(6, 6, 6, 6);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        pathLabel = new QLabel(centralWidget);
        pathLabel->setObjectName(QString::fromUtf8("pathLabel"));

        horizontalLayout_2->addWidget(pathLabel);

        pathLineEdit = new QLineEdit(centralWidget);
        pathLineEdit->setObjectName(QString::fromUtf8("pathLineEdit"));
        pathLineEdit->setMinimumSize(QSize(600, 0));

        horizontalLayout_2->addWidget(pathLineEdit);


        gridLayout->addLayout(horizontalLayout_2, 2, 1, 1, 1);


        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label = new QLabel(centralWidget);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_3->addWidget(label);

        timeScaleDoubleSpinBox = new QDoubleSpinBox(centralWidget);
        timeScaleDoubleSpinBox->setObjectName(QString::fromUtf8("timeScaleDoubleSpinBox"));
        timeScaleDoubleSpinBox->setDecimals(1);
        timeScaleDoubleSpinBox->setMinimum(0);
        timeScaleDoubleSpinBox->setMaximum(50);
        timeScaleDoubleSpinBox->setSingleStep(0.5);
        timeScaleDoubleSpinBox->setValue(1);

        horizontalLayout_3->addWidget(timeScaleDoubleSpinBox);

        reverseCheckBox = new QCheckBox(centralWidget);
        reverseCheckBox->setObjectName(QString::fromUtf8("reverseCheckBox"));

        horizontalLayout_3->addWidget(reverseCheckBox);

        showClockCheckBox = new QCheckBox(centralWidget);
        showClockCheckBox->setObjectName(QString::fromUtf8("showClockCheckBox"));
        showClockCheckBox->setChecked(true);

        horizontalLayout_3->addWidget(showClockCheckBox);

        showScoresCheckBox = new QCheckBox(centralWidget);
        showScoresCheckBox->setObjectName(QString::fromUtf8("showScoresCheckBox"));
        showScoresCheckBox->setChecked(true);

        horizontalLayout_3->addWidget(showScoresCheckBox);

        showHudCheckBox = new QCheckBox(centralWidget);
        showHudCheckBox->setObjectName(QString::fromUtf8("showHudCheckBox"));
        showHudCheckBox->setChecked(true);

        horizontalLayout_3->addWidget(showHudCheckBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);


        gridLayout_2->addLayout(horizontalLayout_3, 2, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        playButton = new QPushButton(centralWidget);
        playButton->setObjectName(QString::fromUtf8("playButton"));

        horizontalLayout->addWidget(playButton);

        stopButton = new QPushButton(centralWidget);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));

        horizontalLayout->addWidget(stopButton);

        progressSlider = new QSlider(centralWidget);
        progressSlider->setObjectName(QString::fromUtf8("progressSlider"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(progressSlider->sizePolicy().hasHeightForWidth());
        progressSlider->setSizePolicy(sizePolicy);
        progressSlider->setMaximum(200);
        progressSlider->setOrientation(Qt::Horizontal);
        progressSlider->setInvertedControls(false);
        progressSlider->setTickPosition(QSlider::NoTicks);
        progressSlider->setTickInterval(0);

        horizontalLayout->addWidget(progressSlider);


        gridLayout_2->addLayout(horizontalLayout, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 5, 0, 1, 1);

        paintWidget = new PaintWidget(centralWidget);
        paintWidget->setObjectName(QString::fromUtf8("paintWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(paintWidget->sizePolicy().hasHeightForWidth());
        paintWidget->setSizePolicy(sizePolicy1);
        paintWidget->setMinimumSize(QSize(800, 800));

        gridLayout_2->addWidget(paintWidget, 3, 0, 1, 1);

        MyClassClass->setCentralWidget(centralWidget);

        retranslateUi(MyClassClass);

        QMetaObject::connectSlotsByName(MyClassClass);
    } // setupUi

    void retranslateUi(QMainWindow *MyClassClass)
    {
        MyClassClass->setWindowTitle(QApplication::translate("MyClassClass", "2D Demo Viewer", 0, QApplication::UnicodeUTF8));
        pathLabel->setText(QApplication::translate("MyClassClass", "Demo Path:", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MyClassClass", "Timescale:", 0, QApplication::UnicodeUTF8));
        timeScaleDoubleSpinBox->setPrefix(QString());
        reverseCheckBox->setText(QApplication::translate("MyClassClass", "Reverse Time", 0, QApplication::UnicodeUTF8));
        showClockCheckBox->setText(QApplication::translate("MyClassClass", "Show Clock", 0, QApplication::UnicodeUTF8));
        showScoresCheckBox->setText(QApplication::translate("MyClassClass", "Show Scores", 0, QApplication::UnicodeUTF8));
        showHudCheckBox->setText(QApplication::translate("MyClassClass", "Show HUD", 0, QApplication::UnicodeUTF8));
        playButton->setText(QApplication::translate("MyClassClass", "Play", 0, QApplication::UnicodeUTF8));
        stopButton->setText(QApplication::translate("MyClassClass", "Stop", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MyClassClass: public Ui_MyClassClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // GUI_GEN_H
