#include <QGuiApplication>
#include <QtQml>
#include <QQmlComponent>

#include "qffuture.h"
#include "quickfuture.h"

Q_DECLARE_METATYPE(QFuture<QString>)
Q_DECLARE_METATYPE(QFuture<int>)
Q_DECLARE_METATYPE(QFuture<void>)
Q_DECLARE_METATYPE(QFuture<bool>)
Q_DECLARE_METATYPE(QFuture<qreal>)
Q_DECLARE_METATYPE(QFuture<QByteArray>)
Q_DECLARE_METATYPE(QFuture<QVariant>)
Q_DECLARE_METATYPE(QFuture<QVariantMap>)
Q_DECLARE_METATYPE(QFuture<QSize>)

namespace {
	QMap<int, QuickFuture::VariantWrapperBase*> &typeRegister() {
		static QMap<int, QuickFuture::VariantWrapperBase*> wrappers;
		return wrappers;
	}
}

namespace QuickFuture {

static int typeId(const QVariant& v) {
    return v.userType();
}

Future::Future(QObject *parent) : QObject(parent)
{
}

void Future::registerType(int typeId, VariantWrapperBase* wrapper)
{
    auto &reg = typeRegister();
    if (reg.contains(typeId)) {
        qWarning() << QString("QuickFuture::registerType:It is already registered:%1").arg(QMetaType::typeName(typeId));
        return;
    }

    reg[typeId] = wrapper;
}

QJSEngine *Future::engine() const
{
    return m_engine;
}

void Future::setEngine(QQmlEngine *engine)
{
    m_engine = engine;
#ifdef QUICK_FUTURE_PROMISE_SUPPORT
    if (m_engine.isNull()) {
        return;
    }

    QString qml = "import QtQuick 2.0\n"
                  "import QuickPromise 1.0\n"
                  "import QuickFuture 1.0\n"
                  "QtObject { \n"
                  "function create(future) {\n"
                  "    var promise = Q.promise();\n"
                  "    Future.onFinished(future, function(value) {\n"
                  "        if (Future.isCanceled(future)) {\n"
                  "            promise.reject();\n"
                  "        } else {\n"
                  "            promise.resolve(value);\n"
                  "        }\n"
                  "    });\n"
                  "    return promise;\n"
                  "}\n"
                  "}\n";

    QQmlComponent comp (engine);
    comp.setData(qml.toUtf8(), QUrl());
    QObject* holder = comp.create();
    if (holder == 0) {
        return;
    }

    promiseCreator = engine->newQObject(holder);
#endif
}

bool Future::isFinished(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->isFinished(future);
}

bool Future::isRunning(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->isRunning(future);
}

bool Future::isCanceled(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future.isCanceled: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->isCanceled(future);
}

int Future::progressValue(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future.progressValue: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->progressValue(future);
}

int Future::progressMinimum(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future.progressMinimum: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->progressMinimum(future);
}

int Future::progressMaximum(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future.progressMaximum: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return false;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->progressMaximum(future);
}

void Future::onFinished(const QVariant &future, QJSValue func, QJSValue owner)
{
    Q_UNUSED(owner);
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return;
    }
    VariantWrapperBase* wrapper = reg[typeId(future)];
    wrapper->onFinished(m_engine, future, func, owner.toQObject());
}

void Future::onCanceled(const QVariant &future, QJSValue func, QJSValue owner)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(static_cast<int>(future.type())));
        return;
    }
    VariantWrapperBase* wrapper = reg[typeId(future)];
    wrapper->onCanceled(m_engine, future, func, owner.toQObject());
}

void Future::onProgressValueChanged(const QVariant &future, QJSValue func)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future.onProgressValueChanged: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return;
    }
    VariantWrapperBase* wrapper = reg[typeId(future)];
    wrapper->onProgressValueChanged(m_engine, future, func);
}

QVariant Future::result(const QVariant &future)
{
    auto &reg = typeRegister();
    QVariant res;
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return res;
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->result(future);
}

QVariant Future::results(const QVariant &future)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return QVariant();
    }

    VariantWrapperBase* wrapper = reg[typeId(future)];
    return wrapper->results(future);
}

#ifdef QUICK_FUTURE_PROMISE_SUPPORT
QJSValue Future::promise(QJSValue future)
{
    QJSValue create = promiseCreator.property("create");
    QJSValueList args;
    args << future;

    QJSValue result = create.call(args);
    if (result.isError() || result.isUndefined()) {
        qWarning() << "Future.promise: QuickPromise is not installed or setup properly";
        result = QJSValue();
    }

    return result;
}
#endif

void Future::sync(const QVariant &future, const QString &propertyInFuture, QObject *target, const QString &propertyInTarget)
{
    auto &reg = typeRegister();
    if (!reg.contains(typeId(future))) {
        qWarning() << QString("Future: Can not handle input data type: %1").arg(QMetaType::typeName(future.type()));
        return;
    }


    VariantWrapperBase* wrapper = reg[typeId(future)];
    wrapper->sync(future, propertyInFuture, target, propertyInTarget);
}

static QObject *provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
    Q_UNUSED(scriptEngine);

    Future* object = new Future();
    object->setEngine(engine);

    return object;
}

static void init() {
    bool called = false;
    if (called) {
        return;
    }
    called = true;

    QCoreApplication* app = QCoreApplication::instance();
    QObject* tmp = new QObject(app);

    QObject::connect(tmp,&QObject::destroyed,[=]() {
        auto &reg = typeRegister();
        auto iter = reg.begin();
        while (iter != reg.end()) {
            delete iter.value();
            iter++;
        }
    });

    qmlRegisterSingletonType<Future>("QuickFuture", 1, 0, "Future", provider);

    Future::registerType<QString>();
    Future::registerType<int>();
    Future::registerType<void>();
    Future::registerType<bool>();
    Future::registerType<qreal>();
    Future::registerType<QByteArray>();
    Future::registerType<QVariant>();
    Future::registerType<QVariantMap>();
    Future::registerType<QSize>();
}

#ifndef QUICK_FUTURE_BUILD_PLUGIN
Q_COREAPP_STARTUP_FUNCTION(init)
#endif
} // End of namespace

#ifdef QUICK_FUTURE_BUILD_PLUGIN
void QuickFutureQmlPlugin::registerTypes(const char *uri) {
    Q_ASSERT(QString("QuickFuture") == uri);
    QuickFuture::init();
}
#endif
