#-------------------------------------------------
#
# Project created by QtCreator 2014-06-14T20:22:07
#
#-------------------------------------------------

QT       += core gui sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jpbilling
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += \
    mainwindow.h

FORMS    += mainwindow.ui \
    paymentswindow.ui \
    billingwindow.ui \
    billinglistwindow.ui \
    collectwindow.ui \
    pucwindow.ui \
    homeswindow.ui \
    providerswindow.ui \
    homehistorywindow.ui \
    accountdetailwindow.ui \
    specialentry.ui \
    descriptorsbrowser.ui \
    summarydebts.ui \
    summarycollect.ui \
    budgetexecutionwindow.ui

RESOURCES += \
    jpbilling.qrc
