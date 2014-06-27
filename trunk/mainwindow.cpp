#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_paymentswindow.h"
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QMessageBox>
#include <iostream>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    resize(800,500);
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
  db.setUserName("accountant");
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
  connect(uipayments->combobox_type,SIGNAL(),uipayments->combobox_provider,SLOT(setFocus()));
  connect(uipayments->combobox_provider,SIGNAL(currentTextChanged(QString)),uipayments->dateedit_date,SLOT(setFocus()));
  connect(uipayments->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDatePayment()));
  connect(uipayments->lineedit_value,SIGNAL(editingFinished()),uipayments->lineedit_detail,SLOT(setFocus()));
  connect(uipayments->lineedit_detail,SIGNAL(editingFinished()),uipayments->lineedit_voucher,SLOT(setFocus()));
  loadProvidersType();
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
    if (subwindow->windowTitle() == "Facturación a registrar") {
      subwindow->activateWindow();
      return NULL;
    }
  }

  QWidget* subwindow = new QWidget;
  uibillinglist = new Ui::BillingListWindow;
  uibillinglist->setupUi(subwindow);
  subwindow->setWindowTitle("Facturación a registrar");
  subwindow->setMinimumWidth(330);
  ui->mdiArea->addSubWindow(subwindow);
  QStringList labels;
  uibillinglist->table_billing->setHorizontalHeaderLabels(labels<<"Casa"<<"Valor"<<"CI");
  uibillinglist->label_number->setHidden(true);
  uibillinglist->label_detail->setHidden(true);
  uibillinglist->label_account0->setHidden(true);
  uibillinglist->label_account1->setHidden(true);
  uibillinglist->label_date->setHidden(true);
  connect(uibillinglist->button_save,SIGNAL(clicked()),this,SLOT(saveGeneralBilling()));
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
  subwindow->setMinimumWidth(600);
  ui->mdiArea->addSubWindow(subwindow);
  QStringList labels;
  uicollect->table_detail->setHorizontalHeaderLabels(labels<<"Concepto"<<"Detalle"<<"Deuda"<<"Valor");
  uicollect->table_detail->setColumnWidth(0,190);
  uicollect->table_detail->setColumnWidth(1,170);
  uicollect->table_detail->setColumnWidth(2, 90);
  uicollect->table_detail->setColumnWidth(3, 90);
  uicollect->table_detail->setColumnHidden(4,true);
  uicollect->table_detail->setColumnHidden(5,true);
  subwindow->show();
  connect(uicollect->lineedit_home,SIGNAL(editingFinished()),this,SLOT(loadDebut()));
  connect(uicollect->button_save,SIGNAL(clicked()),this,SLOT(saveCollect()));
  connect(uicollect->table_detail,SIGNAL(cellChanged(int,int)),this,SLOT(resumeCollect(int,int)));
  connect(uicollect->dateedit_date,SIGNAL(editingFinished()),this,SLOT(showDateCollect()));
  connect(uicollect->lineedit_voucher,SIGNAL(editingFinished()),uicollect->table_detail,SLOT(setFocus()));
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
  subwindow->setMinimumWidth(330);
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
    if (subwindow->windowTitle() == "Proveedores de servicios") {
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
  subwindow->setMinimumWidth(630);
  QStringList labels;
  uihomehistory->table_history->setHorizontalHeaderLabels(labels<<"Fecha"<<"Concepto"<<"Debito"<<"Crédito"<<"Saldo");
  uihomehistory->table_history->setColumnWidth(0, 90);
  uihomehistory->table_history->setColumnWidth(1,250);
  uihomehistory->table_history->setColumnWidth(2, 80);
  uihomehistory->table_history->setColumnWidth(3, 80);
  uihomehistory->table_history->setColumnWidth(4, 80);
  ui->mdiArea->addSubWindow(subwindow);
  subwindow->show();
  connect(uihomehistory->lineedit_home,SIGNAL(editingFinished()),this,SLOT(loadHistory()));
}


void MainWindow::loadProvidersType()
{
  uipayments->combobox_type->addItem("");
  QSqlQuery query;
  query.exec(QString("SELECT number,name FROM accounts WHERE handler IN (SELECT number FROM accounts WHERE handler = 51)"));
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
    uipayments->combobox_provider->addItem(query.value(0).toString()+" "+query.value(1).toString(),query.value(0).toInt());
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

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  account0    = uipayments->combobox_type->currentText().split(" ").at(0).toInt();
  account1    = uipayments->combobox_from->currentText().split(" ").at(0).toInt();
  detail      = uipayments->lineedit_detail->text();
  date        = uipayments->dateedit_date->date().toString("yyyy-MM-dd");
  value       = uipayments->lineedit_value->text().toInt();
  descriptor  = uipayments->combobox_provider->currentText().split(" ").at(0);
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

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
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
//  handler = uibilling->combobox_type->currentData().toString();
  handler = uibilling->combobox_type->currentText().split(" ").at(0);

  query.exec(QString("SELECT id FROM descriptors WHERE type=1"));
  QWidget* list = openBillingList();
  uibillinglist->label_number->setText(QString("%1").arg(number));
  uibillinglist->label_detail->setText(uibilling->lineedit_detail->text());
  uibillinglist->label_account0->setText(handler);
  uibillinglist->label_account1->setText(QString("%1").arg(425035*100 + handler.toInt()%100));
  uibillinglist->label_date->setText(uibilling->dateedit_date->date().toString("yyyy-MM-dd"));

  int i=0;
  while (query.next()) {
    uibillinglist->table_billing->setItem(i,0,new QTableWidgetItem(query.value(0).toString()));
    uibillinglist->table_billing->setItem(i,1,new QTableWidgetItem(QString("%L1").arg(value)));
    uibillinglist->table_billing->setItem(i,2,new QTableWidgetItem(QString("%1").arg(voucher)));

    if (uibilling->checkbox_voucher->isChecked()) { voucher++; }
    i++;
  }
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
  QString   querytext = QString("SELECT entries.number AS n,account,date,name,detail,value,descriptorid,"
                                "(SELECT sum(value) FROM entries WHERE type=1 AND joinentry=n) "
                                "FROM entries,accounts "
                                "WHERE account = 132030 AND type=0 AND date >= '%1' "
                                "AND entries.account=accounts.number "
                                "ORDER BY descriptorid,date").arg(uibilling->dateedit_since->date().toString("yyyy-MM-dd"));
  query.exec(querytext);
  int i=0;
  while (query.next()) {
    if (!query.value(6).isNull() && query.value(5).toInt()<=query.value(7).toInt()) continue;

    QSqlQuery fastquery;
    QString fastquerytext =   QString("SELECT count(*) "
                                      "FROM entries "
                                      "WHERE account = 132035 AND detail like '%1%' "
                                      "AND descriptorid = '%2'").arg(query.value(4).toString()).arg(query.value(6).toString());
    fastquery.exec(fastquerytext);
    fastquery.next();
    int consecutive = fastquery.value(0).toInt()+1;

    uibilling->table_penalties->insertRow(uibilling->table_penalties->rowCount());
    uibilling->table_penalties->setItem(i,0,new QTableWidgetItem(query.value(6).toString()));
    uibilling->table_penalties->setItem(i,1,new QTableWidgetItem(query.value(4).toString()+QString(" [%1]").arg(consecutive)));
    int debut = query.value(5).toInt();
    if (!query.value(6).isNull()) debut -= query.value(7).toInt();
    uibilling->table_penalties->setItem(i,2,new QTableWidgetItem(QString("%L1").arg(debut)));
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
  int     account1  = 42503535;
  QString date      = uibilling->dateedit_at->date().toString("yyyy-MM-dd");
  QString home;

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

    QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,null)").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(home);
    query.exec(querytext0);

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,null)").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(home);
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

  for (int i=0; i<49; i++) {
    name      = uibillinglist->table_billing->item(i,0)->text();
    value     = uibillinglist->table_billing->item(i,1)->text().toInt();
    voucher   = uibillinglist->table_billing->item(i,2)->text().toInt();
    detail    = uibillinglist->label_detail->text();

    QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(name).arg(voucher);
    query.exec(querytext0);

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(name).arg(voucher);
    query.exec(querytext1);

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
  account1    = 425035*100 + account0%100;
  detail      = uibilling->lineedit_p_detail->text();
  date        = uibilling->dateedit_p_date->date().toString("yyyy-MM-dd");
  value       = uibilling->lineedit_p_value->text().toInt();
  descriptor  = uibilling->combobox_p_home->currentText();
  voucher     = uibilling->lineedit_p_voucher->text().toInt();

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  QString querytext1 = QString("INSERT INTO entries VALUES(%1,2,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
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
      widget->addTopLevelItem(it);
      loadAccounts(widget,it,query.value(0).toInt());
    }
  }
  else {
    query.exec(QString("SELECT number, name FROM accounts WHERE handler=%1").arg(handler));
    while (query.next()) {
      QStringList labels = QStringList(query.value(0).toString()+" - "+query.value(1).toString());
      QTreeWidgetItem* it = new QTreeWidgetItem((QTreeWidget*)0, labels<<query.value(0).toString());
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

void MainWindow::loadDebut()
{

  QSqlQuery query;
  query.exec(QString("SELECT id,field1 FROM descriptors WHERE type=1 AND id = 'Casa%1'").arg(uicollect->lineedit_home->text()));
  if (query.next()) {
    uicollect->label_home->setText(query.value(0).toString());
    uicollect->label_owner->setText(query.value(1).toString());
    uicollect->dateedit_date->setFocus();
  }
  else {
    uicollect->label_data->setText("");
    uicollect->lineedit_home->selectAll();
    return;
  }

  uicollect->table_detail->setRowCount(0);

  QString id = uicollect->label_home->text();

  QString   querytext = QString("SELECT entries.number AS n,account,date,name,detail,value, "
                                "(SELECT sum(value) FROM entries WHERE type=1 AND joinentry=n)"
                                "FROM entries,accounts "
                                "WHERE account LIKE '1320%' AND descriptorid='%1' AND type=0 "
                                "AND entries.account=accounts.number "
                                "ORDER BY date").arg(id);
  query.exec(querytext);
  int i=0,total=0;
  while (query.next()) {
    if (!query.value(6).isNull() && query.value(5).toInt()<=query.value(6).toInt()) continue;
    uicollect->table_detail->insertRow(uicollect->table_detail->rowCount());
    uicollect->table_detail->setItem(i,0,new QTableWidgetItem(query.value(3).toString()));
    uicollect->table_detail->setItem(i,1,new QTableWidgetItem(query.value(4).toString()));

    int debut = query.value(5).toInt();
    if (!query.value(6).isNull()) debut -= query.value(6).toInt();

    uicollect->table_detail->setItem(i,2,new QTableWidgetItem(QString("%L1").arg(debut)));
    uicollect->table_detail->setItem(i,3,new QTableWidgetItem("0"));
    uicollect->table_detail->setItem(i,4,new QTableWidgetItem(query.value(0).toString()));
    uicollect->table_detail->setItem(i,5,new QTableWidgetItem(query.value(1).toString()));
    for (int c=0; c<3; c++) {
      uicollect->table_detail->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }
    i++;
    total += debut; //query.value(5).toInt();
  }
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
  int       joinentry;
  QString   detail;
  QString   descriptor;
  QString   voucher;

  query.exec("SELECT max(number) FROM entries");
  if (query.next()) {
    number = query.value(0).toInt()+1;
  }

  account0    = 110505; // General cashing
  date        = uicollect->dateedit_date->date().toString("yyyy-MM-dd");
  voucher     = uicollect->lineedit_voucher->text();
  value       = uicollect->label_total->text().toInt();
  detail      = QString("Recaudo");
  descriptor  = uicollect->label_home->text();

  QString querytext0 = QString("INSERT INTO entries VALUES(%1,1,%2,'%3',%4,%5,'%6','%7',null,%8)").arg(number).arg(account0).arg(date).arg(DEBIT).arg(value).arg(detail).arg(descriptor).arg(voucher);
  query.exec(querytext0);

  int record = 2;
  for (int i=0; i<uicollect->table_detail->rowCount(); i++) {
    value       = uicollect->table_detail->item(i,3)->text().toInt();
    if (value==0) continue;
    account1    = uicollect->table_detail->item(i,5)->text().toInt();
    joinentry   = uicollect->table_detail->item(i,4)->text().toInt();
    detail      = uicollect->table_detail->item(i,1)->text();

    QString querytext1 = QString("INSERT INTO entries VALUES(%1,%9,%2,'%3',%4,%5,'%6','%7',%8,%10)").arg(number).arg(account1).arg(date).arg(CREDIT).arg(value).arg(detail).arg(descriptor).arg(joinentry).arg(record).arg(voucher);
    query.exec(querytext1);

    QString querytext2 = QString("UPDATE entries SET joinentry=%1 WHERE number=%2 AND account=%3").arg(number).arg(joinentry).arg(account1);
    query.exec(querytext2);
    record ++;
  }

  QMessageBox::information(this,"Transacción procesada",QString("Asiento registrado [Número: %1]").arg(number));
  uicollect->lineedit_home->setText("");
  uicollect->label_home->setText("");
  uicollect->label_owner->setText("");
  uicollect->lineedit_voucher->setText("");
  uicollect->table_detail->setRowCount(0);
  uicollect->lineedit_home->setFocus();
  uicollect->label_debt->setText("");
  uicollect->label_total->setText("");
}

void MainWindow::resumeCollect(int,int col)
{
  if (col==3) {
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
  }
  else {
    return;
  }
  uihomehistory->lineedit_home->selectAll();

  QString id = uihomehistory->label_home->text();
  query.exec(QString("SELECT field1 FROM descriptors WHERE id='%1'").arg(id));
  query.next();
  uihomehistory->label_owner->setText(query.value(0).toString());
  query.exec(QString("SELECT date,name,type+0,value,detail,entries.number,joinentry FROM entries,accounts WHERE descriptorid='%1' AND account like '1320%' AND accounts.number=entries.account ORDER BY date").arg(id));

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
    uihomehistory->table_history->setItem(i,1,new QTableWidgetItem((type==DEBIT?"CxC ":"Pago ")+account+": "+detail));
    uihomehistory->table_history->setItem(i,2+type,new QTableWidgetItem(QString("%L1").arg(value)));
    uihomehistory->table_history->setItem(i,3-type,new QTableWidgetItem(""));
    uihomehistory->table_history->setItem(i,4,new QTableWidgetItem(QString("%L1").arg(balance)));
    uihomehistory->table_history->item(i,2)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uihomehistory->table_history->item(i,3)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uihomehistory->table_history->item(i,4)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);

    for (int c=0; c<5; c++) {
      uihomehistory->table_history->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }

    i++;
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

void MainWindow::loadAccountDetail(int account)
{
  QSqlQuery query;
  query.exec(QString("SELECT date,name,type+0,value,detail,entries.number,joinentry,descriptorid "
                     "FROM entries,accounts "
                     "WHERE account = %1 AND accounts.number=entries.account "
                     "ORDER BY date").arg(account));

  uiaccountdetail->table_detail->setRowCount(0);

  int i=0;
  int balance = 0;
  while (query.next()) {
    uiaccountdetail->table_detail->insertRow(uiaccountdetail->table_detail->rowCount());
    QString date        = query.value(0).toDate().toString("dd-MMM-yyyy");
    int     type        = query.value(2).toBool();
    int     value       = query.value(3).toInt();
    QString detail      = query.value(4).toString();
    QString descriptor  = query.value(7).toString();
    type==DEBIT?balance+=value:balance-=value;

    uiaccountdetail->table_detail->setItem(i,0,new QTableWidgetItem(date));
    uiaccountdetail->table_detail->setItem(i,1,new QTableWidgetItem(descriptor+": "+detail));
    uiaccountdetail->table_detail->setItem(i,2+type,new QTableWidgetItem(QString("%L1").arg(value)));
    uiaccountdetail->table_detail->setItem(i,3-type,new QTableWidgetItem(""));
    uiaccountdetail->table_detail->setItem(i,4,new QTableWidgetItem(QString("%L1").arg(balance)));
    uiaccountdetail->table_detail->item(i,2)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uiaccountdetail->table_detail->item(i,3)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
    uiaccountdetail->table_detail->item(i,4)->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);

    for (int c=0; c<5; c++) {
      uiaccountdetail->table_detail->item(i,c)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }
    i++;
  }
}
