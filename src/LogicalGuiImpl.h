#pragma once

#include <QFuture>
#include <QThreadPool>
#include <tuple>
#include <exception>
#include <type_traits>
#include <mutex>
#include <functional>

#include "QObjectPrivate.h"

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

template <typename T> struct Identity
{
	typedef T type;
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

class BaseExecutor
{
public:
	virtual ~BaseExecutor()
	{
	}

	virtual void execute(void *receiver, void *ret, void *args) = 0;
};
template <class Obj, typename Ret, typename... Params>
class MemberExecutor : public BaseExecutor
{
public:
	typedef Ret (Obj::*Func)(Params...);

	explicit MemberExecutor(Func func) : m_func(func)
	{
	}

	template <std::size_t... S>
	Ret call(void *receiver, std::tuple<Params...> params, Detail::Sequence<S...>)
	{
		return (reinterpret_cast<Obj *>(receiver)->*m_func)(std::get<S>(params)...);
	}
	void execute(void *receiver, void *ret, void *args) override
	{
		auto tuple = *reinterpret_cast<std::tuple<Params...> *>(args);
		call(receiver, tuple, typename Detail::SequenceGenerator<sizeof...(Params)>::type()),
			QtPrivate::ApplyReturnValue<Ret>(ret);
	}

private:
	Func m_func;
};
template <typename Ret, typename... Params> class FunctorExecutor : public BaseExecutor
{
public:
	typedef std::function<Ret(Params...)> Func;

	explicit FunctorExecutor(Func func) : m_func(func)
	{
	}

	template <std::size_t... S> Ret call(std::tuple<Params...> params, Detail::Sequence<S...>)
	{
		return m_func(std::get<S>(params)...);
	}
	void execute(void *receiver, void *ret, void *args) override
	{
		auto tuple = *reinterpret_cast<std::tuple<Params...> *>(args);
		call(tuple, typename Detail::SequenceGenerator<sizeof...(Params)>::type()),
			QtPrivate::ApplyReturnValue<Ret>(ret);
	}

private:
	Func m_func;
};
template <typename Ret> class FunctorExecutor<Ret, void> : public BaseExecutor
{
public:
	typedef std::function<Ret()> Func;

	explicit FunctorExecutor(Func func) : m_func(func)
	{
	}

	template <std::size_t... S> Ret call()
	{
		m_func();
	}
	void execute(void *receiver, void *ret, void *args) override
	{
		call(), QtPrivate::ApplyReturnValue<Ret>(ret);
	}

private:
	Func m_func;
};

struct Binding
{
	Binding(const QObject *receiver, const QMetaMethod &method)
		: receiver(receiver), method(method)
	{
	}
	Binding(const QObject *receiver, BaseExecutor *executor)
		: receiver(receiver), executor(executor)
	{
	}
	Binding()
	{
	}
	const QObject *receiver;
	QMetaMethod method;
	BaseExecutor *executor = nullptr;
};

struct ExecutionData
{
	BaseExecutor *executor;
	std::exception_ptr *exception;
	void *receiver;
	void *ret;
	void *args;
	std::mutex &mutex;
};

template <typename Func> class Functionize
{
public:
	std::nullptr_t get(Func slot)
	{
	}
};
template <typename Ret, typename... Args> class Functionize<std::function<Ret (*)(Args...)>>
{
	typedef Ret (*Func)(Args...);

public:
	std::function<Func> get(std::function<Func> slot)
	{
		return slot;
	}
};
template <typename Return, typename... Args> class Functionize<Return (*)(Args...)>
{
	typedef Return (*Func)(Args...);

public:
	std::function<Func> get(Func slot)
	{
		return std::function<Func>(slot);
	}
};

template <typename T> struct FunctorTraits
{
};
template <typename R, typename A> struct FunctorTraits<R(A)>
{
	typedef R ReturnType;
	typedef A ArgumentType;
};
template <typename R, typename It, typename A> struct FunctorTraits<R (It::*)(A)>
{
	typedef R ReturnType;
	typedef A ArgumentType;
};
template <typename R, typename A> struct FunctorTraits<std::function<R(A)>>
{
	typedef R ReturnType;
	typedef A ArgumentType;
};
template <typename R, typename It, typename A> struct FunctorTraits<std::function<R (It::*)(A)>>
{
	typedef R ReturnType;
	typedef A ArgumentType;
};
template <typename R, typename It, typename A> struct FunctorTraits<R (It::*)(A) const>
{
	typedef R ReturnType;
	typedef A ArgumentType;
};
template <typename R, typename It> struct FunctorTraits<R (It::*)()>
{
	typedef R ReturnType;
	typedef void ArgumentType;
};
template <typename R, typename It> struct FunctorTraits<R (It::*)() const>
{
	typedef R ReturnType;
	typedef void ArgumentType;
};
template <typename R> struct FunctorTraits<R()>
{
	typedef R ReturnType;
	typedef void ArgumentType;
};
template <typename R, typename It> struct FunctorTraits<std::function<R (It::*)()>>
{
	typedef R ReturnType;
	typedef void ArgumentType;
};
template <typename R> struct FunctorTraits<std::function<R()>>
{
	typedef R ReturnType;
	typedef void ArgumentType;
};

template <typename Func> class FunctorHelper
{
public:
	BaseExecutor *createExecutor(Func slot)
	{
		return new Detail::FunctorExecutor<
			typename Detail::FunctorTraits<decltype(&Func::operator())>::ReturnType,
			typename Detail::FunctorTraits<decltype(&Func::operator())>::ArgumentType>(
			Detail::Functionize<Func>().get(slot));
	}
};
template <typename Func> class FunctorHelper<std::function<Func> &>
{
public:
	BaseExecutor *createExecutor(std::function<Func> &slot)
	{
		return new Detail::FunctorExecutor<typename Detail::FunctorTraits<Func>::ReturnType,
										   typename Detail::FunctorTraits<Func>::ArgumentType>(
			Detail::Functionize<std::function<Func>>().get(slot));
	}
};
}
