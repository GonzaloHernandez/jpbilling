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
#include <ui_providerswindow.h>
#include <ui_homehistorywindow.h>
#include <ui_accountdetailwindow.h>
#include <ui_summarydebts.h>
#include <ui_summarycollect.h>
#include <ui_budgetexecutionwindow.h>
#include <ui_specialentry.h>

#define DEBIT 0
#define CREDIT 1

namespace Ui {
class MainWindow;
}
/**
 * @brief The MainWindow class provide the all functionality to the application.
 * This class has list the all methods, which be developed in the mainwindow.cpp file.
 * To design this interface was coded its characteristics in
 * <a href="https://github.com/GonzaloHernandez/jpbilling/blob/master/mainwindow.ui">
 * mainwindow.ui</a> file using XML format.
 * <img src="screenshoots/mainwindow.png">
 */
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
    void openProviders();
    void openHomeHistory();
    void openSummaryDebts();
    void openSummaryCollect();
    void openBudgetExecution();

    void loadPaymentType();
    void loadProviders();
    void loadResources();
    void savePayment();
    void saveProvider();
    void showDatePayment();
    void showDateBilling();
    void showDatePBilling();
    void showDateCollect();

    void loadBillingType();
    void createGeneralBilling();
    void loadParms();
    void loadPenalties();
    void savePenalties();
    void showDatePenaltiesSince();
    void showDatePenaltiesAt();
    void printBilling(QModelIndex);
    void saveGeneralBilling();
    void saveParticularBilling();

    void loadOwners();

    void loadAccounts(QTreeWidget* widget, QTreeWidgetItem* item=NULL, int handler=0);
    void modifyAccount(QPoint);

    void loadHomes();
    void addHome();
    void saveHomes();
    void homeModified(int,int);
    void loadProvidersWindow();
    void saveProvidersWindow();

    void loadDebt();
    void saveCollect();
    void resumeCollect(int,int);

    void loadHistory();
    void printHistory();
    void highlight(QModelIndex);
    void highlight();

    void openAccountDetail();
    void loadAccountDetail(int,int year=0,int month=0);

    void loadSummaryDebts();
    void showDateSummaryDebts();
    void printSummaryDebts();

    void loadSummaryCollect();

    void loadAccountsBudget(QTreeWidget* widget, QTreeWidgetItem* item=NULL, int handler=0);
    void loadAccountsBudgetTotals(QTreeWidget* widget);

    void switchBudgetMounthHidde();
    void adjustBudgetView();
    void openBudgetDetail(QTreeWidgetItem*,int);
    void openAccountDetail(int account,int year,int month);
    void changeBudgetYear(int);
    void printBudget();

    void createBackup();

    void openAbout();

    void openSpecialEntry();

private:
    Ui::MainWindow*             ui;
    Ui::PaymentsWindow*         uipayments;
    Ui::BillingWindow*          uibilling;
    Ui::BillingListWindow*      uibillinglist;
    Ui::CollectWindow*          uicollect;
    Ui::PUCWindow*              uipuc;
    Ui::HomesWindow*            uihomes;
    Ui::ProvidersWindow*        uiproviders;
    Ui::HomeHistoryWindow*      uihomehistory;
    Ui::AccountDetail*          uiaccountdetail;
    Ui::SummaryDebtsWindow*     uisummarydebts;
    Ui::SummaryCollectWindow*   uisummarycollect;
    Ui::BudgetExecutionWindow*  uibudgetexecution;
    Ui::SpecialEntry*           uispecialentry;

    bool                dbconnected;
    QSqlDatabase        db;
};

#endif // MAINWINDOW_H
