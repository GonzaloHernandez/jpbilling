#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include <QtSql/QSqlDatabase>
#include <ui_paymentswindow.h>
#include <ui_billingwindow.h>
#include <ui_billinglistwindow.h>
#include <ui_collectwindow.h>
#include <ui_pucwindow.h>
#include <ui_homeswindow.h>

#define DEBIT 0
#define CREDIT 1

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void connectDB();
    void disconnectDB();
    void openPayments();
    void openBilling();
    QWidget* openBillingList();
    void openCollect();
    void openPUC();
    void openHomes();

    void loadProvidersType();
    void loadProviders();
    void loadResources();
    void savePayment();
    void saveProvider();

    void loadBillingType();
    void createGeneralBilling();

    void saveGeneralBilling();
    void saveParticularBilling();

    void loadOwners();

    void loadAccounts(QTreeWidget* widget, QTreeWidgetItem* item=NULL, int handler=0);

    void loadHomes();
    void saveHomes();
    void loadHistory();
    void saveCollect();
    void loadSummary();

private:
    Ui::MainWindow*         ui;
    Ui::PaymentsWindow*     uipayments;
    Ui::BillingWindow*      uibilling;
    Ui::BillingListWindow*  uibillinglist;
    Ui::CollectWindow*      uicollect;
    Ui::PUCWindow*          uipuc;
    Ui::HomesWindow*        uihomes;

    bool                dbconnected;
    QSqlDatabase        db;
};

#endif // MAINWINDOW_H
