#include "mqttsubscriber.h"
#include <QCoreApplication>
#include <QDebug>

void MqttSubscriber::updateState(bool subscriptionState)
{
    //what we emitted last time
    QMqttClient::ConnectionState org_state=m_lastClientConnectionState;
    if((m_lastClientConnectionState==QMqttClient::ClientState::STATE_CONNECTED)&&(m_lastSubscriptionState))
    {
        org_state=(QMqttClient::ClientState::ConnectionState)MqttSubscriber::STATE_CONNECTED_SUBSCRIBED;
    }

    //update state
    QMqttClient::ConnectionState state=QMqttClient::ClientState::STATE_DISCONNECTED;
    if(client)state=client->state();
    m_lastSubscriptionState=subscriptionState;
    m_lastClientConnectionState=state;

    //what we will emit this time if changed
    if((state==QMqttClient::ClientState::STATE_CONNECTED)&&(subscriptionState))
    {
        state=(QMqttClient::ClientState::ConnectionState)MqttSubscriber::STATE_CONNECTED_SUBSCRIBED;
    }

    //emit if changed
    if(state!=org_state)
    {
        emit connectionStateChange((MqttSubscriber::ConnectionState)state);
    }

}

void MqttSubscriber::updateState()
{
    updateState(m_lastSubscriptionState);
}

MqttSubscriber::MqttSubscriber(QObject* parent) : QObject(parent),
    client(nullptr),
    messageId(0),
    m_lastSubscriptionState(false),
    m_lastClientConnectionState(QMqttClient::STATE_DISCONNECTED)
{

}

MqttSubscriber::~MqttSubscriber()
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::~MqttSubscriber";
#endif
}

void MqttSubscriber::onSslErrors(const QList<QSslError>& errors)
{
    Q_UNUSED(errors)
    updateState();
    // Optionally, set ssl errors you want to ignore. Be careful, because this may weaken security.
    // See QSslSocket::ignoreSslErrors(const QList<QSslError> &) for more information.

    // Investigate the errors here, if you find no serious problems, call ignoreSslErrors()
    // to continue connecting.
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<errors;
#endif
    if(!client)return;
    client->ignoreSslErrors();
}

void MqttSubscriber::connectToHost(const MqttSubscriber_Settings_Object &settings)
{
    setSettings(settings);
    connectToHost();
}

void MqttSubscriber::disconnectFromHost()
{
    if(!client)return;
    client->disconnectFromHost();
}

void MqttSubscriber::delay(int delay_ms)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(delay_ms);
    loop.exec();
}

void MqttSubscriber::connectToHost()
{

#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::connectToHost";
#endif

    //delete old client
    if(client)
    {
        client->disconnectFromHost();
        client->deleteLater();
    }

    //create a new client with us as the parent
    if(settings.encryption)
    {
        client=new QMqttClient::Client(settings.host,settings.port,QSslConfiguration::defaultConfiguration(),false,this);
    }
    else
    {
        client=new QMqttClient::Client(QHostAddress(settings.host),settings.port,this);
    }

    //add it to the list
    //not needed but oh well
    client_list.append(client);

    //add the connections
    connect(client, &QMqttClient::Client::connected, this, &MqttSubscriber::onConnected);
    connect(client, &QMqttClient::Client::subscribed, this, &MqttSubscriber::onSubscribed);
    connect(client, &QMqttClient::Client::received, this, &MqttSubscriber::onReceived);
    connect(client, &QMqttClient::Client::sslErrors, this, &MqttSubscriber::onSslErrors);
    connect(client, &QMqttClient::Client::disconnected, this, &MqttSubscriber::onDisconnected);
    connect(client, &QMqttClient::Client::destroyed, this, &MqttSubscriber::onClientDestroyed);
    connect(client, &QMqttClient::Client::error, this, &MqttSubscriber::onError);
    connect(client, &QMqttClient::Client::unsubscribed, this, &MqttSubscriber::onUnsubscribed);

    client->setClientId(settings.clientId);
    client->setUsername(settings.username);
    client->setPassword(settings.password);
    client->setCleanSession(true);
    client->setAutoReconnect(true);

    messageId=0;

    client->connectToHost();

    updateState(false);

}

void MqttSubscriber::onClientDestroyed(QObject *client)
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onClientDestroyed: client index ="<<client_list.indexOf((QMqttClient::Client*)client);
    if(client_list.indexOf((QMqttClient::Client*)client)<0)
    {
        qDebug()<<"MqttSubscriber::onClientDestroyed: error can't find client in list";
    }
#endif
    client_list.removeAll((QMqttClient::Client*)client);
    updateState(false);
}

void MqttSubscriber::onDisconnected()
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onDisconnected";
#endif
    updateState(false);
}

void MqttSubscriber::setSettings(const MqttSubscriber_Settings_Object &settings)
{
    if(this->settings!=settings)
    {
#ifdef QMqttClient_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::setSettings: settings have changed";
#endif
        this->settings=settings;
        if((client)&&(client->state()!=QMqttClient::STATE_DISCONNECTED))
        {
#ifdef QMqttClient_DEBUG_SUBSCRIBER
            qDebug()<<"MqttSubscriber::setSettings: reconnecting to host with new settings";
#endif
            connectToHost();
        }
    }
}

void MqttSubscriber::onConnected()
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onConnected";
#endif
    updateState(false);
    onSubscribeTimeout();
}

void MqttSubscriber::onSubscribed(const QString& topic)
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onSubscribed:"<<topic;
#endif
    if((settings.topic==topic)&&(settings.subscribe))
    {
        updateState(true);
#ifdef QMqttClient_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::onSubscribed: subscribed to wanted topic";
#endif
    }
}

void MqttSubscriber::onError(const QMqttClient::ClientError error)
{
    Q_UNUSED(error)
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onError"<<error;
#endif
    updateState();
}

void MqttSubscriber::onUnsubscribed(const QString& topic)
{
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onUnsubscribed";
#endif
    if((settings.topic==topic)&&(settings.subscribe))
    {
        updateState(false);
    }
}

void MqttSubscriber::onSubscribeTimeout()
{
    if(
            (client)&&
            (settings.subscribe)&&
            (!m_lastSubscriptionState)&&
            ((client->state()==QMqttClient::STATE_CONNECTED))
      )
    {
#ifdef QMqttClient_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::onSubscribeTimeout: tring to subscribe to"<<settings.topic;
#endif
        client->subscribe(settings.topic, 0);
        QTimer::singleShot(QMQTT_SUBSCRIBE_TIME_IN_MS,this,SLOT(onSubscribeTimeout()));
    }
}

void MqttSubscriber::onReceived(const QMqttClient::Message& message)
{
//#ifdef QMqttClient_DEBUG_SUBSCRIBER
//    qDebug()<<"MqttSubscriber::onReceived: receiving a message";
//#endif
    if(!settings.subscribe)return;
    QByteArray ba=qUncompress(message.payload());
    if(!aco.fromQByteArray(ba))
    {
#ifdef QMqttClient_DEBUG_SUBSCRIBER
        qDebug()<<"received message looks bad. ditching";
#endif
        qDebug()<<message.payload();
        return;
    }
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    else qDebug()<<"received good message";
#endif
    emit ACARSsignal(aco.toACARSItem());
}

void MqttSubscriber::ACARSslot(ACARSItem &acarsitem)
{
    if(!settings.publish)return;
#ifdef QMqttClient_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::ACARSslot: sending a message";
#endif
    aco=acarsitem;
    QByteArray ba=qCompress(aco,9);
    QMqttClient::Message message(messageId, settings.topic,ba);
    client->publish(message);
    messageId++;
}
