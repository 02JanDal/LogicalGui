#include "LogicalGui.h"

#include <QThread>
#include <QCoreApplication>
#include <QEventLoop>
#include <QReadWriteLock>

LogicalGui::LogicalGui()
{
	connect(this, &LogicalGui::callSignal, this, &LogicalGui::callInternal);
}
LogicalGui *LogicalGui::instance()
{
	static LogicalGui logicalgui;
	if (logicalgui.thread() != qApp->thread())
	{
		logicalgui.moveToThread(qApp->thread());
	}
	return &logicalgui;
}

void LogicalGui::registerWatcher(const QString &id, AbstractWatcher *watcher)
{
	QWriteLocker locker(&m_lock);
	if (m_watchers.contains(id))
	{
		qWarning("LogicalGui::registerWatcher: ID already used. Overwritting.");
	}
	m_watchers.insert(id, watcher);
}
void LogicalGui::unregisterWatcher(const QString &id)
{
	QWriteLocker locker(&m_lock);
	if (!m_watchers.contains(id))
	{
		qWarning("LogicalGui::unregisterWatcher: ID not registered");
	}
	else
	{
		m_watchers.remove(id);
	}
}

Internal::LogicalGuiResult *LogicalGui::call(const QString &id, const QVariantList &args, const bool direct)
{
	QReadLocker locker(&m_lock);
	Internal::LogicalGuiResult *result = new Internal::LogicalGuiResult;
	if (!m_watchers.contains(id))
	{
		qWarning("LogicalGui::call: Attempt to call unknown watcher \"%s\"", qPrintable(id));
		result->doStartEventLoop = false;
	}
	else
	{
		if (direct)
		{
			callInternal(args, m_watchers[id], result);
		}
		else
		{
			emit callSignal(args, m_watchers[id], result);
		}
	}
	return result;
}
void LogicalGui::callInternal(const QVariantList &args, AbstractWatcher *watcher, Internal::LogicalGuiResult *result)
{
	Q_ASSERT(QThread::currentThread() == qApp->thread());
	try
	{
		result->result = watcher->call(args);
	}
	catch (std::exception &e)
	{
#if !defined(QT_DEBUG)
		qWarning("LogicalGui::callInternal: Uncaught exception: %s", e.what());
#else
		throw e; // rethrow
#endif
	}
	emit result->done();
}

QVariant Internal::wait(const QString &id, const QVariantList &arguments)
{
	Internal::LogicalGuiResult *result;
	if (QThread::currentThread() == qApp->thread())
	{
		// we're in the main thread, so no need to create an event loop
		result = LogicalGui::instance()->call(id, arguments, true);
	}
	else
	{
		result = LogicalGui::instance()->call(id, arguments, false);
		if (result->doStartEventLoop)
		{
			QEventLoop loop;
			QObject::connect(result, &Internal::LogicalGuiResult::done,
							 &loop, &QEventLoop::quit);
			loop.exec();
		}
	}
	return result->result;
}
