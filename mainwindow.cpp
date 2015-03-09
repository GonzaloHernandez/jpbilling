#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_paymentswindow.h"
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QMessageBox>
#include <iostream>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QItemDelegate>
#include <QInputDialog>
#include <QDir>
#include <QFileDialog>
#include <QProcess>

#define ch(x) (int)(10200.0*x/200.0)
#define cv(x) (int)(13200.0*x/270.0)

using namespace std;

class MyItemDelegate : public QItemDelegate {
private:
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
    {
        int x = (rect.x()+rect.width()-1);

        painter->setPen(Qt::darkGray);
        painter->drawLine(x, rect.y(), x, (rect.y()+rect.height()));
        painter->drawLine(rect.x(), rect.y()+rect.height()-1, x,rect.y()+rect.height()-1);
        QItemDelegate::drawDisplay(painter, option, rect, text);
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(1000,700);
    dbconnected = false;
    connectDB();
}

MainWindow::~MainWindow()
{
    delete ui;
    disconnectDB();
}

void MainWindow::connectDB()
{
  db = QSqlDatabase::addDatabase("QMYSQL");
  db.setHostName("localhost");
  db.setDatabaseName("accounting");
  db.setUserName("root");
  db.setPassword("123");
  dbconnected = db.open();
}

void MainWindow::disconnectDB()
{
  if (dbconnected) {
    db.close();
  }
}

void MainWindow::openPayments()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Pagos") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uipayments = new Ui::PaymentsWindow;
  uipayments->setupUi(subwindow);
  uipayments->dateedit_date->setDate(QDate::currentDate());
  subwindow->setWindowTitle("Pagos");
  subwindow->setMinimumWidth(400);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  connect(uipayments->button_savepayment,SIGNAL(clicked()),this,SLOT(savePayment()));
  connect(uipayments->button_saveprovider,SIGNAL(clicked()),this,SLOT(saveProvider()));
  //connect(uipayments->combobox_type,SIGNAL(),uipayments->combobox_provider,SLOT(setFocus()));
  connect(uipayments->combobox_provider,SIGNAL(currentTextChanged(QString)),uipayments->dateedit_date,SLOT(setFocus()));
  connect(uipayments->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDatePayment()));
  connect(uipayments->lineedit_value,SIGNAL(editingFinished()),uipayments->lineedit_detail,SLOT(setFocus()));
  connect(uipayments->lineedit_detail,SIGNAL(editingFinished()),uipayments->lineedit_voucher,SLOT(setFocus()));
  loadPaymentType();
  loadProviders();
  loadResources();
}

void MainWindow::openBilling()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Facturación") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uibilling = new Ui::BillingWindow;
  uibilling->setupUi(subwindow);
  subwindow->setWindowTitle("Facturación");
  subwindow->setMinimumWidth(460);
  ui->mdiArea->addSubWindow(subwindow);
  uibilling->dateedit_date->setDate(QDate::currentDate());
  uibilling->dateedit_p_date->setDate(QDate::currentDate());
  QStringList labels;
  labels<<"Casa"<<"Cuota"<<"Deuda"<<"Multa";
  uibilling->table_penalties->setHorizontalHeaderLabels(labels);
  uibilling->table_penalties->setColumnWidth(0,80);
  uibilling->table_penalties->setColumnWidth(1,150);
  uibilling->table_penalties->setColumnWidth(2,80);
  uibilling->table_penalties->setColumnWidth(3,80);
  labels.clear();
  labels<<"Valor expensa"<<"Valor multa";
  uibilling->table_parms->setHorizontalHeaderLabels(labels);
  uibilling->table_parms->setColumnWidth(0,100);
  uibilling->table_parms->setColumnWidth(1,100);

  uibilling->dateedit_at->setDate(QDate::currentDate());
  subwindow->show();
  connect(uibilling->button_savegeneralbilling,SIGNAL(clicked()),this,SLOT(createGeneralBilling()));
  connect(uibilling->button_saveparticularbilling,SIGNAL(clicked()),this,SLOT(saveParticularBilling()));
  connect(uibilling->button_generate,SIGNAL(clicked()),this,SLOT(loadPenalties()));
  connect(uibilling->button_savepenalties,SIGNAL(clicked()),this,SLOT(savePenalties()));
  connect(uibilling->checkbox_voucher,SIGNAL(clicked(bool)),uibilling->lineedit_voucher,SLOT(setEnabled(bool)));
  connect(uibilling->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDateBilling()));
  connect(uibilling->dateedit_p_date,SIGNAL(editingFinished()),this,SLOT(showDatePBilling()));
  connect(uibilling->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(loadParms()));
  connect(uibilling->dateedit_since,SIGNAL(editingFinished()),this,SLOT(showDatePenaltiesSince()));
  connect(uibilling->dateedit_at,SIGNAL(editingFinished()),this,SLOT(showDatePenaltiesAt()));
  loadBillingType();
}

QWidget* MainWindow::openBillingList()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Facturación general") {
      subwindow->activateWindow();
      return NULL;
    }
  }

  QWidget* subwindow = new QWidget;
  uibillinglist = new Ui::BillingListWindow;
  uibillinglist->setupUi(subwindow);
  subwindow->setWindowTitle("Facturación general");
  subwindow->setMinimumWidth(330);
  ui->mdiArea->addSubWindow(subwindow);
  QStringList labels;
  labels = labels <<"Casa"<<"Cobro"<<"CI";
  uibillinglist->table_billing->setHorizontalHeaderLabels(labels);

  QSqlQuery query;
  query.exec("SELECT number,name FROM accounts WHERE handler = 1320");
  int i=3;
  while (query.next()) {
    QTableWidgetItem* headeritem = new QTableWidgetItem(query.value(0).toString());
    headeritem->setToolTip(query.value(1).toString());
    uibillinglist->table_billing->setHorizontalHeaderItem(i,headeritem);
    i++;
  }
  uibillinglist->table_billing->setHorizontalHeaderItem(9,new QTableWidgetItem("Total"));
  uibillinglist->table_billing->setHorizontalHeaderItem(10,new QTableWidgetItem("Anticipo"));

  uibillinglist->table_billing->setColumnWidth(0,60);
  uibillinglist->table_billing->setColumnWidth(1,80);
  uibillinglist->table_billing->setColumnWidth(2,50);
  uibillinglist->table_billing->setColumnWidth(3,80);
  uibillinglist->table_billing->setColumnWidth(4,80);
  uibillinglist->table_billing->setColumnWidth(5,80);
  uibillinglist->table_billing->setColumnWidth(6,80);
  uibillinglist->table_billing->setColumnWidth(7,80);
  uibillinglist->table_billing->setColumnWidth(8,80);
  uibillinglist->table_billing->setColumnWidth(9,80);
  uibillinglist->label_number->setHidden(true);
  uibillinglist->label_detail->setHidden(true);
  uibillinglist->label_account0->setHidden(true);
  uibillinglist->label_account1->setHidden(true);
  uibillinglist->label_date->setHidden(true);
  connect(uibillinglist->button_save,SIGNAL(clicked()),this,SLOT(saveGeneralBilling()));
  connect(uibillinglist->table_billing,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(printBilling(QModelIndex)));
  return subwindow;
}

void MainWindow::openCollect()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Recaudo") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uicollect = new Ui::CollectWindow;
  uicollect->setupUi(subwindow);
  subwindow->setWindowTitle("Recaudo");
  subwindow->setMinimumWidth(610);
  ui->mdiArea->addSubWindow(subwindow);
  QStringList labels;
  uicollect->table_detail->setHorizontalHeaderLabels(labels<<"Concepto"<<"Detalle"<<"Deuda"<<"Valor");
  uicollect->table_detail->setColumnWidth(0,190);
  uicollect->table_detail->setColumnWidth(1,170);
  uicollect->table_detail->setColumnWidth(2, 90);
  uicollect->table_detail->setColumnWidth(3, 90);
  uicollect->table_detail->setColumnHidden(4,true);
  uicollect->table_detail->setColumnHidden(5,true);
  uicollect->table_detail->setColumnHidden(6,true);
  subwindow->show();
  connect(uicollect->lineedit_home,SIGNAL(returnPressed()),this,SLOT(loadDebt()));
  connect(uicollect->button_save,SIGNAL(clicked()),this,SLOT(saveCollect()));
  connect(uicollect->table_detail,SIGNAL(cellChanged(int,int)),this,SLOT(resumeCollect(int,int)));
  connect(uicollect->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDateCollect()));
  connect(uicollect->lineedit_voucher,SIGNAL(returnPressed()),uicollect->table_detail,SLOT(setFocus()));
  uicollect->dateedit_date->setDate(QDate::currentDate());
  uicollect->lineedit_home->setFocus();
}

void MainWindow::openPUC()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Plan Unico de Cuentas") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uipuc = new Ui::PUCWindow;
  uipuc->setupUi(subwindow);
  subwindow->setWindowTitle("Plan Unico de Cuentas");
  subwindow->setMinimumWidth(450);
  subwindow->setMinimumHeight(400);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  QStringList labels("Cuentas");
  uipuc->tree_puc->setHeaderLabels(labels);
  connect(uipuc->button_detail,SIGNAL(clicked()),this,SLOT(openAccountDetail()));
  loadAccounts(uipuc->tree_puc);
}

void MainWindow::openHomes()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Casas en el Condominio") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uihomes = new Ui::HomesWindow;
  uihomes->setupUi(subwindow);
  subwindow->setWindowTitle("Casas en el Condominio");
  subwindow->setMinimumWidth(455);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  QStringList labels;
  labels<<"Identificación"<<"Propietario"<<"Teléfono";
  uihomes->table_homes->setHorizontalHeaderLabels(labels);
  uihomes->table_homes->setColumnWidth(0, 80);
  uihomes->table_homes->setColumnWidth(1,250);
  connect(uihomes->button_save,SIGNAL(clicked()),this,SLOT(saveHomes()));
  connect(uihomes->button_reload,SIGNAL(clicked()),this,SLOT(loadHomes()));
  loadHomes();
}

void MainWindow::openProviders()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Proveedores o Beneficiarios") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uiproviders = new Ui::ProvidersWindow;
  uiproviders->setupUi(subwindow);
  subwindow->setWindowTitle("Proveedores de servicios");
  subwindow->setMinimumWidth(610);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  QStringList labels;
  labels<<"Identificación"<<"Nombre"<<"Teléfono"<<"Dirección";
  uiproviders->table_prividers->setHorizontalHeaderLabels(labels);
  uiproviders->table_prividers->setColumnWidth(0, 90);
  uiproviders->table_prividers->setColumnWidth(1,200);
  uiproviders->table_prividers->setColumnWidth(2, 80);
  uiproviders->table_prividers->setColumnWidth(3,200);
  connect(uiproviders->button_save,SIGNAL(clicked()),this,SLOT(saveProvidersWindow()));
  connect(uiproviders->button_reload,SIGNAL(clicked()),this,SLOT(loadProvidersWindow()));
  loadProvidersWindow();
}

void MainWindow::openHomeHistory()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Historial por Casa") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uihomehistory = new Ui::HomeHistoryWindow;
  uihomehistory->setupUi(subwindow);
  subwindow->setWindowTitle("Historial por Casa");
  subwindow->setMinimumWidth(700);
  QStringList labels;
  uihomehistory->table_history->setHorizontalHeaderLabels(labels<<"Fecha"<<"Concepto"<<"Debito"<<"Crédito"<<"Saldo");
  uihomehistory->table_history->setColumnWidth(0, 90);
  uihomehistory->table_history->setColumnWidth(1,320);
  uihomehistory->table_history->setColumnWidth(2, 80);
  uihomehistory->table_history->setColumnWidth(3, 80);
  uihomehistory->table_history->setColumnWidth(4, 80);
  uihomehistory->table_history->setColumnHidden(5,true);
  uihomehistory->table_history->setColumnHidden(6,true);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  connect(uihomehistory->lineedit_home,SIGNAL(editingFinished()),this,SLOT(loadHistory()));
  connect(uihomehistory->button_print,SIGNAL(clicked()),this,SLOT(printHistory()));
  connect(uihomehistory->table_history,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(highlight(QModelIndex)));
  connect(uihomehistory->button_highlight,SIGNAL(clicked()),this,SLOT(highlight()));
}

void MainWindow::openSummaryDebts()
{
  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == "Resumen de deudas") {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uisummarydebts= new Ui::SummaryDebtsWindow;
  uisummarydebts->setupUi(subwindow);
  subwindow->setWindowTitle("Resumen de deudas");
  subwindow->setMinimumWidth(650);
  ui->mdiArea->addSubWindow(subwindow);
  QStringList labels;
  labels = labels <<"Casa";
  uisummarydebts->table_info->setHorizontalHeaderLabels(labels);

  QSqlQuery query;
  query.exec("SELECT number,name FROM accounts WHERE handler = 1320");
  int i=1;
  while (query.next()) {
    QTableWidgetItem* headeritem = new QTableWidgetItem(query.value(0).toString());
    headeritem->setToolTip(query.value(1).toString());
    uisummarydebts->table_info->setHorizontalHeaderItem(i,headeritem);
    i++;
  }
  uisummarydebts->table_info->setHorizontalHeaderItem(7,new QTableWidgetItem("Total"));

  uisummarydebts->table_info->setColumnWidth(0,60);
  uisummarydebts->table_info->setColumnWidth(1,80);
  uisummarydebts->table_info->setColumnWidth(2,80);
  uisummarydebts->table_info->setColumnWidth(3,80);
  uisummarydebts->table_info->setColumnWidth(4,80);
  uisummarydebts->table_info->setColumnWidth(5,80);
  uisummarydebts->table_info->setColumnWidth(6,80);
  uisummarydebts->table_info->setColumnWidth(7,80);
  connect(uisummarydebts->button_refresh,SIGNAL(clicked()),this,SLOT(loadSummaryDebts()));
  connect(uisummarydebts->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDateSummaryDebts()));
  connect(uisummarydebts->button_print,SIGNAL(clicked()),this,SLOT(printSummaryDebts()));
  uisummarydebts->dateedit_date->setDate(QDate::currentDate());
  loadSummaryDebts();
  showDateSummaryDebts();
  subwindow->show();
}

void MainWindow::openSummaryCollect()
{
    foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
      if (subwindow->windowTitle() == "Resumen de recaudo") {
        subwindow->activateWindow();
        return;
      }
    }

    QWidget* subwindow = new QWidget;
    uisummarycollect= new Ui::SummaryCollectWindow;
    uisummarycollect->setupUi(subwindow);
    subwindow->setWindowTitle("Resumen de recaudo");
    subwindow->setMinimumWidth(300);
    subwindow->setMinimumHeight(400);
    ui->mdiArea->addSubWindow(subwindow);
    QStringList labels;
    labels = labels <<"Año"<<"Mes"<<"Valor";
    uisummarycollect->table_info->setHorizontalHeaderLabels(labels);

    uisummarycollect->table_info->setColumnWidth(0,60);
    uisummarycollect->table_info->setColumnWidth(1,90);
    uisummarycollect->table_info->setColumnWidth(2,90);

    connect(uisummarycollect->button_refresh,SIGNAL(clicked()),this,SLOT(loadSummaryCollect()));

    loadSummaryCollect();
    subwindow->show();
}

void MainWindow::openBudgetExecution()
{
    foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
      if (subwindow->windowTitle() == "Ejecución Presupuestal") {
        subwindow->activateWindow();
        return;
      }
    }

    QWidget* subwindow = new QWidget;
    uibudgetexecution= new Ui::BudgetExecutionWindow;
    uibudgetexecution->setupUi(subwindow);
    subwindow->setWindowTitle("Ejecución Presupuestal");
    subwindow->setMinimumWidth(600);
    subwindow->setMinimumHeight(400);
    ui->mdiArea->addSubWindow(subwindow);
    subwindow->show();
    QStringList labels("Cuentas");
    labels = labels << "Presupuesto" << "Ene" << "Feb" << "Mar" << "Abr" << "May" << "Jun";
    labels = labels << "Jul" << "Ago" << "Sep" << "Oct" << "Nov" << "Dic" << "Ejecución" << "Saldo";
    uibudgetexecution->tree_puc->setHeaderLabels(labels);

    uibudgetexecution->tree_puc->setColumnWidth(0,300);
    for (int i=2; i<=13; i++)
    {
        uibudgetexecution->tree_puc->setColumnWidth(i,80);
    }
    uibudgetexecution->tree_puc->setColumnWidth( 1,90);
    uibudgetexecution->tree_puc->setColumnWidth(14,90);
    uibudgetexecution->tree_puc->setColumnWidth(15,90);
    uibudgetexecution->tree_puc->setItemDelegate(new MyItemDelegate());
    loadAccountsBudget(uibudgetexecution->tree_puc);
    loadAccountsBudgetTotals(uibudgetexecution->tree_puc);
    switchBudgetMounthHidde();
    uibudgetexecution->tree_puc->expandToDepth(0);
    connect(uibudgetexecution->checkBox_1,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_2,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_3,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_4,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_5,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_6,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_7,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_8,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_9,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_10,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_11,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->checkBox_12,SIGNAL(clicked()),this,SLOT(switchBudgetMounthHidde()));
    connect(uibudgetexecution->tree_puc,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(openBudgetDetail(QTreeWidgetItem*,int)));
    connect(uibudgetexecution->spinBox_year,SIGNAL(valueChanged(int)),this,SLOT(changeBudgetYear(int)));
}

void MainWindow::loadPaymentType()
{
  uipayments->combobox_type->addItem("");
  QSqlQuery query;
  query.exec(QString("SELECT number,name FROM accounts WHERE (number LIKE '5%' OR number LIKE '1524%') AND length(number)=6"));
  while (query.next())
  {
    uipayments->combobox_type->addItem(query.value(0).toString()+" "+query.value(1).toString(),query.value(0).toInt());
  }
}

void MainWindow::loadProviders()
{
  uipayments->combobox_provider->clear();
  uipayments->combobox_provider->addItem("");
  QSqlQuery query;
  query.exec("SELECT id,field1 FROM descriptors WHERE type=2 ORDER BY field1");
  while (query.next())
  {
    uipayments->combobox_provider->addItem(query.value(1).toString()+"  ["+query.value(0).toString()+"]",query.value(0).toString());
  }
}

void MainWindow::loadResources()
{
  QSqlQuery query;
  query.exec(QString("SELECT number,name FROM accounts WHERE handler IN (SELECT number FROM accounts WHERE handler = 11)"));
  while (query.next())
  {
    uipayments->combobox_from->addItem(query.value(0).toString()+" "+query.value(1).toString(),query.value(0).toInt());
  }
}

void MainWindow::savePayment()
{
  QSqlQuery query;
  int       number = 1;
  int       account0,account1;
  QString   date;
  int       value;
  QString   detail;
  QString   descriptor;
  int       voucher;

  if (QMessageBox(QMessageBox::Information,
                  "Preparado para guardar",
                  "Esta seguro que desea guardar este pago?",
                  QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) {
    return;
  }

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  account0    = uipayments->combobox_type->currentText().split(" ").at(0).toInt();
  account1    = uipayments->combobox_from->currentText().split(" ").at(0).toInt();
  detail      = uipayments->lineedit_detail->text();
  date        = uipayments->dateedit_date->date().toString("yyyy-MM-dd");
  value       = uipayments->lineedit_value->text().toInt();
  descriptor  = uipayments->combobox_provider->currentData().toString();
  voucher     = uipayments->lineedit_voucher->text().toInt();

  if (uipayments->combobox_type->currentText()=="") {
    QMessageBox::information(this,"Error","Debe seleccionar un tipo de Gasto");
    return;
  }
  if (uipayments->combobox_provider->currentText()=="") {
    QMessageBox::information(this,"Error","Debe seleccionar un Proveedor del servicio");
    return;
  }
  if (uipayments->lineedit_value->text()=="") {
    QMessageBox::information(this,"Error","Debe digitar un valor");
    return;
  }
  if (uipayments->lineedit_detail->text()=="") {
    QMessageBox::information(this,"Error","Debe escribir un detalle");
    return;
  }

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8,'1')").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,%8,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext1);

  QMessageBox::information(this,"Transacción procesada",QString("Asiento registrado [Número: %1]").arg(number));
  uipayments->combobox_type->setCurrentIndex(0);
  uipayments->combobox_provider->setCurrentIndex(0);
  uipayments->lineedit_value->setText("");
  uipayments->lineedit_detail->setText("");
}

void MainWindow::saveProvider()
{
  QString id      = uipayments->lineedit_id->text();
  QString name    = uipayments->lineedit_name->text();
  QString phone   = uipayments->lineedit_phone->text();
  QString address = uipayments->lineedit_address->text();

  if (QMessageBox(QMessageBox::Information,
                  "Preparado para guardar",
                  "Esta seguro que desea guardar este proveedor?",
                  QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) {
    return;
  }

  if (id == "") {
    QMessageBox::information(this,"Error!","No ha ingresado la identificación");
    return;
  }
  if (name == "") {
    QMessageBox::information(this,"Error!","No ha ingresado el nombre");
    return;
  }

  QString querytext = QString("INSERT INTO descriptors VALUES('%1','%2','%3','%4',2)").arg(id).arg(name).arg(phone).arg(address);

  QSqlQuery query;
  query.exec(querytext);

  loadProviders();
  QMessageBox::information(this,"Registro guardado",QString("Proveedor de servicio guardado "));
  uipayments->lineedit_id->setText("");
  uipayments->lineedit_name->setText("");
  uipayments->lineedit_phone->setText("");
  uipayments->lineedit_address->setText("");
}

void MainWindow::showDatePayment()
{
  uipayments->label_date->setText(uipayments->dateedit_date->date().toString("dd-MMM-yyyy"));
  uipayments->lineedit_value->setFocus();
}

void MainWindow::showDateBilling()
{
  uibilling->label_date->setText(uibilling->dateedit_date->date().toString("dd-MMM-yyyy"));
  uibilling->lineedit_value->setFocus();
}

void MainWindow::showDatePBilling()
{
  uibilling->label_p_date->setText(uibilling->dateedit_p_date->date().toString("dd-MMM-yyyy"));
}

void MainWindow::showDateCollect()
{
  uicollect->label_data->setText(uicollect->dateedit_date->date().toString("dd-MMM-yyyy"));
  uicollect->lineedit_voucher->setFocus();
}

void MainWindow::loadBillingType()
{
  QSqlQuery query;
  query.exec(QString("SELECT number,name FROM accounts WHERE number IN (132030,132040,132050)"));
  while (query.next())
  {
    uibilling->combobox_type->addItem(query.value(0).toString()+" "+query.value(1).toString(),query.value(0).toInt());
//    uibilling->combobox_type->addItem(query.value(1).toString()+query.value(0).toInt());
  }
  query.exec(QString("SELECT number,name FROM accounts WHERE handler=1320 AND number NOT IN (132030,132040,132050)"));
  while (query.next())
  {
    uibilling->combobox_p_type->addItem(query.value(0).toString()+" "+query.value(1).toString(),query.value(0).toInt());
//    uibilling->combobox_p_type->addItem(query.value(1).toString(),query.value(0).toInt());
  }
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1"));
  while(query.next()) {
    uibilling->combobox_p_home->addItem(query.value(0).toString(),query.value(0));
  }
}

void MainWindow::createGeneralBilling()
{
  QSqlQuery query;
  int       number = 1;
  int       value;
  int       voucher = 0;
  QString   handler;

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  value   = uibilling->lineedit_value->text().toInt();
  if (uibilling->checkbox_voucher->isChecked()) {
    voucher = uibilling->lineedit_voucher->text().toInt();
  }
  handler = uibilling->combobox_type->currentText().split(" ").at(0);

  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1"));
  QWidget* list = openBillingList();
  uibillinglist->label_number->setText(QString("%1").arg(number));
  uibillinglist->label_detail->setText(uibilling->lineedit_detail->text());
  uibillinglist->label_account0->setText(handler);
  uibillinglist->label_account1->setText(QString("%1").arg(423520*100 + handler.toInt()%100));
  uibillinglist->label_date->setText(uibilling->dateedit_date->date().toString("yyyy-MM-dd"));

  int i=0;
  while (query.next()) {
    uibillinglist->table_billing->setItem(i,0,new QTableWidgetItem(query.value(0).toString()));
    uibillinglist->table_billing->item(i,0)->setToolTip(query.value(1).toString());
    uibillinglist->table_billing->setItem(i,1,new QTableWidgetItem(QString("%L1").arg(value)));
    uibillinglist->table_billing->setItem(i,2,new QTableWidgetItem(QString("%1").arg(voucher)));
    uibillinglist->table_billing->setItem(i,10,new QTableWidgetItem("0"));
    uibillinglist->table_billing->setItem(i,11,new QTableWidgetItem("0"));

    if (uibilling->checkbox_voucher->isChecked()) { voucher++; }

    QString   querytext1= QString("SELECT e.number num,record rec,account,date,name,detail,value val, "
                                  "  (SELECT sum(value) "
                                  "   FROM entries e,relations r "
                                  "   WHERE r.compensation_number = e.number AND r.compensation_record = e.record "
                                  "   AND source_number=num AND source_record=rec) com "
                                  "FROM entries e, accounts a "
                                  "WHERE e.account = a.number "
                                  "AND descriptorid = '%1' AND type=1 AND account like '28%' "
                                  "ORDER BY date").arg(query.value(0).toString());

    QSqlQuery individualquery;
    individualquery.exec(querytext1);

    int newvalue = value;
    while(individualquery.next()) {
      if (individualquery.value(6).toInt()-individualquery.value(7).toInt()>0) {
        int balance = individualquery.value(6).toInt()-individualquery.value(7).toInt();
        int rebate = 0;
        newvalue = value - balance;
        if (newvalue <= 0 ) {
          newvalue = 0;
          rebate = value;
        }
        else {
          rebate = value-newvalue;
        }
        uibillinglist->table_billing->item(i,1)->setText(QString("%L1").arg(newvalue));
        uibillinglist->table_billing->item(i,10)->setText(QString("%L1").arg(balance));
        uibillinglist->table_billing->item(i,11)->setText(QString("%L1").arg(rebate));
        QString key = QString("%1 %2").arg(individualquery.value(0).toInt()).arg(individualquery.value(1).toInt());
        uibillinglist->table_billing->setItem(i,12,new QTableWidgetItem(key));
        break;
      }
    }

    QString   querytext = QString("SELECT e.number num,record rec,account,date,name,detail,value val, "
                                  "   (SELECT sum(value) "
                                  "   FROM entries e,relations r "
                                  "   WHERE r.compensation_number = e.number AND r.compensation_record = e.record "
                                  "   AND source_number=num AND source_record=rec) "
                                  "FROM entries e, accounts a "
                                  "WHERE e.account = a.number "
                                  "AND descriptorid = '%1' AND type=0 AND account like '13%' "
                                  "ORDER BY date").arg(query.value(0).toString());

    int totals[6];
    for (int t=0; t<6; t++) totals[t]= 0;
    int total = newvalue;

    individualquery.exec(querytext);
    while(individualquery.next()) {
      if (!individualquery.value(7).isNull() && individualquery.value(6).toInt()<=individualquery.value(7).toInt()) continue;
      int debt = individualquery.value(6).toInt();
      if (!individualquery.value(7).isNull()) debt -= individualquery.value(7).toInt();

      int t = (individualquery.value(2).toString().right(2).toInt()-30)/5;
      totals[t] += debt;
      total += debt;
    }
    for (int t=0; t<6; t++) {
      uibillinglist->table_billing->setItem(i,3+t,new QTableWidgetItem(QString("%L1").arg(totals[t])));
    }
    uibillinglist->table_billing->setItem(i,9,new QTableWidgetItem(QString("%L1").arg(total)));

    for (int c=0; c<11; c++) {
//      if (c!=2) {
//          uibillinglist->table_billing->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
//      }
      if (c>=3) {
        uibillinglist->table_billing->item(i,c)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
      }
    }

    i++;
  }
  QString comment1 = QString("Fecha de expedición %1.")
          .arg(uibilling->dateedit_date->date().toString("dd MM yy"));

  QString comment2 = QString("Pasados 10 días, debe cancelar $5.000 por cada mes de incumplimiento.");

  uibillinglist->lineedit_comment1->setText( comment1 );
  uibillinglist->lineedit_comment2->setText( comment2 );
  list->show();
}

void MainWindow::loadParms()
{
  QSqlQuery query;
  QString querytext = QString("SELECT value FROM entries "
                              "WHERE account = 132030 AND type=0 AND date >= '2014-01-01' "
                              "GROUP BY value");
  query.exec(querytext);
  uibilling->table_parms->setRowCount(0);
  int i=0;
  while (query.next()) {
    uibilling->table_parms->insertRow(uibilling->table_parms->rowCount());
    uibilling->table_parms->setItem(i,0,new QTableWidgetItem(QString("%L1").arg(query.value(0).toInt())));
    uibilling->table_parms->setItem(i,1,new QTableWidgetItem(QString("%L1").arg(0)));
    uibilling->table_parms->item(i,0)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uibilling->table_parms->item(i,1)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uibilling->table_parms->item(i,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    i++;
  }
  uibilling->dateedit_since->setFocus();
}

int penalty(QTableWidget* table,QString value) {
  for(int i=0; i<table->rowCount(); i++) {
    if (table->item(i,0)->text() == value) {
      return table->item(i,1)->text().toInt();
    }
  }
  return 0;
}

void MainWindow::loadPenalties()
{
  uibilling->table_penalties->setRowCount(0);

  QSqlQuery query;
  QString   querytext = QString("SELECT e.number num,record rec,account,date,name,detail,value,descriptorid, "
                                "   (SELECT sum(value) "
                                "   FROM entries e, relations r "
                                "   WHERE e.number = r.compensation_number AND e.record = r.compensation_record "
                                "   AND r.source_number=num AND r.source_record=rec) com "
                                "FROM entries e,accounts a "
                                "WHERE account = 132030 AND type=0 AND date >= '%1' AND special=0 "
                                "AND e.account=a.number "
                                "ORDER BY descriptorid,date").arg(uibilling->dateedit_since->date().toString("yyyy-MM-dd"));
  query.exec(querytext);
  int i=0;
  while (query.next()) {
    if (!query.value(7).isNull() && query.value(6).toInt()<=query.value(8).toInt()) continue;

    QSqlQuery fastquery;
    QString fastquerytext =   QString("SELECT count(*) "
                                      "FROM entries "
                                      "WHERE account = 132035 AND detail like '%1%' "
                                      "AND descriptorid = '%2'").arg(query.value(5).toString()).arg(query.value(7).toString());
    fastquery.exec(fastquerytext);
    fastquery.next();
    int consecutive = fastquery.value(0).toInt()+1;

    uibilling->table_penalties->insertRow(uibilling->table_penalties->rowCount());
    uibilling->table_penalties->setItem(i,0,new QTableWidgetItem(query.value(7).toString()));
    uibilling->table_penalties->setItem(i,1,new QTableWidgetItem(query.value(5).toString()+QString(" [%1]").arg(consecutive)));
    int debt = query.value(6).toInt();
    if (!query.value(7).isNull()) debt -= query.value(8).toInt();
    uibilling->table_penalties->setItem(i,2,new QTableWidgetItem(QString("%L1").arg(debt)));
    uibilling->table_penalties->setItem(i,3,new QTableWidgetItem(QString("%1").arg(penalty(uibilling->table_parms,uibilling->table_penalties->item(i,2)->text()))));
    for (int c=0; c<3; c++) {
      uibilling->table_penalties->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }
    i++;
  }
}

void MainWindow::savePenalties()
{
  int     number    = 1;
  QString detail;
  int     account0  = 132035;
  int     account1  = 42352035;
  QString date      = uibilling->dateedit_at->date().toString("yyyy-MM-dd");
  QString home;

  if (QMessageBox(QMessageBox::Information,
                  "Preparado para guardar",
                  "Esta seguro que desea guardar estas multas?",
                  QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) {
    return;
  }

  int     value;
  QSqlQuery query;
  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  for (int i=0; i<uibilling->table_penalties->rowCount(); i++) {
    home      = uibilling->table_penalties->item(i,0)->text();
    value     = uibilling->table_penalties->item(i,3)->text().toInt();
    detail    = uibilling->table_penalties->item(i,1)->text();

    QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,null,'1')").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(home);
    query.exec(querytext0);

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,null,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(home);
    query.exec(querytext1);

    number++;
  }
  QMessageBox::information(this,"Registros guardados",QString("%1 asientos contables procesados").arg(uibilling->table_penalties->rowCount()));
  uibilling->table_penalties->setRowCount(0);
}

void MainWindow::showDatePenaltiesSince()
{
  uibilling->label_since->setText(uibilling->dateedit_since->date().toString("dd-MMM-yyyy"));
  uibilling->dateedit_at->setFocus();
}

void MainWindow::showDatePenaltiesAt()
{
  uibilling->label_at->setText(uibilling->dateedit_at->date().toString("dd-MMM-yyyy"));
  uibilling->table_parms->setFocus();
  uibilling->table_parms->setCurrentCell(0,1);
}

void MainWindow::printBilling(QModelIndex mi)
{
  if (mi.column()==0) {
    int row = mi.row();
    QString home    = uibillinglist->table_billing->item(row,0)->text();
    QString owner   = uibillinglist->table_billing->item(row,0)->toolTip();
    QString current = uibillinglist->table_billing->item(row,1)->text();
    QString number  = uibillinglist->table_billing->item(row,2)->text();
    QString t30     = uibillinglist->table_billing->item(row,3)->text();
    QString t35     = uibillinglist->table_billing->item(row,4)->text();
    QString t40     = uibillinglist->table_billing->item(row,5)->text();
    QString t45     = uibillinglist->table_billing->item(row,6)->text();
    QString t50     = uibillinglist->table_billing->item(row,7)->text();
    QString t55     = uibillinglist->table_billing->item(row,8)->text();
    QString t       = uibillinglist->table_billing->item(row,9)->text();
    QString n45     = uibillinglist->table_billing->horizontalHeaderItem(6)->toolTip();
    QString n50     = uibillinglist->table_billing->horizontalHeaderItem(7)->toolTip();
    QString n55     = uibillinglist->table_billing->horizontalHeaderItem(8)->toolTip();

    QPrinter printer(QPrinter::HighResolution);
    printer.setCreator("JP-Billing");
    printer.setDocName(QString("Facturación %1").arg(number));

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
      QPainter painter(&printer);

      double y = 33.0;

      painter.drawText(ch( 20),cv(y),ch(100),cv(5),Qt::AlignLeft,owner); y += 4.9;
      painter.drawText(ch( 20),cv(y),ch(100),cv(5),Qt::AlignLeft,home);  y += 9.7;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,QString("Expensa ordinaria %1").arg(uibillinglist->label_detail->text()));
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,current); y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,"Multas por mora");
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t35);     y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,"Cuotas anteriores");
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t30);     y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,"Cuota extraordinaria");
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t40);     y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,n45);
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t45);     y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,n50);
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t50);     y += 4.3;

      painter.drawText(ch( 70),cv(y),ch( 80),cv(4),Qt::AlignLeft,n55);
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t55);     y += 4.3;

      y += 4.3;

      QFont font = painter.font();
      font.setBold(true);
      font.setPointSize(font.pointSize()+1);
      painter.setFont(font);

      painter.drawText(ch( 70),cv(y),ch( 30),cv(4),Qt::AlignLeft,"TOTAL A PAGAR");
      painter.drawText(ch(128),cv(y),ch( 30),cv(4),Qt::AlignRight,t);

      y += 4.3;
      y += 4.8;

      font.setBold(false);
      font.setPointSize(font.pointSize()-2);
      painter.setFont(font);

      painter.drawText(ch( 10),cv(y),ch(120),cv(4),Qt::AlignLeft,QString(uibillinglist->lineedit_comment1->text()));
      y += 4.3;
      painter.drawText(ch( 10),cv(y),ch(120),cv(4),Qt::AlignLeft,QString(uibillinglist->lineedit_comment2->text()));
    }
  }
}

void MainWindow::saveGeneralBilling()
{
  int     number    = uibillinglist->label_number->text().toInt();
  QString detail    = uibillinglist->label_detail->text();
  int     account0  = uibillinglist->label_account0->text().toInt();
  int     account1  = uibillinglist->label_account1->text().toInt();
  QString date      = uibillinglist->label_date->text();
  QString name;
  int     value;
  int     voucher;
  QSqlQuery query;

  if (QMessageBox(QMessageBox::Information,
                  "Preparado para guardar",
                  "Esta seguro que desea guardar esta facturación?",
                  QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) {
    return;
  }

  for (int i=0; i<49; i++) {
    name      = uibillinglist->table_billing->item(i,0)->text();
    value     = uibillinglist->table_billing->item(i,1)->text().remove(QRegExp("[.,]")).toInt();
    value    += uibillinglist->table_billing->item(i,11)->text().remove(QRegExp("[.,]")).toInt();
    voucher   = uibillinglist->table_billing->item(i,2)->text().toInt();
    detail    = uibillinglist->label_detail->text();

    QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',0,%8,'1')").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(name).arg(voucher);
    query.exec(querytext0);

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',0,%8,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(name).arg(voucher);
    query.exec(querytext1);

    number++;

    int rebate  = uibillinglist->table_billing->item(i,11)->text().remove(QRegExp("[.,]")).toInt();
    if (rebate > 0) {
      int numberentry = uibillinglist->table_billing->item(i,12)->text().split(" ").at(0).toInt();
      int recordentry = uibillinglist->table_billing->item(i,12)->text().split(" ").at(1).toInt();

      querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',0,null,'0')").arg(number).arg(280595).arg(date).arg(DEBIT).arg(rebate).arg(detail).arg(name);
      query.exec(querytext0);

      querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',0,null,'0')").arg(number).arg(account0).arg(date).arg(CREDIT).arg(rebate).arg(detail).arg(name);
      query.exec(querytext1);

      querytext0 = QString("INSERT INTO relations VALUES(%1,%2,%3,%4)").arg(numberentry).arg(recordentry).arg(number).arg(1);
      query.exec(querytext0);

      querytext1 = QString("INSERT INTO relations VALUES(%1,%2,%3,%4)").arg(number-1).arg(1).arg(number).arg(2);
      query.exec(querytext1);
    }

    number++;

  }
  uibillinglist->button_save->setEnabled(false);
  QMessageBox::information(this,"Registros guardados","49 asientos contables procesados");
}

void MainWindow::saveParticularBilling()
{
  QSqlQuery query;
  int       number = 1;
  int       account0,account1;
  QString   date;
  int       value;
  QString   detail;
  QString   descriptor;
  int       voucher;

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

//  account0    = uibilling->combobox_p_type->currentData().toInt();
  account0    = uibilling->combobox_p_type->currentText().split(" ").at(0).toInt();
  account1    = 423520*100 + account0%100;
  detail      = uibilling->lineedit_p_detail->text();
  date        = uibilling->dateedit_p_date->date().toString("yyyy-MM-dd");
  value       = uibilling->lineedit_p_value->text().toInt();
  descriptor  = uibilling->combobox_p_home->currentText();
  voucher     = uibilling->lineedit_p_voucher->text().toInt();

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8,'1')").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,%8,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext1);

  QMessageBox::information(this,"Transacción procesada",QString("Asiento registrado [Número: %1]").arg(number));
}

void MainWindow::loadOwners()
{
  QSqlQuery query;
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1 AND id = 'Casa%1'").arg(uicollect->lineedit_home->text()));
  if (query.next()) {
    uicollect->label_home->setText(query.value(1).toString());
    uicollect->dateedit_date->setFocus();
  }
  else {
    uicollect->label_data->setText("");
    uicollect->lineedit_home->selectAll();
  }
}

void MainWindow::loadAccounts(QTreeWidget* widget, QTreeWidgetItem* item, int handler)
{
  QSqlQuery query;
  if (!item) {
    query.exec("SELECT number, name FROM accounts WHERE handler IS NULL");
    while (query.next()) {
      QStringList labels = QStringList(query.value(0).toString()+" - "+query.value(1).toString());
      QTreeWidgetItem* it = new QTreeWidgetItem((QTreeWidget*)0, labels<<query.value(0).toString());
      it->setTextColor(0,Qt::darkRed);
      widget->addTopLevelItem(it);
      loadAccounts(widget,it,query.value(0).toInt());
    }
  }
  else {
    query.exec(QString("SELECT number, name FROM accounts WHERE handler=%1").arg(handler));
    while (query.next()) {
      QStringList labels = QStringList(query.value(0).toString()+" - "+query.value(1).toString());
      QTreeWidgetItem* it = new QTreeWidgetItem((QTreeWidget*)0, labels<<query.value(0).toString());
      switch (QString("%1").arg(handler).length()) {
      case 1: it->setTextColor(0,Qt::darkBlue); break;
      case 2: it->setTextColor(0,Qt::darkGreen); break;
      }
      item->addChild(it);
      loadAccounts(widget,it,query.value(0).toInt());
    }
  }
}

void MainWindow::loadHomes()
{
  QSqlQuery query;
  query.exec(QString("SELECT id,field1,field2 FROM descriptors WHERE type=1"));
  int i = 0;
  while (query.next()) {
    QTableWidgetItem *item = new QTableWidgetItem(query.value(0).toString());
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    uihomes->table_homes->setItem(i,0,item);
    uihomes->table_homes->setItem(i,1,new QTableWidgetItem(query.value(1).toString()));
    uihomes->table_homes->setItem(i,2,new QTableWidgetItem(query.value(2).toString()));
    i++;
  }
}

void MainWindow::saveHomes()
{
  QSqlQuery query;
  for (int i=0; i<49; i++) {
    QString id    = uihomes->table_homes->item(i,0)->text();
    QString owner = uihomes->table_homes->item(i,1)->text();
    QString phone = uihomes->table_homes->item(i,2)->text();

    QString querytext = QString("UPDATE descriptors SET field1 = '%1', field2 = '%2' WHERE id = '%3'").arg(owner).arg(phone).arg(id);
    query.exec(querytext);
  }
  QMessageBox::information(this,"Registros guardados","Las modificaciones fueron guardadas");
}

void MainWindow::loadProvidersWindow()
{
  QSqlQuery query;
  query.exec(QString("SELECT id,field1,field2,field3 FROM descriptors WHERE type=2"));
  int i = 0;
  while (query.next()) {
    uiproviders->table_prividers->insertRow(uiproviders->table_prividers->rowCount());
    QTableWidgetItem *item = new QTableWidgetItem(query.value(0).toString());
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    uiproviders->table_prividers->setItem(i,0,item);
    uiproviders->table_prividers->setItem(i,1,new QTableWidgetItem(query.value(1).toString()));
    uiproviders->table_prividers->setItem(i,2,new QTableWidgetItem(query.value(2).toString()));
    uiproviders->table_prividers->setItem(i,3,new QTableWidgetItem(query.value(3).toString()));
    i++;
  }
}

void MainWindow::saveProvidersWindow()
{
  QSqlQuery query;
  for (int i=0; i<uiproviders->table_prividers->rowCount(); i++) {
    QString id      = uiproviders->table_prividers->item(i,0)->text();
    QString name    = uiproviders->table_prividers->item(i,1)->text();
    QString phone   = uiproviders->table_prividers->item(i,2)->text();
    QString address = uiproviders->table_prividers->item(i,3)->text();

    QString querytext = QString("UPDATE descriptors SET field1 = '%1', field2 = '%2', field3 = '%3' WHERE id = '%4'").arg(name).arg(phone).arg(address).arg(id);
    query.exec(querytext);
  }
  QMessageBox::information(this,"Registros guardados","Las modificaciones fueron guardadas");
}

void MainWindow::loadDebt()
{

  QSqlQuery query;
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1 AND id = 'Casa%1'").arg(uicollect->lineedit_home->text()));
  if (query.next()) {
    uicollect->label_home->setText(query.value(0).toString());
    uicollect->label_owner->setText(query.value(1).toString());
    uicollect->dateedit_date->setFocus();
  }
  else {
    uicollect->label_home->setText("");
    uicollect->label_owner->setText("");
    uicollect->label_data->setText("");
    uicollect->table_detail->setRowCount(0);
    uicollect->label_debt->setText("0");
    uicollect->label_total->setText("0");
    uicollect->label_balance->setText("0");
    uicollect->lineedit_home->selectAll();
    return;
  }

  uicollect->table_detail->setRowCount(0);

  QString id = uicollect->label_home->text();

  QString   querytext = QString("SELECT e.number num,record rec,account,date,name,detail,value, "
                                "   (SELECT sum(value) "
                                "   FROM entries e, relations r "
                                "   WHERE e.number = r.compensation_number AND e.record = r.compensation_record "
                                "   AND r.source_number=num AND r.source_record=rec) com "
                                "FROM entries e, accounts a "
                                "WHERE e.account = a.number "
                                "AND account LIKE '1320%' AND descriptorid='%1' AND type=0 "
                                "ORDER BY date;).arg(id) ").arg(id);

  query.exec(querytext);
  int i=0,total=0;
  while (query.next()) {
    if (!query.value(7).isNull() && query.value(6).toInt()<=query.value(7).toInt()) continue;
    uicollect->table_detail->insertRow(uicollect->table_detail->rowCount());
    uicollect->table_detail->setItem(i,0,new QTableWidgetItem(query.value(4).toString()));
    uicollect->table_detail->setItem(i,1,new QTableWidgetItem(query.value(5).toString()));

    int debt = query.value(6).toInt();
    if (!query.value(7).isNull()) debt -= query.value(7).toInt();

    uicollect->table_detail->setItem(i,2,new QTableWidgetItem(QString("%L1").arg(debt)));
    uicollect->table_detail->setItem(i,3,new QTableWidgetItem("0"));
    uicollect->table_detail->setItem(i,4,new QTableWidgetItem(query.value(0).toString()));
    uicollect->table_detail->setItem(i,5,new QTableWidgetItem(query.value(2).toString()));
    uicollect->table_detail->setItem(i,6,new QTableWidgetItem(query.value(1).toString()));
    for (int c=0; c<3; c++) {
      uicollect->table_detail->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      if (query.value(2).toInt()==132030) {
        uicollect->table_detail->item(i,c)->setForeground(Qt::blue);
      }
    }
    i++;
    total += debt; //query.value(6).toInt();
  }
  uicollect->table_detail->insertRow(uicollect->table_detail->rowCount());
  uicollect->table_detail->setItem(i,0,new QTableWidgetItem("Anticipo"));
  uicollect->table_detail->setItem(i,1,new QTableWidgetItem("?"));
  uicollect->table_detail->setItem(i,2,new QTableWidgetItem("0"));
  uicollect->table_detail->setItem(i,3,new QTableWidgetItem("0"));
  uicollect->table_detail->setItem(i,5,new QTableWidgetItem("280595"));

  uicollect->table_detail->item(i,0)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
  uicollect->table_detail->item(i,2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

  uicollect->label_debt->setText(QString("%L1").arg(total));
  uicollect->label_debt->setToolTip(QString("%1").arg(total));
  uicollect->label_balance->setText(QString("%L1").arg(total));
  uicollect->table_detail->setCurrentCell(0,3);
}

void MainWindow::saveCollect()
{
  QSqlQuery query;
  int       number = 1;
  int       account0,account1;
  QString   date;
  int       value;
  int       joinentrynumber;
  int       joinentryrecord;
  QString   detail;
  QString   descriptor;
  QString   voucher;

  if (QMessageBox(QMessageBox::Information,
                  "Preparado para guardar",
                  "Esta seguro que desea guardar este recaudo?",
                  QMessageBox::Yes|QMessageBox::No).exec() == QMessageBox::No) {
    return;
  }

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  account0    = 110505; // General cashing
  date        = uicollect->dateedit_date->date().toString("yyyy-MM-dd");
  voucher     = uicollect->lineedit_voucher->text();
  value       = uicollect->label_total->text().remove(QRegExp("[.,]")).toInt();
  detail      = QString("Recaudo");
  descriptor  = uicollect->label_home->text();

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8,'1')").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  int record = 2;
  for (int i=0; i<uicollect->table_detail->rowCount()-1; i++) {
    value       = uicollect->table_detail->item(i,3)->text().toInt();
    if (value==0) continue;
    account1    = uicollect->table_detail->item(i,5)->text().toInt();
    joinentrynumber = uicollect->table_detail->item(i,4)->text().toInt();
    joinentryrecord = uicollect->table_detail->item(i,6)->text().toInt();
    detail      = uicollect->table_detail->item(i,1)->text();

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,%9,%2,'%3',%4,%5,'%6','%7',%8,%10,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(0).arg(record).arg(voucher);
    query.exec(querytext1);

    QString querytext2 = QString("INSERT INTO relations VALUES(%1,%2,%3,%4)").arg(joinentrynumber).arg(joinentryrecord).arg(number).arg(record);
    query.exec(querytext2);
    record ++;
  }

  int i = uicollect->table_detail->rowCount()-1;
  if (uicollect->table_detail->item(i,3)->text()!="0") {
      value       = uicollect->table_detail->item(i,3)->text().toInt();
      account1    = uicollect->table_detail->item(i,5)->text().toInt();
      detail      = uicollect->table_detail->item(i,1)->text();

      QString querytext1 = QString("INSERT INTO entries VALUES(%1,%8,%2,'%3',%4,%5,'%6','%7',null,%9,'1')").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(record).arg(voucher);
      query.exec(querytext1);
  }

  QMessageBox::information(this,"Transacción procesada",QString("Asiento registrado [Número: %1]").arg(number));
  uicollect->lineedit_home->setText("");
  uicollect->label_home->setText("");
  uicollect->label_owner->setText("");
  uicollect->lineedit_voucher->setText("");
  uicollect->table_detail->setRowCount(0);
  uicollect->lineedit_home->setFocus();
  uicollect->label_debt->setText("0");
  uicollect->label_total->setText("0");
  uicollect->label_balance->setText("0");
}

void MainWindow::resumeCollect(int row,int col)
{
  if (col==3) {
    int row2 = uicollect->table_detail->item(row,2)->text().remove(QRegExp("[.,]")).toInt();
    int row3 = uicollect->table_detail->item(row,3)->text().toInt();
    if (row < uicollect->table_detail->rowCount()-1 && row3 > row2) {
      QMessageBox(QMessageBox::Information,
                  "Atención",
                  "No puede ingresar un pago mayor a lo adeudado.",
                  QMessageBox::Ok).exec();
      uicollect->table_detail->item(row,3)->setText("0");
      uicollect->table_detail->setFocus();
    }
    int total=0;
    for (int i=0; i<uicollect->table_detail->rowCount(); i++) {
      total += uicollect->table_detail->item(i,3)->text().toInt();
    }
    uicollect->label_total->setText(QString("%L1").arg(total));
    uicollect->label_balance->setText(QString("%L1").arg(uicollect->label_debt->toolTip().toInt()-total));

  }
}

void MainWindow::loadHistory()
{
  QSqlQuery query;
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1 AND id = 'Casa%1'").arg(uihomehistory->lineedit_home->text()));
  if (query.next()) {
    uihomehistory->label_home->setText(query.value(0).toString());
    uihomehistory->label_owner->setText(query.value(1).toString());
    uihomehistory->button_print->setEnabled(true);
    uihomehistory->lineedit_home->selectAll();
  }
  else {
    uihomehistory->table_history->setRowCount(0);
    uihomehistory->button_print->setText("Imprimir");
    uihomehistory->button_print->setEnabled(false);
    uihomehistory->lineedit_home->selectAll();
    return;
  }

  QString id = uihomehistory->label_home->text();
  query.exec(QString("SELECT field1 FROM descriptors WHERE id='%1'").arg(id));
  query.next();
  uihomehistory->label_owner->setText(query.value(0).toString());
  query.exec(QString("SELECT date,name,type+0,value,detail,e.number num,e.record rec,voucher,account,concat(e.number,' ',record), "
                     "   (SELECT concat(source_number,' ',source_record) "
                     "   FROM relations "
                     "   WHERE compensation_number=num AND compensation_record=rec) "
                     "FROM entries e,accounts a "
                     "WHERE descriptorid='%1' AND state='1' "
                     "AND (account LIKE '1320%' OR account LIKE '2805%') "
                     "AND a.number=e.account "
                     "ORDER BY date,a.number,e.number").arg(id));

  uihomehistory->table_history->setRowCount(0);

  int i=0;
  int balance = 0;
  while (query.next()) {
    uihomehistory->table_history->insertRow(uihomehistory->table_history->rowCount());
    QString date    = query.value(0).toDate().toString("dd-MMM-yyyy");
    QString account = query.value(1).toString();
    int     type    = query.value(2).toInt();
    int     value   = query.value(3).toInt();
    QString detail  = query.value(4).toString();
    type==DEBIT?balance+=value:balance-=value;

    uihomehistory->table_history->setItem(i,0,new QTableWidgetItem(date));
    QString description = account+": "+detail;
    if (detail.left(7) != "Condona") {
      if (type==DEBIT) {
        description = "CxC " + account+" "+detail;
      }
      else {
        description = "Pago " + account+" "+detail;
      }
    }
    uihomehistory->table_history->setItem(i,1,new QTableWidgetItem(description));
    uihomehistory->table_history->setItem(i,2+type,new QTableWidgetItem(QString("%L1").arg(value)));
    uihomehistory->table_history->setItem(i,3-type,new QTableWidgetItem(""));
    uihomehistory->table_history->setItem(i,4,new QTableWidgetItem(QString("%L1").arg(balance)));
    uihomehistory->table_history->item(i,2)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uihomehistory->table_history->item(i,3)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uihomehistory->table_history->item(i,4)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uihomehistory->table_history->item(i,1)->setToolTip(QString("Comprobante %1").arg(query.value(7).toString()));
    uihomehistory->table_history->setItem(i,5,new QTableWidgetItem(query.value(9).toString()));
    uihomehistory->table_history->setItem(i,6,new QTableWidgetItem(query.value(10).toString()));

    for (int c=0; c<5; c++) {
      uihomehistory->table_history->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      if (query.value(8).toInt()==132030 && query.value(2).toInt()==0) {
        uihomehistory->table_history->item(i,c)->setForeground( Qt::blue );
      }
    }

    i++;
  }

  int pagecount = uihomehistory->table_history->rowCount() / 44;
  if (uihomehistory->table_history->rowCount()%44>0) pagecount ++;

  uihomehistory->button_print->setText(QString("Imprimir %1 pagina%2").arg(pagecount).arg(pagecount==1?"":"s"));
}

void MainWindow::printHistory()
{
  QPrinter printer(QPrinter::HighResolution);
  printer.setCreator("JP-Property");
  printer.setDocName("Historial");

  QPrintDialog printDialog(&printer, this);
  if (printDialog.exec() == QDialog::Accepted) {
    QPainter painter(&printer);
    QFont font = painter.font();
    font.setPointSize(font.pointSize()-2);
    painter.setFont(font);

    int page = printDialog.fromPage();
    int to = printDialog.toPage();

    int y = cv(22);
    int p = 0;
    int min = 0;
    int max = uihomehistory->table_history->rowCount();
    if (page>0) {
      min = (page-1)*44;
      max = max<(to*44)?max:(to*44);
    }
    else {
      page=1;
    }
    for (int i=min; i<max; i++) {
      if (p==44 || p==0) {
        if (p>0) {
          printer.newPage();
          page++;
        }
        p = 0;
        y = cv(22);

        font.setPointSize(font.pointSize()+6);
        painter.setFont(font);
        painter.drawText(ch(60),cv(11),ch(80),cv(6),Qt::AlignCenter,uihomehistory->label_home->text()+" "+uihomehistory->label_owner->text());
        font.setPointSize(font.pointSize()-6);
        painter.setFont(font);

        painter.drawText(ch(170),cv(11),ch(15),cv(6),Qt::AlignRight,QString("Pagina %1").arg(page));
        painter.drawText(ch(9),y,ch(20),cv(6),Qt::AlignLeft,"FECHA");
        painter.drawText(ch(31),y,ch(98),cv(6),Qt::AlignLeft,"CONCEPTO");
        painter.drawText(ch(130),y,ch(15),cv(6),Qt::AlignRight,"DEBE");
        painter.drawText(ch(150),y,ch(15),cv(6),Qt::AlignRight,"HABER");
        painter.drawText(ch(170),y,ch(15),cv(6),Qt::AlignRight,"SALDO");
      }
      int y = cv( (30+p*5) );
      painter.drawText(ch(9),y,ch(20),cv(6),Qt::AlignLeft,uihomehistory->table_history->item(i,0)->text());

      QString detail = uihomehistory->table_history->item(i,1)->text();
      if (uihomehistory->table_history->item(i,1)->toolTip().split(" ").at(1)!="0") {
        detail += " C."+uihomehistory->table_history->item(i,1)->toolTip().split(" ").at(1);
      }

      painter.drawText(ch(31),y,ch(98),cv(6),Qt::AlignLeft,detail);
      painter.drawText(ch(130),y,ch(15),cv(6),Qt::AlignRight,uihomehistory->table_history->item(i,2)->text());
      painter.drawText(ch(150),y,ch(15),cv(6),Qt::AlignRight,uihomehistory->table_history->item(i,3)->text());
      painter.drawText(ch(170),y,ch(15),cv(6),Qt::AlignRight,uihomehistory->table_history->item(i,4)->text());
      p++;
    }
  }
}

void MainWindow::highlight(QModelIndex mi)
{
  int number    = uihomehistory->table_history->item(mi.row(),5)->text().toInt();
  int joinentry = uihomehistory->table_history->item(mi.row(),6)->text().toInt();

  for (int i=0; i<uihomehistory->table_history->rowCount(); i++) {
    uihomehistory->table_history->item(i,2)->setBackgroundColor(Qt::white);
    uihomehistory->table_history->item(i,3)->setBackgroundColor(Qt::white);
  }

  if (mi.data().toString()=="" || mi.column()>3 || mi.column()<2) {
    return;
  }

  if (mi.column()==3) {
    for (int i=0; i<uihomehistory->table_history->rowCount(); i++) {
      if (uihomehistory->table_history->item(i,5)->text().toInt() == joinentry) {
        uihomehistory->table_history->item(i,2)->setBackgroundColor(Qt::yellow);
      }
      else {
        uihomehistory->table_history->item(i,2)->setBackgroundColor(Qt::white);
      }
      uihomehistory->table_history->item(i,3)->setBackgroundColor(Qt::white);
    }
    return;
  }

  if (mi.column()==2) {
    for (int i=0; i<uihomehistory->table_history->rowCount(); i++) {
      if (uihomehistory->table_history->item(i,6)->text().toInt() == number) {
        uihomehistory->table_history->item(i,3)->setBackgroundColor(Qt::cyan);
      }
      else {
        uihomehistory->table_history->item(i,3)->setBackgroundColor(Qt::white);
      }
      uihomehistory->table_history->item(i,2)->setBackgroundColor(Qt::white);
    }
  }
}

void MainWindow::highlight()
{
  for (int i=0; i<uihomehistory->table_history->rowCount(); i++) {
    if (uihomehistory->table_history->item(i,2)->text()!="" && uihomehistory->table_history->item(i,6)->text()!="") {
      uihomehistory->table_history->item(i,2)->setBackgroundColor(220<<16|220<<8|220);
    }
    if (uihomehistory->table_history->item(i,3)->text()!="" && uihomehistory->table_history->item(i,6)->text()!="") {
      uihomehistory->table_history->item(i,3)->setBackgroundColor(220<<16|220<<8|220);
    }
  }
}

void MainWindow::openAccountDetail()
{
  if (!uipuc->tree_puc->currentItem()) return;
  int account =  uipuc->tree_puc->currentItem()->text(1).toInt();
  QString name = uipuc->tree_puc->currentItem()->text(0);

  foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
    if (subwindow->windowTitle() == QString("Detalle de la cuenta %1").arg(name)) {
      subwindow->activateWindow();
      return;
    }
  }

  QWidget* subwindow = new QWidget;
  uiaccountdetail = new Ui::AccountDetail;
  uiaccountdetail->setupUi(subwindow);
  subwindow->setWindowTitle(QString("Detalle de la cuenta %1").arg(name));
  subwindow->setMinimumWidth(620);
  QStringList labels;
  uiaccountdetail->table_detail->setHorizontalHeaderLabels(labels<<"Fecha"<<"Concepto"<<"Debito"<<"Crédito"<<"Saldo");
  uiaccountdetail->table_detail->setColumnWidth(0, 90);
  uiaccountdetail->table_detail->setColumnWidth(1,250);
  uiaccountdetail->table_detail->setColumnWidth(2, 80);
  uiaccountdetail->table_detail->setColumnWidth(3, 80);
  uiaccountdetail->table_detail->setColumnWidth(4, 80);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  loadAccountDetail(account);
}

/*
 * 1,5,6 DEBIT
 * 2,3,4 CREDIT
 * */

void MainWindow::loadAccountDetail(int account, int year, int month)
{
  QSqlQuery query;
  if (year==0) {
      query.exec(QString("SELECT date,name,e.type+0,value,detail,e.number,d.id,voucher,field1 "
                         "FROM accounts a,entries e "
                         "LEFT JOIN descriptors d ON e.descriptorid = d.id "
                         "WHERE a.number=e.account "
                         "AND account LIKE '%1%' "
                         "ORDER BY date").arg(account));
  }
  else if (month==13){
      query.exec(QString("SELECT date,name,e.type+0,value,detail,e.number,d.id,voucher,field1 "
                         "FROM accounts a,entries e "
                         "LEFT JOIN descriptors d ON e.descriptorid = d.id "
                         "WHERE a.number=e.account "
                         "AND account LIKE '%1%' "
                         "AND year(date) = %2 "
                         "ORDER BY date").arg(account).arg(year));
  }
  else {
      query.exec(QString("SELECT date,name,e.type+0,value,detail,e.number,d.id,voucher,field1 "
                         "FROM accounts a,entries e "
                         "LEFT JOIN descriptors d ON e.descriptorid = d.id "
                         "WHERE a.number=e.account "
                         "AND account LIKE '%1%' "
                         "AND year(date) = %2 "
                         "AND month(date) = %3 "
                         "ORDER BY date").arg(account).arg(year).arg(month));
  }

  uiaccountdetail->table_detail->setRowCount(0);

  int i=0;
  int balance = 0;
  int digit = QString("%1").arg(account).left(1).toInt();
  int sign = 1;
  if (digit==2 || digit==3 || digit ==4) {
      sign = -1;
  }

  while (query.next()) {
    uiaccountdetail->table_detail->insertRow(uiaccountdetail->table_detail->rowCount());
    QString date        = query.value(0).toDate().toString("dd-MMM-yyyy");
    int     type        = query.value(2).toBool();
    int     value       = query.value(3).toInt();
    QString detail      = query.value(4).toString();
    QString descriptor  = query.value(6).toString();
    int     voucher     = query.value(7).toInt();
    QString name        = query.value(8).toString();

    type==DEBIT?balance+=value:balance-=value;

    uiaccountdetail->table_detail->setItem(i,0,new QTableWidgetItem(date));
    uiaccountdetail->table_detail->setItem(i,1,new QTableWidgetItem(descriptor+": "+detail));
    uiaccountdetail->table_detail->setItem(i,2+type,new QTableWidgetItem(QString("%L1").arg(value)));
    uiaccountdetail->table_detail->setItem(i,3-type,new QTableWidgetItem(""));
    uiaccountdetail->table_detail->setItem(i,4,new QTableWidgetItem(QString("%L1").arg(balance*sign)));
    uiaccountdetail->table_detail->item(i,2)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uiaccountdetail->table_detail->item(i,3)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uiaccountdetail->table_detail->item(i,4)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);

    uiaccountdetail->table_detail->item(i,1)->setToolTip(QString("%1 [Comprobante %2]").arg(name).arg(voucher));

    for (int c=0; c<5; c++) {
      uiaccountdetail->table_detail->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }
    i++;
  }
}

void MainWindow::loadSummaryDebts()
{
  uisummarydebts->table_info->setRowCount(0);
  QSqlQuery query;
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1"));
  int fulltotals[6];
  for (int t=0; t<6; t++) fulltotals[t]= 0;
  int fulltotal = 0;
  int i;
  for ( i=0; query.next(); i++) {
    uisummarydebts->table_info->insertRow(uisummarydebts->table_info->rowCount());
    uisummarydebts->table_info->setItem(i,0,new QTableWidgetItem(query.value(0).toString()));
    uisummarydebts->table_info->item(i,0)->setToolTip(query.value(1).toString());

    QString   querytext = QString("SELECT e.number num,record rec,account,date,name,detail,value val, "
                                  "  (SELECT sum(value) "
                                  "   FROM entries e,relations r "
                                  "   WHERE r.compensation_number = e.number AND r.compensation_record = e.record "
                                  "   AND source_number=num AND source_record=rec) "
                                  "FROM entries e, accounts a "
                                  "WHERE e.account = a.number "
                                  "AND descriptorid = '%1' AND type=0 AND account like '1320%' "
                                  "AND date <= '%2'"
                                  "ORDER BY date").arg(query.value(0).toString()).arg(uisummarydebts->dateedit_date->date().toString("yyyy-MM-dd"));

    int totals[6];
    for (int t=0; t<6; t++) totals[t]= 0;
    int total = 0;

    QSqlQuery individualquery;
    individualquery.exec(querytext);
    while(individualquery.next()) {
      if (!individualquery.value(7).isNull() && individualquery.value(6).toInt()<=individualquery.value(7).toInt()) continue;
      int debt = individualquery.value(6).toInt();
      if (!individualquery.value(7).isNull()) debt -= individualquery.value(7).toInt();

      int t = (individualquery.value(2).toString().right(2).toInt()-30)/5;
      totals[t] += debt;
      total += debt;
    }
    for (int t=0; t<6; t++) {
      uisummarydebts->table_info->setItem(i,1+t,new QTableWidgetItem(QString("%L1").arg(totals[t])));
      fulltotals[t] += totals[t];
    }
    uisummarydebts->table_info->setItem(i,7,new QTableWidgetItem(QString("%L1").arg(total)));
    fulltotal += total;

    for (int c=0; c<8; c++) {
      uisummarydebts->table_info->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      if (c>=1) {
        uisummarydebts->table_info->item(i,c)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
      }
    }
  }
  uisummarydebts->table_info->insertRow(uisummarydebts->table_info->rowCount());
  uisummarydebts->table_info->setItem(i,0,new QTableWidgetItem("Totales"));
  for (int t=0; t<6; t++) {
    uisummarydebts->table_info->setItem(i,1+t,new QTableWidgetItem(QString("%L1").arg(fulltotals[t])));
  }
  uisummarydebts->table_info->setItem(i,7,new QTableWidgetItem(QString("%L1").arg(fulltotal)));

  for (int c=0; c<8; c++) {
    uisummarydebts->table_info->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    uisummarydebts->table_info->item(i,c)->setForeground(Qt::blue);
    if (c>=1) {
      uisummarydebts->table_info->item(i,c)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    }
  }
}

void MainWindow::showDateSummaryDebts()
{
  uisummarydebts->label_date->setText(uisummarydebts->dateedit_date->date().toString("dd-MMM-yyyy"));
}

void MainWindow::printSummaryDebts()
{
  QPrinter printer(QPrinter::HighResolution);
  printer.setCreator("JP-Property");
  printer.setDocName("Resumen de Deudas");

  QPrintDialog printDialog(&printer, this);
  if (printDialog.exec() == QDialog::Accepted) {
    QPainter painter(&printer);
    QFont font = painter.font();
    font.setPointSize(font.pointSize()+4);
    painter.setFont(font);

    painter.drawText(ch(60),cv(7),ch(80),cv(6),Qt::AlignLeft,QString("Resumen de Deudas. %1").arg(uisummarydebts->dateedit_date->date().toString("dd-MMM-yyyy")));
    font.setPointSize(font.pointSize()-6);
    painter.setFont(font);

    QString headers[] = {"Casa","Expensa","Mora","Extraordinaria","Inasistencia","Retroactivo","Otros","Total"};

    font.setBold(true);
    painter.setFont(font);

    painter.drawText(ch(10),cv( (17) ),ch(22),cv(5),Qt::AlignLeft,headers[0]);
    for (int c=1; c<8; c++) {
      painter.drawText(ch((c*22)),cv( (17) ),ch(22),cv(5),Qt::AlignRight,headers[c]);
    }

    font.setBold(false);
    painter.setFont(font);

    int i;
    for (i=0; i<uisummarydebts->table_info->rowCount()-1; i++) {
      painter.drawText(ch(10),cv( (23+i*4.5) ),ch(30),cv(4.5),Qt::AlignLeft,uisummarydebts->table_info->item(i,0)->text());
      for (int c=1; c<8; c++) {
        painter.drawText(ch((c*22)),cv( (23+i*(4.5)) ),ch(22),cv(4.5),Qt::AlignRight,uisummarydebts->table_info->item(i,c)->text());
      }
    }
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(ch(10),cv( (25+i*4.5) ),ch(30),cv(4.5),Qt::AlignLeft,uisummarydebts->table_info->item(i,0)->text());
    for (int c=1; c<8; c++) {
      painter.drawText(ch((c*22)),cv( (25+i*(4.5)) ),ch(22),cv(4.5),Qt::AlignRight,uisummarydebts->table_info->item(i,c)->text());
    }
  }
}

void MainWindow::loadSummaryCollect()
{
    uisummarycollect->table_info->setRowCount(0);
    QSqlQuery query;
    query.exec(QString("SELECT year(date) y,month(date) m,sum(value) v "
                       "FROM entries "
                       "WHERE type = 0 AND account LIKE '11%' "
                       "GROUP BY y,m "
                       "ORDER BY y,m; "));
    QString months[] = {"Enero","Febrero","Marzo","Abril","Mayo","Junio","Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre"};

    for (int i=0; query.next(); i++) {
        uisummarycollect->table_info->insertRow(uisummarycollect->table_info->rowCount());
        uisummarycollect->table_info->setItem(i,0,new QTableWidgetItem(QString("%1").arg(query.value(0).toInt())));
        uisummarycollect->table_info->setItem(i,1,new QTableWidgetItem(months[query.value(1).toInt()-1]));
        uisummarycollect->table_info->setItem(i,2,new QTableWidgetItem(QString("%L1").arg(query.value(2).toInt())));
    }
}

QTreeWidgetItem* addTreeWidgetItem(QSqlQuery query,int year) {
    QStringList labels = QStringList(query.value(0).toString()+" - "+query.value(1).toString());
    int budget = 0;
    QString subquerytext = QString("SELECT sum(value) "
                                   "FROM budget "
                                   "WHERE year = %1 "
                                   "AND account LIKE '%2%'; ")
            .arg(year)
            .arg(query.value(0).toString());
    QSqlQuery subquery;
    subquery.exec(subquerytext);
    if (subquery.next()) {
        budget = subquery.value(0).toInt();
        labels = labels << QString("%L1").arg(budget);
    }
    else {
        labels = labels << "";
    }
    int total = 0;
    for (int i=1; i<=12; i++) {
        QString subquerytext = QString("SELECT sum(value) value "
                                       "FROM entries e,accounts a "
                                       "WHERE e.account = a.number "
                                       "AND account LIKE  '%1%' "
                                       "AND year(date)=%2 "
                                       "AND month(date)=%3; ")
                .arg(query.value(0).toString())
                .arg(year)
                .arg(i);

        QSqlQuery subquery;
        subquery.exec(subquerytext);
        if (subquery.next()) {
            labels = labels << QString("%L1").arg(subquery.value(0).toInt());
        }
        else {
            labels = labels << 0;
        }
        total += subquery.value(0).toInt();
    }
    labels = labels << QString("%L1").arg(total);

    int balance = budget - total;
    labels = labels << QString("%L1").arg(balance);

    labels = labels << QString("%1").arg(query.value(0).toString());

    QTreeWidgetItem* it = new QTreeWidgetItem((QTreeWidget*)0, labels);
    QColor textcolor,backcolor;
    switch(query.value(0).toString().length()) {
    case 2: textcolor = Qt::darkRed;        backcolor = QColor(255,200,200); break;
    case 4: textcolor = Qt::darkGreen;      backcolor = QColor(200,255,200); break;
    case 6: textcolor = QColor(50,50,50);   backcolor = QColor(255,255,255); break;
    }

    QFont font = it->font(3);
    font.setPointSize(10);

    it->setTextColor(0,textcolor);
    it->setBackgroundColor(0,backcolor);
    for (int i=1; i<=15; i++) {
        it->setTextAlignment(i,Qt::AlignRight|Qt::AlignCenter);
        it->setTextColor(i,textcolor);
        it->setFont(i,font);
    }

    it->setBackgroundColor(0,backcolor);
    it->setBackgroundColor(1,backcolor);
    it->setBackgroundColor(14,backcolor);
    it->setBackgroundColor(15,backcolor);

    return it;
}

void MainWindow::loadAccountsBudget(QTreeWidget* widget, QTreeWidgetItem* item, int handler)
{
    int year = uibudgetexecution->spinBox_year->text().toInt();
    QSqlQuery query;
    if (!item) {
        query.exec("SELECT number, name FROM accounts WHERE handler = 5 OR number = 15");
        while (query.next()) {
          QTreeWidgetItem* it = addTreeWidgetItem(query,year);
          widget->addTopLevelItem(it);
          loadAccountsBudget(widget,it,query.value(0).toInt());
        }
    }
    else {
        query.exec(QString("SELECT number, name FROM accounts WHERE handler=%1").arg(handler));
        while (query.next()) {
            QTreeWidgetItem* it = addTreeWidgetItem(query,year);
            item->addChild(it);
            loadAccountsBudget(widget,it,query.value(0).toInt());
        }
    }
}

void MainWindow::loadAccountsBudgetTotals(QTreeWidget* widget) {
    int year = uibudgetexecution->spinBox_year->text().toInt();
    QStringList labels = QStringList("TOTALES");
    int budget = 0;
    QString subquerytext = QString("SELECT sum(value) "
                                   "FROM budget "
                                   "WHERE year = %1; ")
            .arg(year);
    QSqlQuery subquery;
    subquery.exec(subquerytext);
    if (subquery.next()) {
        budget = subquery.value(0).toInt();
        labels = labels << QString("%L1").arg(budget);
    }
    else {
        labels = labels << "";
    }
    int total = 0;
    for (int i=1; i<=12; i++) {
        QString subquerytext = QString("SELECT sum(value) value "
                                       "FROM entries e,accounts a "
                                       "WHERE e.account = a.number "
                                       "AND (account LIKE  '5%' OR account LIKE '15%') "
                                       "AND year(date)=%1 "
                                       "AND month(date)=%2; ")
                .arg(year)
                .arg(i);

        QSqlQuery subquery;
        subquery.exec(subquerytext);
        if (subquery.next()) {
            labels = labels << QString("%L1").arg(subquery.value(0).toInt());
        }
        else {
            labels = labels << 0;
        }
        total += subquery.value(0).toInt();
    }
    labels = labels << QString("%L1").arg(total);

    int balance = budget - total;
    labels = labels << QString("%L1").arg(balance);

    QTreeWidgetItem* it = new QTreeWidgetItem((QTreeWidget*)0, labels);

    QFont font = it->font(3);
    font.setPointSize(10);

    it->setTextColor(0,Qt::darkBlue);
    for (int i=1; i<=15; i++) {
        it->setTextAlignment(i,Qt::AlignRight|Qt::AlignCenter);
        it->setTextColor(i,Qt::darkBlue);
        it->setFont(i,font);
    }

    widget->addTopLevelItem(it);
}

void MainWindow::switchBudgetMounthHidde()
{
    uibudgetexecution->tree_puc->setColumnHidden(2,!uibudgetexecution->checkBox_1->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(3,!uibudgetexecution->checkBox_2->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(4,!uibudgetexecution->checkBox_3->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(5,!uibudgetexecution->checkBox_4->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(6,!uibudgetexecution->checkBox_5->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(7,!uibudgetexecution->checkBox_6->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(8,!uibudgetexecution->checkBox_7->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(9,!uibudgetexecution->checkBox_8->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(10,!uibudgetexecution->checkBox_9->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(11,!uibudgetexecution->checkBox_10->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(12,!uibudgetexecution->checkBox_11->isChecked());
    uibudgetexecution->tree_puc->setColumnHidden(13,!uibudgetexecution->checkBox_12->isChecked());
}

void MainWindow::adjustBudgetView()
{

}

int updateBudgetValues(QTreeWidget* tree, QTreeWidgetItem *it) {
    if (!it) {
        int sum = 0;
        for (int i=0; i<tree->topLevelItemCount()-1; i++) {
           sum += updateBudgetValues(tree,tree->topLevelItem(i));
        }
        return sum;
    }
    else {
        if (it->childCount()==0) return it->text(1).remove(QRegExp("[.,]")).toInt();
        int sum = 0;
        for (int i=0; i<it->childCount(); i++) {
           sum += updateBudgetValues(tree,it->child(i));
        }
        int total = it->text(14).remove(QRegExp("[.,]")).toInt();
        it->setText(1,QString("%L1").arg(sum));
        it->setText(15,QString("%L1").arg(sum-total));
        return sum;
    }
}

void MainWindow::openBudgetDetail(QTreeWidgetItem *it, int column)
{
    int year = uibudgetexecution->spinBox_year->text().toInt();
    QString account = it->data(0,0).toString().split(" ").at(0);
    if (column == 1) {
        if (it->childCount()!=0) return;
        bool ok;
        QString text = QInputDialog::getText(this, tr("QInputDialog::getText()"),
                                             tr("Valor:"), QLineEdit::Normal,
                                             it->text(1), &ok);
        if (ok && !text.isEmpty()) {
            it->setText(1,QString("%L1").arg(text.toInt()));
            int total = it->text(14).remove(QRegExp("[.,]")).toInt();
            it->setText(15,QString("%L1").arg(text.toInt()-total));
            int sum = updateBudgetValues(uibudgetexecution->tree_puc,NULL);
            uibudgetexecution->tree_puc->topLevelItem(3)->setText(1,QString("%L1").arg(sum));
            total = uibudgetexecution->tree_puc->topLevelItem(3)->text(14).remove(QRegExp("[.,]")).toInt();
            uibudgetexecution->tree_puc->topLevelItem(3)->setText(15,QString("%L1").arg(sum-total));

            int year = uibudgetexecution->spinBox_year->value();
            int account = it->text(16).toInt();
            int value = text.toInt();

            QString querytext = QString("SELECT * FROM budget WHERE year=%1 AND account=%2; ").arg(year).arg(account);
            QSqlQuery query;
            query.exec(querytext);
            if (query.next()) {
                querytext = QString("UPDATE budget SET value=%1 WHERE year=%2 AND account=%3; ").arg(value).arg(year).arg(account);
                query.exec(querytext);
            }
            else {
                querytext = QString("INSERT INTO budget VALUES (%1,%2,%3,null); ").arg(year).arg(account).arg(value);
                query.exec(querytext);
            }
        }
    }
    else {
        openAccountDetail(account.toInt(),year,column-1);
    }
}

void MainWindow::openAccountDetail(int account, int year, int month)
{
    QString months[] = {"","Enero","Febrero","Marzo","Abril","Mayo","Junio"
                       "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre"};
    QString title = "";

    if (month>13) return;
    if (month==13) title = QString("%1").arg(year);
    else title = QString("%1/%2").arg(months[month]).arg(year);

    foreach (QMdiSubWindow* subwindow, ui->mdiArea->subWindowList()) {
      if (subwindow->windowTitle() == QString("Ejecución cuenta %1 [%2]").arg(account).arg(title)) {
        subwindow->activateWindow();
        return;
      }
    }

    QWidget* subwindow = new QWidget;
    uiaccountdetail = new Ui::AccountDetail;
    uiaccountdetail->setupUi(subwindow);
    subwindow->setWindowTitle(QString("Ejecución cuenta %1 [%2]").arg(account).arg(title));
    subwindow->setMinimumWidth(620);
    QStringList labels;
    uiaccountdetail->table_detail->setHorizontalHeaderLabels(labels<<"Fecha"<<"Concepto"<<"Debito"<<"Crédito");
    uiaccountdetail->table_detail->setColumnWidth(0, 90);
    uiaccountdetail->table_detail->setColumnWidth(1,350);
    uiaccountdetail->table_detail->setColumnWidth(2, 80);
    uiaccountdetail->table_detail->setColumnWidth(3, 80);
    uiaccountdetail->table_detail->setColumnHidden(4,true);
    ui->mdiArea->addSubWindow(subwindow);
    subwindow->show();
    loadAccountDetail(account,year,month);
}

void MainWindow::changeBudgetYear(int)
{
    uibudgetexecution->tree_puc->clear();
    loadAccountsBudget(uibudgetexecution->tree_puc);
    loadAccountsBudgetTotals(uibudgetexecution->tree_puc);
    uibudgetexecution->tree_puc->expandToDepth(0);
}

void MainWindow::createBackup()
{

    QString backupname = QString("jpbilling-backup-%1%2%3-%4%5%6.sql")
            .arg(QString("%1").arg(QDate::currentDate().year()).rightJustified(4,'0'))
            .arg(QString("%1").arg(QDate::currentDate().month()).rightJustified(2,'0'))
            .arg(QString("%1").arg(QDate::currentDate().day()).rightJustified(2,'0'))
            .arg(QString("%1").arg(QTime::currentTime().hour()).rightJustified(2,'0'))
            .arg(QString("%1").arg(QTime::currentTime().minute()).rightJustified(2,'0'))
            .arg(QString("%1").arg(QTime::currentTime().second()).rightJustified(2,'0'));

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                backupname,
                                tr("DB Backup (*.sql)"));

    if (fileName=="") return;

    QProcess dumpProcess(this);
    QStringList args;
    args << "-uroot" << "-p123" << "accounting" <<"-B";
    dumpProcess.setStandardOutputFile(fileName);
    dumpProcess.start("mysqldump", args);
    while( !dumpProcess.waitForFinished(100) );
}

