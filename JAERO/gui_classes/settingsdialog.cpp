#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QDebug>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include "../databasetext.h"

QString settings_name="";

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    populatesettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::poulatepublicvars()
{

    msgdisplayformat=ui->comboBoxDisplayformat->currentText();
    dropnontextmsgs=ui->checkBoxdropnontextmsgs->isChecked();
    donotdisplaysus.clear();
    QRegExp rx("([\\da-fA-F]+)");
    int pos = 0;
    while ((pos = rx.indexIn(ui->lineEditdonotdisplaysus->text(), pos)) != -1)
    {
        bool ok = false;
        uint value = rx.cap(1).toUInt(&ok,16);
        if(ok)donotdisplaysus.push_back(value);
        pos += rx.matchedLength();
    }

    audioinputdevice=QAudioDeviceInfo::defaultInputDevice();
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
    {
        if(deviceInfo.deviceName()==ui->comboBoxsoundcard->currentText())
        {
            audioinputdevice=deviceInfo;
            break;
        }
    }

    loggingdirectory=ui->lineEditlogdir->text();
    loggingenable=ui->checkBoxlogenable->isChecked();
    widebandwidthenable=ui->checkBoxlogwidebandwidthenable->isChecked();

    planesfolder=ui->lineEditplanesfolder->text();
    planelookup=ui->lineEditplanelookup->text();

    beepontextmessage=ui->checkBoxbeepontextmessage->isChecked();

    //bottom text window output settings
    QStringList hosts=ui->lineEditudpoutputdecodedmessagesaddress->text().simplified().split(" ");
    udp_for_decoded_messages_address.clear();
    udp_for_decoded_messages_port.clear();
    for(int i=0;i<hosts.size();i++)
    {
        QString host=hosts[i];
        QHostAddress hostaddress;
        quint16 hostport;
        QString hostaddress_string=host.section(':',0,0).simplified();
        if(hostaddress_string.toUpper()=="LOCALHOST")hostaddress_string="127.0.0.1";
        if(!hostaddress.setAddress(hostaddress_string))
        {
            qDebug()<<"Can't set UDP address";
        }
        hostport=host.section(':',1,1).toInt();
        if(hostport==0)
        {
            qDebug()<<"Can't set UDP port reverting to 18765";
            hostport=18765;
        }
        if(!((udp_for_decoded_messages_port.contains(hostport)&&udp_for_decoded_messages_address.contains(hostaddress))))
        {
            udp_for_decoded_messages_address.push_back(hostaddress);
            udp_for_decoded_messages_port.push_back(hostport);
        }
         else
         {
              qDebug()<<"Can't set UDP address:port as it's already used";
         }
    }
    udp_for_decoded_messages_enabled=ui->checkOutputDecodedMessageToUDPPort->isChecked();


    //ads message output using SBS1 protocol over TCP
    QString hostaddr=ui->lineEdittcpoutputadsmessagesaddress->text().section(':',0,0);
    if(!tcp_for_ads_messages_address.setAddress(hostaddr))
    {
        QString tstr=ui->lineEdittcpoutputadsmessagesaddress->text().section(':',1,1);
        ui->lineEdittcpoutputadsmessagesaddress->setText("0.0.0.0:"+tstr);
        tcp_for_ads_messages_address.clear();
        QSettings settings("Jontisoft", settings_name);
        settings.setValue("lineEdittcpoutputadsmessagesaddress", "0.0.0.0:"+tstr);
        qDebug()<<"Can't set TCP address reverting to 0.0.0.0";
    }
    tcp_for_ads_messages_port=ui->lineEdittcpoutputadsmessagesaddress->text().section(':',1,1).toInt();
    if(tcp_for_ads_messages_port==0)
    {
        qDebug()<<"Can't set TCP port reverting to 30003";
        ui->lineEdittcpoutputadsmessagesaddress->setText(hostaddr+":30003");
        QSettings settings("Jontisoft", settings_name);
        settings.setValue("lineEdittcpoutputadsmessagesaddress", hostaddr+":30003");
        tcp_for_ads_messages_port=30003;
    }
    tcp_for_ads_messages_enabled=ui->checkOutputADSMessageToTCP->isChecked();
    tcp_as_client_enabled=ui->checkTCPAsClient->isChecked();

    ui->checkTCPAsClient->setEnabled(ui->checkOutputADSMessageToTCP->isChecked());

}


void SettingsDialog::populatesettings()
{

    //populate soundcard
    ui->comboBoxsoundcard->clear();
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput))
        ui->comboBoxsoundcard->addItem(deviceInfo.deviceName());

    //load settings
    QSettings settings("Jontisoft", settings_name);
    ui->comboBoxDisplayformat->setCurrentIndex(settings.value("comboBoxDisplayformat",2).toInt());
    ui->lineEditdonotdisplaysus->setText(settings.value("lineEditdonotdisplaysus","26 0A C0 00 14 16").toString());
    ui->checkBoxdropnontextmsgs->setChecked(settings.value("checkBoxdropnontextmsgs",true).toBool());
    ui->comboBoxsoundcard->setCurrentText(settings.value("comboBoxsoundcard","").toString());
    ui->lineEditlogdir->setText(settings.value("lineEditlogdir",QStandardPaths::standardLocations(APPDATALOCATIONS)[0]+"/logs").toString());
    ui->checkBoxlogenable->setChecked(settings.value("checkBoxlogenable",false).toBool());
    ui->checkBoxlogwidebandwidthenable->setChecked(settings.value("checkBoxlogwidebandwidthenable",false).toBool());
    ui->lineEditplanesfolder->setText(settings.value("lineEditplanesfolder",QStandardPaths::standardLocations(APPDATALOCATIONS)[0]+"/planes").toString());
    ui->lineEditplanelookup->setText(settings.value("lineEditplanelookup","http://www.flightradar24.com/data/airplanes/{REG}").toString());
    ui->checkBoxbeepontextmessage->setChecked(settings.value("checkBoxbeepontextmessage",true).toBool());
    ui->lineEditudpoutputdecodedmessagesaddress->setText(settings.value("lineEditudpoutputdecodedmessagesaddress","localhost:18765").toString());
    ui->checkOutputDecodedMessageToUDPPort->setChecked(settings.value("checkOutputDecodedMessageToUDPPort",false).toBool());
    ui->lineEdittcpoutputadsmessagesaddress->setText(settings.value("lineEdittcpoutputadsmessagesaddress","0.0.0.0:30003").toString());
    ui->checkOutputADSMessageToTCP->setChecked(settings.value("checkOutputADSMessageToTCP",false).toBool());
    ui->checkTCPAsClient->setChecked(settings.value("checkTCPAsClient",false).toBool());

//these have been tested so far as lineEditplanelookup
//http://www.flightradar24.com/data/airplanes/{REG}

    on_lineEditlogdir_editingFinished();

    poulatepublicvars();
}

void SettingsDialog::accept()
{    

    //save settings
    QSettings settings("Jontisoft", settings_name);
    settings.setValue("comboBoxDisplayformat", ui->comboBoxDisplayformat->currentIndex());
    settings.setValue("lineEditdonotdisplaysus", ui->lineEditdonotdisplaysus->text());
    settings.setValue("checkBoxdropnontextmsgs", ui->checkBoxdropnontextmsgs->isChecked());
    settings.setValue("comboBoxsoundcard", ui->comboBoxsoundcard->currentText());
    settings.setValue("lineEditlogdir", ui->lineEditlogdir->text());
    settings.setValue("checkBoxlogenable", ui->checkBoxlogenable->isChecked());
    settings.setValue("checkBoxlogwidebandwidthenable", ui->checkBoxlogwidebandwidthenable->isChecked());
    settings.setValue("lineEditplanesfolder", ui->lineEditplanesfolder->text());
    settings.setValue("lineEditplanelookup", ui->lineEditplanelookup->text());
    settings.setValue("checkBoxbeepontextmessage", ui->checkBoxbeepontextmessage->isChecked());
    settings.setValue("lineEditudpoutputdecodedmessagesaddress", ui->lineEditudpoutputdecodedmessagesaddress->text());
    settings.setValue("checkOutputDecodedMessageToUDPPort", ui->checkOutputDecodedMessageToUDPPort->isChecked());  
    settings.setValue("lineEdittcpoutputadsmessagesaddress", ui->lineEdittcpoutputadsmessagesaddress->text());
    settings.setValue("checkOutputADSMessageToTCP", ui->checkOutputADSMessageToTCP->isChecked());
    settings.setValue("checkTCPAsClient", ui->checkTCPAsClient->isChecked());

    poulatepublicvars();
    QDialog::accept();
}


void SettingsDialog::on_lineEditlogdir_editingFinished()
{
    QFile file(ui->lineEditlogdir->text());
    ui->lineEditlogdir->setText(file.fileName());
}

void SettingsDialog::on_lineEditplanesfolder_editingFinished()
{
    QFile file(ui->lineEditplanesfolder->text());
    ui->lineEditplanesfolder->setText(file.fileName());
}

void SettingsDialog::on_checkOutputADSMessageToTCP_stateChanged(int arg1)
{
    if(!arg1)ui->checkTCPAsClient->setEnabled(false);
     else ui->checkTCPAsClient->setEnabled(true);
}
