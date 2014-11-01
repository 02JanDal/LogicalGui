#include "LogicalGui.h"

#include <QCoreApplication>
#include <QSemaphore>
#include <QThread>

#include <mutex>

#include "QObjectPrivate.h"

Bindable::Bindable(Bindable *parent) : m_parent(parent)
{
}

Bindable::~Bindable()
{
}

void Bindable::setBindableParent(Bindable *parent)
{
	m_parent = parent;
}

void Bindable::bind(const QString &id, const QObject *receiver, const char *methodSignature)
{
	auto mo = receiver->metaObject();
	Q_ASSERT_X(mo, "Bindable::bind", "Invalid metaobject. Did you forget the QObject macro?");
	const QMetaMethod method = mo->method(
		mo->indexOfMethod(QMetaObject::normalizedSignature(methodSignature + 1).constData()));
	Q_ASSERT_X(method.isValid(), "Bindable::bind", "Invalid method signature");
	m_bindings.insert(id, Detail::Binding(receiver, method));
}

void Bindable::unbind(const QString &id)
{
	m_bindings.remove(id);
}

Qt::ConnectionType Bindable::connectionType(const QObject *receiver)
{
	return receiver == nullptr ? Qt::DirectConnection
							   : (QThread::currentThread() == receiver->thread()
									  ? Qt::DirectConnection
									  : Qt::BlockingQueuedConnection);
}

void Bindable::callSlotObject(Detail::Binding binding, void *ret, void *args)
{
	std::exception_ptr exception;
	std::mutex mutex;

	auto lambda = [](Detail::ExecutionData data)
	{
		try
		{
			data.executor->execute(data.receiver, data.ret, data.args);
		}
		catch (...)
		{
			*data.exception = std::current_exception();
		}
		data.mutex.unlock();
	};

	Detail::ExecutionData data = Detail::ExecutionData{
		binding.executor, &exception, const_cast<QObject *>(binding.receiver),
		ret,			  args,		  mutex};

	void *arguments[] = {0, &data};

	auto object =
		new QtPrivate::QFunctorSlotObject<decltype(lambda), 1,
										  QtPrivate::List<Detail::ExecutionData>, void>(lambda);
	QMetaCallEvent *ev = new QMetaCallEvent(object, nullptr, -1, 0, 0, arguments);
	if (connectionType(binding.receiver) == Qt::BlockingQueuedConnection)
	{
		data.mutex.lock();
		QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
		data.mutex.lock();
	}
	else
	{
		ev->placeMetaCall(const_cast<QObject *>(binding.receiver));
	}
}

void Bindable::checkParameterCount(const QMetaMethod &method, const int paramCount)
{
	Q_ASSERT_X(method.parameterCount() == paramCount, "Bindable::wait",
			   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
							  .arg(method.parameterCount(), paramCount)));
}

void Bindable::checkReturnType(const QMetaMethod &method, const int typeId)
{
	Q_ASSERT_X(typeId != QMetaType::UnknownType, "Bindable::wait",
			   "Requested return type is not registered, please use the Q_DECLARE_METATYPE "
			   "macro to make it known to Qt's meta-object system");
	Q_ASSERT_X(
		method.returnType() == typeId ||
			QMetaType::hasRegisteredConverterFunction(method.returnType(), typeId),
		"Bindable::wait",
		qPrintable(
			QString("Requested return type (%1) is incompatible method return type (%2)")
				.arg(QMetaType::typeName(typeId), QMetaType::typeName(method.returnType()))));
}
