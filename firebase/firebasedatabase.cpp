#include "firebasedatabase.h"

FirebaseDatabase::FirebaseDatabase(QObject *parent) : QObject(parent)
{
//    qDebug() << "Creating FirebaseDatabaseInstance";
//    qDebug() << "FirebaseBase App" << FirebaseApp::instance()->getApp();
    if( FirebaseApp::instance()->getApp() )
    {
//        qDebug() << "Before GetInstance";
        firebase::App *currentApp = FirebaseApp::instance()->getApp();
        m_database = firebase::database::Database::GetInstance(currentApp);
//        qDebug() << "Database Instance" << m_database;
        m_database->set_persistence_enabled(true);
    }
}

void FirebaseDatabase::OnValueChanged(const firebase::database::DataSnapshot &snapshot) {
    firebase::Variant * val = new firebase::Variant(snapshot.value());
    qDebug() << "Value Changed" << VariantToQVariant( val );
    setValue(VariantToQVariant( val ), false);

    if( valueObj()->objectName().isEmpty() )
    {
        valueObj()->setObjectName(QString::fromStdString(snapshot.key_string()));
    }

    //emit valueObjChanged(m_valueObj);
}

void FirebaseDatabase::OnChildAdded(const firebase::database::DataSnapshot &snapshot, const char *previous_sibling)
{

}

void FirebaseDatabase::OnChildChanged(const firebase::database::DataSnapshot &snapshot, const char *previous_sibling)
{
    qDebug() << "Child Changed";
}

void FirebaseDatabase::OnChildMoved(const firebase::database::DataSnapshot &snapshot, const char *previous_sibling)
{

}

void FirebaseDatabase::OnChildRemoved(const firebase::database::DataSnapshot &snapshot)
{

}

void FirebaseDatabase::OnCancelled(const firebase::database::Error &error_code, const char *error_message)
{

}

QUrl FirebaseDatabase::basePath() const
{
    return m_basePath;
}

QQmlComponent *FirebaseDatabase::baseComponent() const
{
    return m_baseComponent;
}

QVariant FirebaseDatabase::VariantToQVariant(firebase::Variant *val)
{
    QVariant retVal;
    //qDebug() << firebase::Variant::TypeName(val->type());
    switch (val->type()) {
    case firebase::Variant::kTypeBool:
        retVal = QVariant(val->bool_value());
        break;
    case firebase::Variant::kTypeInt64:
        retVal = QVariant(val->int64_value());
        break;
    case firebase::Variant::kTypeDouble:
        retVal = QVariant(val->double_value());
        break;
    case firebase::Variant::kTypeMutableString:
        retVal = QVariant( QString( val->mutable_string().c_str() ));
        break;
    case firebase::Variant::kTypeMap:
        {
            QMap<QString,QVariant> varMap;
            for(auto const& mpVal : val->map() )
            {
                firebase::Variant keyVariant = firebase::Variant(mpVal.first);
                firebase::Variant valueVariant = firebase::Variant(mpVal.second);
                varMap.insert(VariantToQVariant( &keyVariant ).toString(),VariantToQVariant(&valueVariant));
            }
            retVal = QVariant(varMap);
            break;
        }
    case firebase::Variant::kTypeVector:
    {
        QList<QVariant> varList;
        for(auto const& mpVal : val->vector() )
        {
            firebase::Variant valueVariant = firebase::Variant(mpVal);
            varList.append(VariantToQVariant( &valueVariant) );
        }
        retVal = QVariant( varList );
        break;
    }
    case firebase::Variant::kTypeNull:
        retVal = QVariant();

    default:
        break;
    }
    return retVal;
}

firebase::Variant FirebaseDatabase::QVariantToVariant(QVariant val)
{
    firebase::Variant retVal = firebase::Variant();
    qDebug() << "Parsing QVariant " << val.typeName();
    switch( val.type() )
    {
    case QMetaType::Bool:
        retVal == firebase::Variant::FromBool(val.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::Long:
        retVal = firebase::Variant(val.toInt());
        break;
    case QMetaType::Double:
        retVal = firebase::Variant(val.toDouble());
        break;
    case QMetaType::Float:
        retVal = firebase::Variant(val.toFloat());
        break;
    case QMetaType::QString:
        retVal = firebase::Variant(val.toString().toStdString());
        break;

    case QMetaType::Void:
    case QMetaType::VoidStar:
    default:
            qWarning() << "QVariant to Variant might have failed";
        break;
    }

    return retVal;
}

QObject *FirebaseDatabase::valueObj()
{
    qDebug() << "Reading ValueObj" <<(m_valueObj == NULL) << m_baseComponent;
    if(m_valueObj)
    {
        qDebug() << "OB Value" << m_valueObj << m_valueObj->property("path");
    }
    //m_baseComponent->dumpObjectInfo();
    if( m_valueObj == NULL && m_baseComponent != NULL)
    {
        qDebug() << "Creating ValueObject from Component";
        m_valueObj = m_baseComponent->create();
        m_baseComponent->completeCreate();;
        qDebug() << "Reading Value" << m_valueObj->setProperty("path",QString(m_basePath.path().remove(0,1)));
        qDebug() << m_baseComponent->status();
        qDebug() << m_valueObj->dynamicPropertyNames();
        qDebug() << "OB Value" << m_valueObj << m_valueObj->property("path") << m_valueObj->property("kind");
        m_valueObj->dumpObjectInfo();
        m_valueObj->dumpObjectTree();

    }else{
        return new QObject();
    }
    return m_valueObj;
}

QVariant FirebaseDatabase::value() const
{
    return m_value;
}

void FirebaseDatabase::setBasePath(QUrl basePath)
{
    if (m_basePath == basePath)
        return;

    m_basePath = basePath;
    qDebug() << "Base Path" << basePath.path().remove(0,1);
    if( m_database )
    {
        m_database_ref = new firebase::database::DatabaseReference( m_database->GetReference(basePath.path().remove(0,1).toStdString().c_str()) );
        m_database_ref->AddChildListener(this);
        m_database_ref->AddValueListener(this);
        m_database_ref->GetValue();
        m_database_ref->SetKeepSynchronized(true);

        qDebug() <<"Database Reference URL"<< m_database_ref->url().c_str();
    }
    emit basePathChanged(basePath);
}

void FirebaseDatabase::setBaseComponent(QQmlComponent *baseComponent)
{
    if (m_baseComponent == baseComponent)
        return;

    m_baseComponent = baseComponent;
    emit valueObjChanged(valueObj());
    emit baseComponentChanged(baseComponent);
}

void FirebaseDatabase::setValue(QVariant value, bool write)
{
    qDebug() << "Set Value " << value << write;

    if (m_value == value)
        return;

    m_value = value;
    if(m_write && write)
        m_database_ref->SetValue(QVariantToVariant(value));
    else
    {
        firebase::Variant valVal = QVariantToVariant(value);
        qDebug() << VariantToQVariant(&valVal);
    }
    emit valueChanged(value);
}
