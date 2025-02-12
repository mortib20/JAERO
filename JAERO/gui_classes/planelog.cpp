#include "planelog.h"
#include "ui_planelog.h"

#include <QDateTime>
#include <QDebug>
#include <QFontMetrics>
#include <QSettings>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStandardPaths>
#include <QFileDialog>
#include <QImage>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>
#include <QClipboard>
#include <QScrollBar>
#include <QMessageBox>
#include <QMenuBar>
#include <QRegularExpression>
#include "settingsdialog.h"

int PlaneLog::findAESrow(const QString &aes)
{
    int rows = ui->tableWidget->rowCount();
    int idx=-1;
    for(int i = 0; i < rows; ++i)
    {
        if(ui->tableWidget->item(i, 0)->text() == aes)
        {
            idx=i;
            break;
        }
    }
    return idx;
}

void PlaneLog::imageUpdateslot(const QPixmap &image)
{
    ui->toolButtonimg->setIcon(image);
}

void PlaneLog::dbUpdateUserClicked(bool ok, const QStringList &dbitem)
{
    ui->label_type->clear();
    ui->label_owner->clear();

    if(!ok)
    {
        if(dbitem.size())
        {
             dbUpdateerrorslot(dbitem[0]);
        } else dbUpdateerrorslot("Error: Unknown");
    }
    else
    {

        if(dbitem.size()!=QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>().keyCount())
        {
            dbUpdateerrorslot("Error: Database responce size wrong");
            return;
        }

        if(!selectedAESitem)return;
        if(selectedAESitem->row()<0)return;
        if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
        if(selectedAESitem->text()!=dbitem[DataBaseTextUser::DataBaseSchema::ModeS])return;
        int row=selectedAESitem->row();

        QString manufacturer_and_type=dbitem[DataBaseTextUser::DataBaseSchema::Manufacturer]+" "+dbitem[DataBaseTextUser::DataBaseSchema::Type];
        manufacturer_and_type=manufacturer_and_type.trimmed();

        ui->label_type->setText(manufacturer_and_type);
        ui->label_owner->setText(dbitem[DataBaseTextUser::DataBaseSchema::RegisteredOwners]);

        ui->plainTextEditdatabase->clear();
        ui->plainTextEditdatabase->appendPlainText("Reg. ID         \t"+dbitem[DataBaseTextUser::DataBaseSchema::Registration]);
        ui->plainTextEditdatabase->appendPlainText("Model           \t"+dbitem[DataBaseTextUser::DataBaseSchema::ICAOTypeCode]);
        ui->plainTextEditdatabase->appendPlainText("Type            \t"+manufacturer_and_type);
        ui->plainTextEditdatabase->appendPlainText("Owner           \t"+dbitem[DataBaseTextUser::DataBaseSchema::RegisteredOwners]);
        ui->plainTextEditdatabase->appendPlainText("Country         \t"+dbitem[DataBaseTextUser::DataBaseSchema::ModeSCountry]);

    }
}

void PlaneLog::dbUpdateACARSMessage(bool ok, const QStringList &dbitem)
{
    if(!ok)return;
    if(dbitem.size()!=QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>().keyCount())return;
    int row=findAESrow(dbitem[DataBaseTextUser::DataBaseSchema::ModeS]);
    if(row<0)return;

    QTableWidgetItem *Modelitem=ui->tableWidget->item(row,TableWidgetColumn::Number::Model);
    QTableWidgetItem *Owneritem=ui->tableWidget->item(row,TableWidgetColumn::Number::Owner);
    QTableWidgetItem *Countryitem=ui->tableWidget->item(row,TableWidgetColumn::Number::Country);
    if(Modelitem==nullptr||Owneritem==nullptr||Countryitem==nullptr)
    {
        qDebug()<<"software bug nullpointer dbUpdateACARSMessage";
        return;
    }
    Modelitem->setText(dbitem[DataBaseTextUser::DataBaseSchema::ICAOTypeCode].trimmed());
    Owneritem->setText(dbitem[DataBaseTextUser::DataBaseSchema::RegisteredOwners].trimmed());
    Countryitem->setText(dbitem[DataBaseTextUser::DataBaseSchema::ModeSCountry].trimmed());
}

void PlaneLog::dbUpdateslot(bool ok, int ref, const QStringList &dbitem)
{
    //dispatcher for who should deal with the db responce
    DBaseRequestSource *whoCalled=(DBaseRequestSource*)dbc->getuserdata(ref);
    if(whoCalled==nullptr)return;
    {
        switch (*whoCalled)
        {
        case DBaseRequestSource::Source::UserClicked:
            dbUpdateUserClicked(ok,dbitem);
            break;
        case DBaseRequestSource::Source::ACARSMessage:
            dbUpdateACARSMessage(ok,dbitem);
            break;
        default:
            break;
        }
    }
}

void PlaneLog::dbUpdateerrorslot(const QString &error)
{
    ui->label_type->clear();
    ui->label_owner->clear();
    ui->plainTextEditdatabase->clear();
    ui->plainTextEditdatabase->setPlainText(error);
}

void PlaneLog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatescrollbar();
}

void PlaneLog::updatescrollbar()
{
    double dvertscrollbarval=wantedscrollprop*((double)ui->textEditmessages->verticalScrollBar()->maximum());
    ui->textEditmessages->verticalScrollBar()->setValue(dvertscrollbarval);
}

PlaneLog::PlaneLog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlaneLog)
{
    ui->setupUi(this);
    wantedheightofrow=3;

    ui->label_type->clear();
    ui->label_owner->clear();

    selectedAESitem=NULL;

    planesfolder=((const char*) 0);

    connect(ui->textEditmessages->verticalScrollBar(),SIGNAL(valueChanged(int)),this,SLOT(messagesliderchsnged(int)));

    //create image lookup controller and connect result to us
    ic=new ImageController(this);
    connect(ic,SIGNAL(result(QPixmap)),this,SLOT(imageUpdateslot(QPixmap)));

    //create database lookup controller and connect result to us
    dbc=new DataBaseTextUser(this);
    connect(dbc,SIGNAL(result(bool,int,QStringList)),this,SLOT(dbUpdateslot(bool,int,QStringList)));

    ui->actionLeftRight->setVisible(false);
    ui->actionUpDown->setVisible(false);
    ui->tableWidget->horizontalHeader()->setAutoScroll(true);
    ui->tableWidget->setColumnHidden(5,true);
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));

    //cant i just use a qmainwindow 2 times???
    QMainWindow * mainWindow = new QMainWindow();
    mainWindow->setCentralWidget(ui->widget);
    toolBar = new QToolBar();
    toolBar->addAction(ui->actionClear);
    toolBar->addAction(ui->actionImport_log);
    toolBar->addAction(ui->actionExport_log);
    toolBar->addSeparator();
    toolBar->addSeparator();
    toolBar->addAction(ui->actionUpDown);
    toolBar->addAction(ui->actionLeftRight);
    toolBar->addAction(ui->actionStopSorting);
    mainWindow->addToolBar(toolBar);
    QHBoxLayout * layout = new QHBoxLayout();
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(mainWindow);
    setLayout(layout);

    QMenuBar *menubar=new QMenuBar();
    QMenu *menu_file=new QMenu("File");
    menu_file->addAction(ui->actionImport_log);
    menu_file->addAction(ui->actionExport_log);
    menu_file->addAction(ui->actionClear);
    menubar->addMenu(menu_file);
    mainWindow->setMenuBar(menubar);

    connect(toolBar,SIGNAL(topLevelChanged(bool)),this,SLOT(updatescrollbar()));
    connect(toolBar,SIGNAL(visibilityChanged(bool)),this,SLOT(updatescrollbar()));

    //load settings
    QSettings settings("Jontisoft", settings_name);
    QFontMetrics fm(ui->tableWidget->font());

    int plane_log_db_schema_version=settings.value("PLANE_LOG_DB_SCHEMA_VERSION",-1).toInt();

    //capcity test
    //this is an issue as JAERO simply loads everything into ram
    //that limits the planelog to about 40000 on my computer more
    //than that it will crash.
    //something like QAbstractTableModel and on demand data might be
    //a solution but i'm not sure how to use it. it would require
    //figureing it out. see https://doc.qt.io/qt-5/qtwidgets-itemviews-fetchmore-example.html
    //for a list on demand. that's not a table but it's a place to start
    //looking.
#ifdef PLANELOG_CAPACITY_TEST
    const int test_rowCount=40000;
    ui->tableWidget->setRowCount(test_rowCount);
    for(int row=0;row<test_rowCount;row++)
    {
        ui->tableWidget->setRowHeight(row,fm.height()*wantedheightofrow);
        for(int column=0;column<TableWidgetColumn::Number_Of_Cols;column++)
        {
            QTableWidgetItem *newItem = new QTableWidgetItem();
            {
                QString str="null";
                switch (column)
                {
                case TableWidgetColumn::AES:
                    str=QString::number(row,16);
                    break;
                case TableWidgetColumn::REG:
                    str=QString::number(row,16);
                    break;
                case TableWidgetColumn::FirstHeard:
                    str=QDateTime::currentDateTimeUtc().toString();
                    break;
                case TableWidgetColumn::LastHeard:
                    str=QDateTime::currentDateTimeUtc().toString();
                    break;
                case TableWidgetColumn::Count:
                    str=QString::number(qrand());
                    break;
                case TableWidgetColumn::LastMessage:
                    str="04:37:58 19-03-21 AES:3C65AC GES:90 2 D-AIML ! H1 P●●- #MDPWI/WD320,GODOS,144010.CUTEL,190023.FORTY,239013.SUM,309013.GONUT,2●34018.N64000W010000,259033.N67000W020000,100014.N68000W030000,095037.●N69000W040000,123038.N69000W050000,118059.N69000W060000,121050.080C,3●3400- #MD9.N62300W090000,326035.N55300W100000,290059.VLN,278022.BIL,261019●.FFU,196038.MLF,197026.OVETO,259037.BLD,259045/WD340,GODOS,297008,340●M56.CUTEL,228012,340M59.FORTY,260012,340M59.SUM,291012,340M60.GONUT,2●55022,- #MD340M61.N64000W010000,253034,340M62.N67000W020000,106010,340M60.●N68000W030000,093035,340M66.N69000W040000,110034,340M68.N69000W050000●,120054,340M66.080C,347011,340M58.N62300W090000,326034,340M54.N55300W●100000,2- #MD89055,340M54.VLN,274025,340M57.BIL,252018,340M58.FFU,207028,3●40M56.MLF,227024,340M53.OVETO,258046,340M50/WD360,GODOS,305019.CUTEL,●276014.GONUT,262020.N64000W010000,246026.N67000W020000,131010.N68000W●030000,100- #MD031.N69000W050000,121045.N69000W060000,120056.080C,346014.N●62300W090000,324034.N55300W100000,291051.VLN,274027.BIL,240018.FFU,21●6024.MLF,241030.OVETO,259052/WD380,GODOS,305023.CUTEL,288017.SUM,2720●19.N64000W01- #MD0000,240022.N67000W020000,150010.N68000W030000,109028.N69●000W040000,094038.N69000W050000,128034.N69000W060000,128046.080C,3430●17.N62300W090000,323034.N55300W100000,297046.VLN,277028.BIL,241020.FF●U,226026.MLF,2- #MD47035.OVETO,259054/DD100252048.200248042.310258063.3502●61065.390262068704F●✈: 04:38:17 19-03-21 AES:3C65AC GES:90 2 D-AIML ! 3L U●●58536DB314852MAIRPORT WX,0002,00EGLL●SA 060950 AUTO 30004KT 7000 NCD 06/05 Q0996●  TEMPO 4000 BR BKN003●CF NOT AVBL●FT 060505 0606/0712 VRB03KT 7000 BKN040●  PROB30 TEMPO 0606/0609 1200 BR BKN002●  TEMPO 0609/0612 BKN008●  BECMG 0610/0613 9999●  BECMG 0622/0701 17010KT●  TEMPO 0623/0707 6000 RA BKN012●  PROB40 TEMPO 0703/0707 SCT005 BKN008●  BECMG 0705/0708 24012KT●●BIKF●SA 060930 07027KT CAVOK M00/M08 Q0988●CF NOT AVBL●FT 060744 0609/0709 07025G35KT 9999 SCT030 BKN050 TX04/0703Z●  TNM00/0609Z●  PROB40 TEMPO 0613/0703 07035G45KT SHRASN BKN015●  BECMG 0703/0706 14015KT RA BKN015 OVC025●●KLAX●SA 060953 00000KT 10SM FEW026 SCT040 10/07 A2993●CF NOT AVBL●FT 060902 0609/0712 01008KT P6SM FEW020 SCT035●  FM061400 11010KT P6SM -SHRA SCT025 SCT060●  FM061700 12012KT 5SM -RA BR BKN025 OVC040●  FM061900 16015G22KT 1 1/2SM +RA BR BKN007 OVC015●  FM062200 22015G22KT 4SM RA BR SCT008 OVC015●  FM070100 27010KT 6SM -RA BR SCT025 BKN035●  FM070900 VRB05KT 4SM RA BR SCT015 OVC025●";
                    str="✈: "+str;
                    break;
                case TableWidgetColumn::MessageCount:
                    str=QString::number(qrand());
                    break;
                case TableWidgetColumn::Model:
                    str=QString::number(qrand());
                    break;
                case TableWidgetColumn::Owner:
                    str=QString::number(qrand());
                    break;
                case TableWidgetColumn::Country:
                    str=QString::number(qrand());
                    break;
                case TableWidgetColumn::Notes:
                    str=QString::number(qrand());
                    break;
                }
                newItem->setText(str);
            }
            if(column<7)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
            newItem->setFlags((newItem->flags()&~Qt::ItemIsEditable)|Qt::ItemIsSelectable);
            ui->tableWidget->setItem(row, column, newItem);
        }
    }
#else
    if(plane_log_db_schema_version==PLANE_LOG_DB_SCHEMA_VERSION)
    {
        ui->tableWidget->setRowCount(settings.value("tableWidget-rows",0).toInt());
        for(int row=0;row<ui->tableWidget->rowCount();row++)
        {
            ui->tableWidget->setRowHeight(row,fm.height()*wantedheightofrow);
            for(int column=0;column<ui->tableWidget->columnCount();column++)
            {
                QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
                QTableWidgetItem *newItem = new QTableWidgetItem(settings.value(str,"").toString());
                if(column<7)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                newItem->setFlags((newItem->flags()&~Qt::ItemIsEditable)|Qt::ItemIsSelectable);
                ui->tableWidget->setItem(row, column, newItem);
            }
        }
    }
    //if old schema we have to change to new schema
    if(plane_log_db_schema_version<0)//b4 we called it a schema
    {
        ui->tableWidget->setRowCount(settings.value("tableWidget-rows",0).toInt());
        for(int row=0;row<ui->tableWidget->rowCount();row++)
        {
            ui->tableWidget->setRowHeight(row,fm.height()*wantedheightofrow);
            for(int column=0;column<ui->tableWidget->columnCount();column++)
            {
                QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
                QTableWidgetItem *newItem = new QTableWidgetItem(settings.value(str,"").toString());
                if(column<7)newItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
                if(column==7)column=10;
                else if(column==10)column=7;
                newItem->setFlags((newItem->flags()&~Qt::ItemIsEditable)|Qt::ItemIsSelectable);
                ui->tableWidget->setItem(row, column, newItem);
            }
        }
    }
#endif

    ui->tableWidget->selectRow(0);
    ui->splitter_2->restoreState(settings.value("splitter_2").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    restoreGeometry(settings.value("logwindow").toByteArray());
    wantedscrollprop=settings.value("wantedscrollprop",1.0).toDouble();

    ui->splitter_2->setStretchFactor(0, 2);
    ui->splitter_2->setStretchFactor(1, 2);
    ui->splitter_2->setStretchFactor(2, 10);

}

void PlaneLog::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void PlaneLog::showEvent(QShowEvent *event)
{
    updateinfopain();
    event->accept();
}

PlaneLog::~PlaneLog()
{
    //save settings
    //other options could be then easy to backup if need be
    //QSettings settings(QSettings::IniFormat, QSettings::UserScope,"Jontisoft", "JAERO");
    //QSettings settings(afilename,QSettings::IniFormat);
    QSettings settings("Jontisoft", settings_name);
    settings.setValue("tableWidget-rows",ui->tableWidget->rowCount());
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString str=((QString)"tableWidget-%1-%2").arg(row).arg(column);
            if(ui->tableWidget->item(row,column))settings.setValue(str,ui->tableWidget->item(row,column)->text());
        }
    }
    settings.setValue("PLANE_LOG_DB_SCHEMA_VERSION",PLANE_LOG_DB_SCHEMA_VERSION);
    settings.setValue("splitter", ui->splitter->saveState());
    settings.setValue("splitter_2", ui->splitter_2->saveState());
    settings.setValue("logwindow", saveGeometry());
    settings.setValue("wantedscrollprop",wantedscrollprop);

    delete ui;    
}

void PlaneLog::ACARSslot(ACARSItem &acarsitem)
{
    if(!acarsitem.valid)return;

    ui->tableWidget->setSortingEnabled(false);//!!!!!

    int rows = ui->tableWidget->rowCount();
    QString AESIDstr=((QString)"").asprintf("%06X",acarsitem.isuitem.AESID);
    bool found = false;
    int idx=-1;
    for(int i = 0; i < rows; ++i)
    {
        if(ui->tableWidget->item(i, 0)->text() == AESIDstr)
        {
            found = true;
            idx=i;
            break;
        }
    }
    QTableWidgetItem *AESitem;
    QTableWidgetItem *REGitem;
    QTableWidgetItem *FirstHearditem;
    QTableWidgetItem *LastHearditem;
    QTableWidgetItem *Countitem;
    QTableWidgetItem *LastMessageitem;
    QTableWidgetItem *MessageCountitem;
    QTableWidgetItem *Modelitem;
    QTableWidgetItem *Owneritem;
    QTableWidgetItem *Countryitem;
    QTableWidgetItem *Notesitem;
    if(!found)
    {
        idx=ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(idx);
        QFontMetrics fm(ui->tableWidget->font());
        ui->tableWidget->setRowHeight(idx,fm.height()*wantedheightofrow);
        AESitem = new QTableWidgetItem;
        REGitem = new QTableWidgetItem;
        FirstHearditem = new QTableWidgetItem;
        LastHearditem = new QTableWidgetItem;
        Countitem = new QTableWidgetItem;
        LastMessageitem = new QTableWidgetItem;
        MessageCountitem = new QTableWidgetItem;
        Modelitem = new QTableWidgetItem;
        Owneritem = new QTableWidgetItem;
        Countryitem = new QTableWidgetItem;
        Notesitem = new QTableWidgetItem;
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::AES,AESitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::REG,REGitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::FirstHeard,FirstHearditem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::LastHeard,LastHearditem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Count,Countitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::LastMessage,LastMessageitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::MessageCount,MessageCountitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Model,Modelitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Owner,Owneritem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Country,Countryitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Notes,Notesitem);
        AESitem->setText(AESIDstr);
        Countitem->setText("0");
        MessageCountitem->setText("0");

        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        FirstHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        MessageCountitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

        AESitem->setFlags(AESitem->flags()&~Qt::ItemIsEditable);
        REGitem->setFlags(REGitem->flags()&~Qt::ItemIsEditable);
        Countitem->setFlags(Countitem->flags()&~Qt::ItemIsEditable);
        FirstHearditem->setFlags(FirstHearditem->flags()&~Qt::ItemIsEditable);
        LastHearditem->setFlags(LastHearditem->flags()&~Qt::ItemIsEditable);
        Modelitem->setFlags(Modelitem->flags()&~Qt::ItemIsEditable);
        Owneritem->setFlags(Owneritem->flags()&~Qt::ItemIsEditable);
        Countryitem->setFlags(Countryitem->flags()&~Qt::ItemIsEditable);
        Notesitem->setFlags(Notesitem->flags()&~Qt::ItemIsEditable);

        FirstHearditem->setText(QDateTime::currentDateTime().toUTC().toString("yy-MM-dd hh:mm:ss"));
    }
    AESitem = ui->tableWidget->item(idx,TableWidgetColumn::Number::AES);
    REGitem = ui->tableWidget->item(idx,TableWidgetColumn::Number::REG);
    FirstHearditem = ui->tableWidget->item(idx,TableWidgetColumn::Number::FirstHeard);
    LastHearditem = ui->tableWidget->item(idx,TableWidgetColumn::Number::LastHeard);
    Countitem = ui->tableWidget->item(idx,TableWidgetColumn::Number::Count);
    LastMessageitem = ui->tableWidget->item(idx,TableWidgetColumn::Number::LastMessage);
    MessageCountitem = ui->tableWidget->item(idx,TableWidgetColumn::Number::MessageCount);

    REGitem->setText(acarsitem.PLANEREG);
    LastHearditem->setText(QDateTime::currentDateTime().toUTC().toString("yy-MM-dd hh:mm:ss"));

    Countitem->setText(QString::number(Countitem->text().toInt()+1));

    QString tmp=LastMessageitem->text();
    if(tmp.count('\n')>=10)
    {
        int idx=tmp.indexOf("\n");
        tmp=tmp.right(tmp.size()-idx-1);
    }

    if(!acarsitem.message.isEmpty())
    {

        MessageCountitem->setText(QString::number(MessageCountitem->text().toInt()+1));
        QString message=acarsitem.message;
        message.replace('\r','\n');
        message.replace("\n\n","\n");
        if(message.right(1)=="\n")message.chop(1);
        if(message.left(1)=="\n")message.remove(0,1);
        arincparser.parseDownlinkmessage(acarsitem);
        arincparser.parseUplinkmessage(acarsitem);
        message.replace('\n',"●");//● instead of \n\t

        QByteArray TAKstr;
        TAKstr+=acarsitem.TAK;
        if(acarsitem.TAK==0x15)TAKstr[0]='!';
        uchar label1=acarsitem.LABEL[1];
        if((uchar)acarsitem.LABEL[1]==127)label1='d';        

        if(acarsitem.nonacars)tmp+="✈: "+QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").asprintf("AES:%06X GES:%02X   %s       ●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.PLANEREG.data()));
         else
         {
            if((acarsitem.downlink)&&!arincparser.downlinkheader.flightid.isEmpty())
            {
                tmp+="✈: "+QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").asprintf("AES:%06X GES:%02X %c %s %s %c%c %c Flight %s●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI,arincparser.downlinkheader.flightid.toLatin1().data()));
            }
             else tmp+="✈: "+QDateTime::currentDateTime().toUTC().toString("hh:mm:ss dd-MM-yy ")+(((QString)"").asprintf("AES:%06X GES:%02X %c %s %s %c%c %c●●",acarsitem.isuitem.AESID,acarsitem.isuitem.GESID,acarsitem.MODE,acarsitem.PLANEREG.data(),TAKstr.data(),(uchar)acarsitem.LABEL[0],label1,acarsitem.BI));
         }

        if((!acarsitem.nonacars)&&arincparser.arincmessage.info.size()>2)
        {
            arincparser.arincmessage.info.replace("\n","●");
            LastMessageitem->setText(tmp+message+"●●"+arincparser.arincmessage.info);
        }
        else LastMessageitem->setText(tmp+message+"●");

        if(selectedAESitem&&(idx==selectedAESitem->row()))updateinfopain();

    }


    ui->tableWidget->setSortingEnabled(true);//allow sorting again

    //send a request off to the DB for an update. responce will be async
    if(!planesfolder.isNull())dbc->request(planesfolder,((QString)"").asprintf("%06X",acarsitem.isuitem.AESID),&dBaseRequestSourceACARSMessage);
}

void PlaneLog::on_actionClear_triggered()
{

    //confirm
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
    msgBox.setText("This will erase this window log\nAre you sure?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setWindowTitle("Confirm log deletion");
    switch(msgBox.exec())
    {
    case QMessageBox::No:
        return;
        break;
    case QMessageBox::Yes:
        break;
    default:
        return;
        break;
    }

    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++)
    {
       ui->tableWidget->removeRow(0);
    }
    ui->toolButtonimg->setIcon(QPixmap(":/images/Plane_clip_art.svg"));
    ui->labelAES->clear();
    ui->labelREG->clear();
    ui->labelfirst->clear();
    ui->labellast->clear();
    ui->label_owner->clear();
    ui->label_type->clear();
    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->textEditmessages->clear();
    ui->plainTextEditdatabase->clear();
    selectedAESitem=NULL;


    // remove from registry also, the application gets very slow to load if not removed.
    QSettings settings("Jontisoft", settings_name);
    QStringList keys = settings.childKeys();
    for(int a = 0; a<keys.size(); a++)
    {
        QString key = keys.at(a);
        if(key.startsWith("tableWidget"))settings.remove(key);
    }

}

void PlaneLog::on_actionUpDown_triggered()
{
    QFontMetrics fm(ui->tableWidget->font());
    int big=fm.height()*12;
    int small=fm.height()*3;

    //if adjusted by user then restore
    int rows = ui->tableWidget->rowCount();
    if(rows<1)return;
    double ave=0;
    for(int i = 0; i < rows; i++)
    {
        ave+=(double)ui->tableWidget->rowHeight(i);
    }
    ave=ave/((double)rows);
    double val=qMin(qAbs(ave-(double)big),qAbs(ave-(double)small));
    if(val>1.0)
    {
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
        return;
    }

    if(wantedheightofrow>5)
    {
        wantedheightofrow=3;
        QFontMetrics fm(ui->tableWidget->font());
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
    }
    else
    {
        wantedheightofrow=12;
        QFontMetrics fm(ui->tableWidget->font());
        int rows = ui->tableWidget->rowCount();
        for(int i = 0; i < rows; i++)ui->tableWidget->setRowHeight(i,fm.height()*wantedheightofrow);
    }
}

void PlaneLog::on_actionLeftRight_triggered()
{
    QFontMetrics fm(ui->tableWidget->font());
    int big = fm.horizontalAdvance('_') * (220 + 50);
    int small = fm.horizontalAdvance('_') * (10);
    if(ui->tableWidget->columnWidth(ui->tableWidget->columnCount()-1)>(big-2))
    {
        ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,small);
        ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    }
     else ui->tableWidget->setColumnWidth(ui->tableWidget->columnCount()-1,big);
}

void PlaneLog::on_actionStopSorting_triggered()
{
    ui->tableWidget->sortItems(-1);
}

void PlaneLog::on_tableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);
    if(currentRow==previousRow)return;
    selectedAESitem=ui->tableWidget->item(currentRow, 0);
    updateinfopain();
}

void PlaneLog::updateinfopain()
{

    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();

    QTableWidgetItem *LastMessageitem=ui->tableWidget->item(row,TableWidgetColumn::Number::LastMessage);
    QTableWidgetItem *AESitem = ui->tableWidget->item(row,TableWidgetColumn::Number::AES);
    QTableWidgetItem *REGitem = ui->tableWidget->item(row,TableWidgetColumn::Number::REG);
    QTableWidgetItem *FirstHearditem = ui->tableWidget->item(row,TableWidgetColumn::Number::FirstHeard);
    QTableWidgetItem *LastHearditem = ui->tableWidget->item(row,TableWidgetColumn::Number::LastHeard);
    QTableWidgetItem *Notesitem = ui->tableWidget->item(row,TableWidgetColumn::Number::Notes);

    ui->labelAES->setText(AESitem->text());
    ui->labelREG->setText(REGitem->text());
    ui->labelfirst->setText(FirstHearditem->text());
    ui->labellast->setText(LastHearditem->text());

    QString str=LastMessageitem->text();
    str.replace("●","\n\t");
    str.replace("✈: ","\n");

    //remember scroll val
    if(ui->textEditmessages->verticalScrollBar()->maximum()>0)wantedscrollprop=((double)ui->textEditmessages->verticalScrollBar()->value())/((double)ui->textEditmessages->verticalScrollBar()->maximum());
    ui->textEditmessages->setText(str);
    updatescrollbar();

    //Qt is great. so simple. look, just one line
    ic->asyncImageLookupFromAES(planesfolder,AESitem->text());

    //db lookup request
    ui->label_type->clear();
    ui->label_owner->clear();
    ui->plainTextEditdatabase->clear();
    if(!planesfolder.isNull())dbc->request(planesfolder,AESitem->text(),&dBaseRequestSourceUserCliecked);

    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    ui->plainTextEditnotes->setPlainText(Notesitem->text());
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));

}

void PlaneLog::messagesliderchsnged(int value)
{
    if(ui->textEditmessages->verticalScrollBar()->maximum()>0)wantedscrollprop=((double)value)/((double)ui->textEditmessages->verticalScrollBar()->maximum());
}

void PlaneLog::on_toolButtonimg_clicked()
{

    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();

    QTableWidgetItem *AESitem = ui->tableWidget->item(row,TableWidgetColumn::Number::AES);
    QTableWidgetItem *REGitem = ui->tableWidget->item(row,TableWidgetColumn::Number::REG);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(AESitem->text());

    //if found {REG} is database reg else we guess what it should be from the txed reg that maybe different

    //db reg number
    QString regstr;
    QString str=ui->plainTextEditdatabase->toPlainText();
    QStringList list=str.split('\n');
    if(list.size()>3)
    {
        QRegularExpression rx("Reg. ID([\\s]*)(.+)");
        QRegularExpressionMatch match = rx.match(list[0]);
        if (match.hasMatch())
            regstr = match.captured(2).toLower();
    }

    /*QString str=ui->plainTextEditdatabase->toPlainText();
    QStringList list=str.split('\n');
    if(list.size()>3)
    {
        QRegularExpression rx("Reg. ID([\\s]*)(.+)");
        if(rx.indexIn(list[0])!=-1)
        {
            QString url="http://www.flightradar24.com/data/airplanes/"+rx.cap(2).toLower();
            QDesktopServices::openUrl(QUrl(url));
            return;
        }
    }*/

    if(regstr.isEmpty())
    {
        regstr=REGitem->text().toLower().trimmed();
        regstr.replace(".","");
        QRegularExpression rx("([a-z_0-9]*)");
        if ((regstr.size() == 7)) {
            QRegularExpressionMatch match = rx.match(regstr);
            if (match.hasMatch() && match.captured(1).size() == 7){
                regstr.insert(2,'-');
            }
        }
    }

    QString url=planelookup;
    url.replace("{AES}",AESitem->text());
    url.replace("{REG}",regstr);
    QDesktopServices::openUrl(QUrl(url));
}

void PlaneLog::contextMenuEvent(QContextMenuEvent *event)
{
    //tablewidget menu
    if(!ui->tableWidget->rect().contains(ui->tableWidget->mapFromGlobal(event->globalPos())))
    {
        event->ignore();
        return;
    }
    QMenu menu(this);
    menu.addAction(ui->actionCopy);
    menu.exec(event->globalPos());
    event->accept();
}

void PlaneLog::on_actionCopy_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();

    if(ui->tableWidget->selectedItems().size())
    {
        clipboard->setText(ui->tableWidget->selectedItems()[0]->text());
    }

    /*QString str;
    foreach(QTableWidgetItem *item,ui->tableWidget->selectedItems())
    {
        str+=(item->text()+"\t");
    }
    str.chop(1);
    clipboard->setText(str);*/
}

void PlaneLog::plainTextEditnotesChanged()
{
    if(!selectedAESitem)return;
    if(selectedAESitem->row()<0)return;
    if(selectedAESitem->row()>=ui->tableWidget->rowCount())return;
    int row=selectedAESitem->row();

    QTableWidgetItem *Notesitem = ui->tableWidget->item(row,TableWidgetColumn::Number::Notes);
    Notesitem->setText(ui->plainTextEditnotes->toPlainText().replace("\u2063",""));
}

void PlaneLog::on_actionExport_log_triggered()
{
    //ask for name
    QSettings settings("Jontisoft", settings_name);
    QString filename=QFileDialog::getSaveFileName(this,tr("Save as"), settings.value("exportimportloc",QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]+"/jaerologwindow.csv").toString(), tr("CSV file (*.csv)"));
    if(filename.isEmpty())return;
    settings.setValue("exportimportloc",filename);

    //open file
    QFile outfile(filename);
    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("Cant open file for saving");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    QTextStream outtext(&outfile);
    // outtext.setCodec("UTF-8");

    //write file
    for(int row=0;row<ui->tableWidget->rowCount();row++)
    {
        QString line="\"";
        for(int column=0;column<ui->tableWidget->columnCount();column++)
        {
            QString cell;
            if(ui->tableWidget->item(row,column))cell=ui->tableWidget->item(row,column)->text();

            //" escape
            cell.replace("\"","\\\"");

            //notes replacement
            if(column==TableWidgetColumn::Number::Notes)
            {
                cell.remove("\r");
                cell.replace("\n","●");
            }

            line+=cell.trimmed()+"\",\"";
        }
        line.chop(2);
        line+="\n";
        outtext<<line;
    }

}

void PlaneLog::on_actionImport_log_triggered()
{

    //ask for name
    QSettings settings("Jontisoft", settings_name);
    QString filename=QFileDialog::getOpenFileName(this,tr("Open file"), settings.value("exportimportloc",QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]+"/jaerologwindow.csv").toString(), tr("CSV file (*.csv)"));
    if(filename.isEmpty())return;
    settings.setValue("exportimportloc",filename);

    //open file
    QFile infile(filename);
    if (!infile.open(QIODevice::ReadOnly| QIODevice::Text))
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("Cant open file for reading");
        msgBox.setWindowTitle("Error");
        msgBox.exec();
        return;
    }
    QTextStream intext(&infile);
    // intext.setCodec("UTF-8");

    //confirm
    if(ui->tableWidget->rowCount())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setWindowIcon(QPixmap(":/images/primary-modem.svg"));
        msgBox.setText("This will replace this window log\nAre you sure?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowTitle("Confirm log replacement");
        switch(msgBox.exec())
        {
        case QMessageBox::No:
            return;
            break;
        case QMessageBox::Yes:
            break;
        default:
            return;
            break;
        }
    }

    //clear everything
    ui->tableWidget->clearContents();
    for(int rows = 0; ui->tableWidget->rowCount(); rows++)ui->tableWidget->removeRow(0);
    ui->toolButtonimg->setIcon(QPixmap(":/images/Plane_clip_art.svg"));
    ui->labelAES->clear();
    ui->labelREG->clear();
    ui->labelfirst->clear();
    ui->labellast->clear();
    ui->label_owner->clear();
    ui->label_type->clear();
    disconnect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->plainTextEditnotes->clear();
    connect(ui->plainTextEditnotes,SIGNAL(textChanged()),this,SLOT(plainTextEditnotesChanged()));
    ui->textEditmessages->clear();
    ui->plainTextEditdatabase->clear();
    selectedAESitem=NULL;

    //look at all rows in the file
    QTableWidgetItem *AESitem;
    QTableWidgetItem *REGitem;
    QTableWidgetItem *FirstHearditem;
    QTableWidgetItem *LastHearditem;
    QTableWidgetItem *Countitem;
    QTableWidgetItem *LastMessageitem;
    QTableWidgetItem *MessageCountitem;
    QTableWidgetItem *Modelitem;
    QTableWidgetItem *Owneritem;
    QTableWidgetItem *Countryitem;
    QTableWidgetItem *Notesitem;
    int ec=0;
    while (!intext.atEnd())
    {

        //read row
        QString line = intext.readLine();
        line.chop(1);
        line.remove(0,1);
        line.replace("\\\"","\x10");
        QStringList items=line.split("\",\"");
        //hack for importing old format
        //if number of cols is 8 then probably the old format
        if(items.size()==8)
        {
            items.insert(7,"");
            items.insert(7,"");
            items.insert(7,"");
        }
        if(items.size()!=TableWidgetColumn::Number::Number_Of_Cols)
        {
            ec++;
            if(ec<20)qDebug()<<"csv col count not right (is "<<items.size()<<" should be"<<TableWidgetColumn::Number::Number_Of_Cols<<" )";
            continue;
        }
        for(int i=0;i<items.size();i++)items[i].replace("\x10","\"");

        //insert row
        int idx=ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(idx);
        QFontMetrics fm(ui->tableWidget->font());
        ui->tableWidget->setRowHeight(idx,fm.height()*wantedheightofrow);
        AESitem = new QTableWidgetItem;
        REGitem = new QTableWidgetItem;
        FirstHearditem = new QTableWidgetItem;
        LastHearditem = new QTableWidgetItem;
        Countitem = new QTableWidgetItem;
        LastMessageitem = new QTableWidgetItem;
        MessageCountitem = new QTableWidgetItem;
        Modelitem = new QTableWidgetItem;
        Owneritem = new QTableWidgetItem;
        Countryitem = new QTableWidgetItem;
        Notesitem = new QTableWidgetItem;
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::AES,AESitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::REG,REGitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::FirstHeard,FirstHearditem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::LastHeard,LastHearditem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Count,Countitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::LastMessage,LastMessageitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::MessageCount,MessageCountitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Model,Modelitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Owner,Owneritem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Country,Countryitem);
        ui->tableWidget->setItem(idx,TableWidgetColumn::Number::Notes,Notesitem);
        AESitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        REGitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        Countitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        FirstHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        LastHearditem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        MessageCountitem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        AESitem->setFlags(AESitem->flags()&~Qt::ItemIsEditable);
        REGitem->setFlags(REGitem->flags()&~Qt::ItemIsEditable);
        Countitem->setFlags(Countitem->flags()&~Qt::ItemIsEditable);
        FirstHearditem->setFlags(FirstHearditem->flags()&~Qt::ItemIsEditable);
        LastHearditem->setFlags(LastHearditem->flags()&~Qt::ItemIsEditable);
        Modelitem->setFlags(Modelitem->flags()&~Qt::ItemIsEditable);
        Owneritem->setFlags(Owneritem->flags()&~Qt::ItemIsEditable);
        Countryitem->setFlags(Countryitem->flags()&~Qt::ItemIsEditable);
        Notesitem->setFlags(Notesitem->flags()&~Qt::ItemIsEditable);
        AESitem->setText(items[TableWidgetColumn::Number::AES]);
        REGitem->setText(items[TableWidgetColumn::Number::REG]);
        FirstHearditem->setText(items[TableWidgetColumn::Number::FirstHeard]);
        LastHearditem->setText(items[TableWidgetColumn::Number::LastHeard]);
        Countitem->setText(items[TableWidgetColumn::Number::Count]);
        LastMessageitem->setText(items[TableWidgetColumn::Number::LastMessage]);
        MessageCountitem->setText(items[TableWidgetColumn::Number::MessageCount]);
        Modelitem->setText(items[TableWidgetColumn::Number::Model]);
        Owneritem->setText(items[TableWidgetColumn::Number::Owner]);
        Countryitem->setText(items[TableWidgetColumn::Number::Country]);
        items[TableWidgetColumn::Number::Notes].replace("●","\n");
        Notesitem->setText(items[TableWidgetColumn::Number::Notes].trimmed());

    }


}
