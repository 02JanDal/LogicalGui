#pragma once

#include <QFuture>
#include <QThreadPool>
#include <tuple>

class Bindable;

namespace Detail
{
template <std::size_t... a> struct Sequence
{
};
template <std::size_t N, std::size_t... S>
struct SequenceGenerator : SequenceGenerator<N - 1, N - 1, S...>
{
};
template <std::size_t... S> struct SequenceGenerator<0, S...>
{
	typedef Sequence<S...> type;
};

template <typename Ret, typename... Params>
class BaseRequestRunner : public QFutureInterface<Ret>, public QRunnable
{
public:
	explicit BaseRequestRunner(const QString id, Bindable *parent, Params... params)
		: m_id(id), m_parent(parent), m_params(std::make_tuple(params...))
	{
	}

	QFuture<Ret> start()
	{
		this->setRunnable(this);
		// this->setThreadPool(QThreadPool::globalInstance());
		this->reportStarted();
		QFuture<Ret> future = this->future();
		QThreadPool::globalInstance()->start(this);
		return future;
	}

	void run() override
	{
		if (this->isCanceled())
		{
			this->reportFinished();
			return;
		}

		this->reportResult(runFunctor(m_id, m_parent, m_params));
		this->reportFinished();
	}

protected:
	QString m_id;
	Bindable *m_parent;
	std::tuple<Params...> m_params;

	virtual Ret runFunctor(const QString &id, Bindable *parent,
						   std::tuple<Params...> params) = 0;
};

struct Binding
{
	Binding(const QObject *receiver, const QMetaMethod &method)
		: receiver(receiver), method(method)
	{
	}
	Binding(const QObject *receiver, QtPrivate::QSlotObjectBase *object)
		: receiver(receiver), object(object)
	{
	}
	Binding()
	{
	}
	const QObject *receiver;
	QMetaMethod method;
	QtPrivate::QSlotObjectBase *object = nullptr;
};
}
