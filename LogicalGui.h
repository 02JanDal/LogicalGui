#pragma once

#include <QObject>
#include <QVariant>
#include <QReadWriteLock>
#include <functional>
#include <memory>

class AbstractWatcher
{
public:
	virtual ~AbstractWatcher()
	{
	}

	virtual QVariant call(const QVariantList &arguments) = 0;
};
class LambdaWatcher : public AbstractWatcher
{
	std::function<QVariant(QVariantList)> m_lambda;
public:
	LambdaWatcher(std::function<QVariant(QVariantList)> lambda) : m_lambda(lambda)
	{
	}
	QVariant call(const QVariantList &arguments) override
	{
		return m_lambda(arguments);
	}
};

namespace Internal
{
class LogicalGuiResult : public QObject
{
	Q_OBJECT
public:
	using QObject::QObject;

	QVariant result;
	bool doStartEventLoop = true;

signals:
	void done();
};

QVariant wait(const QString &id, const QVariantList &arguments);

template<typename... Args>
QList<QVariant> parameterPackToList(const Args&... args)
{
	return QList<QVariant>({QVariant::fromValue(args)...});
}
}

class LogicalGui : public QObject
{
	Q_OBJECT
	LogicalGui();
public:
	static LogicalGui *instance();

	void registerWatcher(const QString &id, AbstractWatcher *watcher);
	void unregisterWatcher(const QString &id);

	Internal::LogicalGuiResult *call(const QString &id, const QVariantList &args, const bool direct);

signals:
	void callSignal(const QVariantList &args, AbstractWatcher *watcher, Internal::LogicalGuiResult *result);

private slots:
	void callInternal(const QVariantList &args, AbstractWatcher *watcher, Internal::LogicalGuiResult *result);

private:
	QReadWriteLock m_lock;
	QHash<QString, AbstractWatcher *> m_watchers;
};

template<typename Ret, typename... Params>
Ret wait(const QString &id, Params... params)
{
	const QVariant res = Internal::wait(id, Internal::parameterPackToList(params...));
	return res.value<Ret>();
}
template<typename Ret>
Ret wait(const QString &id, const QVariantList &arguments)
{
	return Internal::wait(id, arguments).value<Ret>();
}
