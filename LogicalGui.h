#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>

class Bindable
{
protected:
	struct Binding
	{
		QObject *receiver;
		QByteArray method;
		Bindable *source = 0;
	};

	QMap<QString, Binding> m_bindings;

public:
	virtual ~Bindable()
	{
	}

	void bind(Bindable *bindable)
	{
		for (auto it = bindable->m_bindings.constBegin(); it != bindable->m_bindings.constEnd();
			 ++it)
		{
			Binding binding;
			binding.receiver = it.value().receiver;
			binding.method = it.value().method;
			binding.source = bindable;
			m_bindings.insert(it.key(), binding);
		}
	}
	void unbind(Bindable *bindable)
	{
		for (const auto key : bindable->m_bindings.keys())
		{
			if (m_bindings.contains(key) && m_bindings[key].source == bindable)
			{
				m_bindings.remove(key);
			}
		}
	}

	void bind(const QString &id, QObject *receiver, const char *method)
	{
		Binding binding;
		binding.receiver = receiver;
		binding.method =
			QByteArray(method + 1); // we increase it by one to remove the leading number
		m_bindings.insert(id, binding);
	}
	void unbind(const QString &id)
	{
		m_bindings.remove(id);
	}
};

class Task : public QObject, public Bindable
{
	Q_OBJECT
public:
	Task(Task *parent = 0) : QObject(parent)
	{
		// inherit the bindings from parent
		bind(parent);
	}
	Task(QObject *parent = 0) : QObject(parent)
	{
	}
	virtual ~Task()
	{
	}

protected:
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		Q_ASSERT(m_bindings.contains(id));
		qMetaTypeId<Ret>();
		QVariantList({qMetaTypeId<Params>()...});
		const auto binding = m_bindings[id];
		Ret ret;
		const auto mo = binding.receiver->metaObject();
		QMetaMethod method = mo->method(mo->indexOfMethod(binding.method));
		const Qt::ConnectionType type = QThread::currentThread() == qApp->thread()
											? Qt::DirectConnection
											: Qt::BlockingQueuedConnection;
		const auto retArg =
			QReturnArgument<Ret>(QMetaType::typeName(qMetaTypeId<Ret>()),
								 ret); // because Q_RETURN_ARG doesn't work with templates...
		method.invoke(binding.receiver, type, retArg, Q_ARG(Params, params)...);
		return ret;
	}
};
