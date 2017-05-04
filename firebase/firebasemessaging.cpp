#include "firebasemessaging.h"


QPointer<FirebaseMessaging> FirebaseMessaging::m_instance;

FirebaseMessaging::FirebaseMessaging(QObject *parent) : QObject(parent)
{
    qDebug() << "FirebaseMessaging" ;
    if(m_instance.isNull())
    {
        while (FirebaseApp::instance()->getApp() == NULL) {
            qt_noop();
        }
        firebase::messaging::Initialize(*FirebaseApp::instance()->getApp(),this);
        m_instance = this;
    }else{
        QObject::connect(instance(),SIGNAL(fcmTokenChanged(QString)),this,SIGNAL(fcmTokenChanged(QString)));
        QObject::connect(instance(),SIGNAL(messageReceived(QVariantMap)),this,SIGNAL(messageReceived(QVariantMap)));
    }
    qDebug() << "FirebaseMessaging" << FirebaseApp::instance()->getApp()->name() << m_topicFilter;

}

FirebaseMessaging *FirebaseMessaging::instance()
{
    if(!m_instance)
    {
        m_instance = new FirebaseMessaging();
    }
    return m_instance;
}

void FirebaseMessaging::OnTokenReceived(const char *token) {
    //LogMessage("The registration token is `%s`", token);
    if(m_fcmToken != QString(token))
    {
        m_fcmToken =  QString(token);
        qDebug() << "FCM Token:" << m_fcmToken;
        emit fcmTokenChanged(m_fcmToken);
    }else{
        qWarning() << "Token Received multiple times";
    }
    // TODO: If necessary send token to application server.
}

void FirebaseMessaging::OnMessage(const firebase::messaging::Message &message) {
    //LogMessage(TAG, "From: %s", message.from.c_str());
    //LogMessage(TAG, "Message ID: %s", message.message_id.c_str());

    QVariantMap messageMap;
    messageMap.insert("from",QString( message.from.c_str() ));
    messageMap.insert("to",QString( message.to.c_str() ));
    messageMap.insert("message_id", QString( message.message_id.c_str() ));
    messageMap.insert("message_type", QString( message.message_type.c_str() ));
    messageMap.insert("collapse_key",QString( message.collapse_key.c_str() ));
    messageMap.insert("priority", QString( message.priority.c_str() ));

    /**!
     * Parse Data to QVariantMap
     */
    QVariantMap dataMap;
    QJsonDocument datDoc;
    for( auto const &data : message.data )
    {
        datDoc = QJsonDocument::fromJson(static_cast<std::string>(data.second).c_str());
        qDebug() << QString( static_cast<std::string>(data.first).c_str() ) << datDoc.toVariant();
        if(datDoc.isObject() || datDoc.isArray())
        {
            dataMap.insert(QString( static_cast<std::string>(data.first).c_str() ),datDoc.toVariant());
        }else if(QVariant( static_cast<std::string>(data.second).c_str() ).isValid()){
            dataMap.insert(QString( static_cast<std::string>(data.first).c_str() ), QVariant(  static_cast<std::string>(data.second).c_str()  ));
        }
    }

    /**!
     * Parse Notification to QVariantMap
     */
    if( message.notification )
    {
        QVariantMap notificationMap;

        if( !QString(message.notification->title.c_str()).isEmpty() )
        {
            notificationMap.insert("title",QString(message.notification->title.c_str()));
        }

        if( !QString(message.notification->body.c_str()).isEmpty() )
        {
            notificationMap.insert("body",QString(message.notification->body.c_str()));
        }

        messageMap.insert("notification",notificationMap);
    }

    messageMap.insert("data",dataMap);
    emit messageReceived(messageMap);
}

QString FirebaseMessaging::fcmToken()
{
    return m_fcmToken;
}

void FirebaseMessaging::subscribe(QString topic)
{
    qDebug() << "Subscribing to " << topic;
    firebase::messaging::Subscribe( topic.toStdString().c_str() );
}

QStringList FirebaseMessaging::topicFilter() const
{
    return m_topicFilter;
}

void FirebaseMessaging::setTopicFilter(QStringList topicFilter)
{
    qDebug() << "Setting topic filter" << topicFilter;
    if (m_topicFilter == topicFilter)
        return;
    if(!topicFilter.isEmpty())
    {
        disconnect(instance(),0,this,0);
        QObject::connect(instance(),SIGNAL(messageReceived(QVariantMap)),this,SLOT(messageReceivedFilter(QVariantMap)));
    }else{
        QObject::connect(instance(),SIGNAL(messageReceived(QVariantMap)),this,SIGNAL(messageReceived(QVariantMap)));
    }
    m_topicFilter = topicFilter;

    emit topicFilterChanged(topicFilter);
}

void FirebaseMessaging::messageReceivedFilter(QVariantMap message){
    qDebug() << "Receive to filter" << message.value("from");
    QString topicFrom = message.value("from").toString();
    if(topicFrom.contains("/topics/"))
    {
        topicFrom = topicFrom.split("/").last();

    }else{
        return;
    }

    qDebug() << "Filtering for topic" << topicFrom;
    if(!m_topicFilter.contains(topicFrom))
    {
        return;
    }

    emit messageReceived(message);
}
