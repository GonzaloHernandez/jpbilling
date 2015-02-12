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
    void openProviders();
    void openHomeHistory();
    void openSummaryDebts();
    void openSummaryCollect();
    void openBudgetExecution();

    void loadProvidersType();
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

    void loadHomes();
    void saveHomes();
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
    void loadAccountDetail(int);

    void loadSummaryDebts();
    void showDateSummaryDebts();
    void printSummaryDebts();

    void loadSummaryCollect();

    void loadAccountsBudget(QTreeWidget* widget, QTreeWidgetItem* item=NULL, int handler=0);
    void loadAccountsBudgetTotals(QTreeWidget* widget);

    void switchBudgetMounthHidde();

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

    bool                dbconnected;
    QSqlDatabase        db;
};

#endif // MAINWINDOW_H
