/* Copyright 2014 Jan Dalheimer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QFutureInterface>
#include <tuple>

#include "LogicalGuiImpl.h"

/**
 * @class Bindable
 * @brief Inherit from Bindable in a logic class to be able to call GUI code from it
 *
 * @par Terminology
 *
 * * Callback - A QObject slot, member function, lambda, static member function, functor, global
 *function etc.
 * * Callback ID - A string identifying a callback. Used by @ref wait and @ref request to
 *look-up callbacks set with @ref bind
 * * Binding - A mapping between a callback ID and a callback. Set using @ref bind and unset
 *using @ref unbind
 * * Bindable - A container of bindings, which can be called by inheriting from Bindable
 *
 * @par Usage
 *
 * In order to use Bindable, follow these steps:
 *
 * 1. Let your class with logic code inherit from Bindable
 * @code
 * class MyClass : public Bindable
 * @endcode
 * 2. Inside @a MyClass, use @ref wait and @ref request
 * @code
 * bool res = wait<bool>("Continue?", "Do you want to continue?");
 * @endcode
 * 3. When creating an instance of @a MyClass, use one of the @ref bind forms
 * @code
 * MyClass *obj = new MyClass;
 * obj->bind("Continue?", [](const QString &question) {
 *     // display the question to the user, return true or false depending on what the user
 *answered
 * });
 * @endcode
 * 4. You can also create a standalone @ref Bindable object, and use it as a binding container
 * @code
 * Bindable *container = new Bindable;
 * container->bind("Continue?", [](const QString &question) {
 *     // display the question to the user, return true or false depending on what the user
 *answered
 * });
 *
 * MyClass *obj = new MyClass;
 * obj->setBindableParent(container);
 * @endcode
 * You could also create a constructor for MyClass that takes a Bindable *, and then pass that
 *to the Bindable::Bindable constructor
 *
 * @par Unit Testing
 *
 * LogicalGui is also useful for unit testing. Just bind callback IDs to placeholder callbacks,
 *that for example return test data.
 * @code
 * MyClass *classUnderTest = new MyClass;
 * classUnderTest->bind("Continue?", [](QString) { return true; });
 * @endcode
 *
 */
class Bindable
{
	friend class tst_LogicalGui;

	template <typename Ret, typename... Params>
	class RequestRunner : public Detail::BaseRequestRunner<Ret, Params...>
	{
	public:
		using Detail::BaseRequestRunner<Ret, Params...>::BaseRequestRunner;

	private:
		template <std::size_t... S>
		Ret call(const QString &id, Bindable *parent, std::tuple<Params...> params,
				 Detail::Sequence<S...>)
		{
			return parent->wait<Ret>(id, std::get<S>(params)...);
		}
		Ret runFunctor(const QString &id, Bindable *parent, std::tuple<Params...> params)
		{
			return call(id, parent, params,
						typename Detail::SequenceGenerator<sizeof...(Params)>::type());
		}
	};

public:
	/**
	 * @param parent This instance of Bindable will inherit bindings from it's parent
	 * @see setBindableParent
	 */
	Bindable(Bindable *parent = 0);
	virtual ~Bindable();

	/**
	 * @brief Lets this instance inherit bindings from parent
	 * @param parent This instance of Bindable will inherit bindings from it's parent
	 *
	 * You can still add bindings to the parent after you've called @a setBindableParent, and
	 *they'll be available to this instance
	 */
	void setBindableParent(Bindable *parent);

	/**
	 * @brief Bind an old-style slot (using SLOT(...) syntax) to a callback ID
	 * @param id              The callback ID, as will be given to @ref wait or @ref request
	 * @param receiver        The QObject instance on which the callback will be called
	 * @param methodSignature The signature of the callback, as given by SLOT(...)
	 */
	void bind(const QString &id, const QObject *receiver, const char *methodSignature);

#ifdef DOXYGEN
	/**
	 * @brief Bind a member function to a callback ID
	 * @param id       The callback ID, as will be given to @ref wait or @ref request
	 * @param receiver The QObject instance on which the callback will be called
	 * @param slot     The member function that will be called
	 */
	void bind(const QString &id, const QObject *receiver, Func slot);
#else
	template <typename Object, typename ReturnType, typename... Arguments>
	void bind(const QString &id, const Object *receiver,
			  ReturnType (Object::*slot)(Arguments...))
	{
		m_bindings.insert(
			id,
			Detail::Binding(
				receiver, new Detail::MemberExecutor<Object, ReturnType, Arguments...>(slot)));
	}
#endif

#ifdef DOXYGEN
	/**
	 * @brief Bind a lambda, static member, functor or similar to a callback ID
	 * @param id   The callback ID, as will be given to @ref wait and @ref request
	 * @param slot The lambda, static member, functor or similar that will be called
	 */
	void bind(const QString &id, Func slot);
#else
	template <typename Func> void bind(const QString &id, Func &&slot)
	{
		m_bindings.insert(id,
						  Detail::Binding(nullptr,
										  Detail::FunctorHelper<Func>().createExecutor(slot)));
	}

#endif

	/**
	 * @brief Remove the binding with the given ID
	 * @param id The callback ID of the binding to remove
	 */
	void unbind(const QString &id);

private:
	QMap<QString, Detail::Binding> m_bindings;

	Bindable *m_parent;

private:
	Qt::ConnectionType connectionType(const QObject *receiver);
	void callSlotObject(Detail::Binding binding, void *ret, void *args);
	void checkParameterCount(const QMetaMethod &method, const int paramCount);
	void checkReturnType(const QMetaMethod &method, const int typeId);

	template <typename Ret, typename... Params>
	Ret waitInternal(const QString &id, Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			return m_parent->waitInternal<Ret, Params...>(id, params...);
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		Ret ret;
		if (binding.executor)
		{
			std::tuple<Params...> args = std::tuple<Params...>(params...);
			callSlotObject(binding, &ret, &args);
		}
		else
		{
			const QMetaMethod method = binding.method;
			checkParameterCount(method, sizeof...(Params));
			checkReturnType(method, qMetaTypeId<Ret>());
			const auto retArg = QReturnArgument<Ret>(
				QMetaType::typeName(qMetaTypeId<Ret>()),
				ret); // because Q_RETURN_ARG doesn't work with templates...
			method.invoke(const_cast<QObject *>(binding.receiver),
						  connectionType(binding.receiver), retArg, Q_ARG(Params, params)...);
		}
		return ret;
	}
	template <typename... Params> void waitVoidInternal(const QString &id, Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			m_parent->waitVoidInternal<Params...>(id, params...);
			return;
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		if (binding.executor)
		{
			std::tuple<Params...> args = std::tuple<Params...>(params...);
			int ret;
			callSlotObject(binding, &ret, &args);
		}
		else
		{
			const QMetaMethod method = binding.method;
			checkParameterCount(method, sizeof...(Params));
			method.invoke(const_cast<QObject *>(binding.receiver),
						  connectionType(binding.receiver), Q_ARG(Params, params)...);
		}
	}

	template <typename Ret, typename... Params> struct wait_t
	{
		Bindable *bindable;
		explicit wait_t(Bindable *bindable) : bindable(bindable)
		{
		}

		Ret operator()(const QString &id, Params... params)
		{
			return bindable->waitInternal<Ret>(id, params...);
		}
	};
	template <typename... Params> struct wait_t<void, Params...>
	{
		Bindable *bindable;
		explicit wait_t(Bindable *bindable) : bindable(bindable)
		{
		}

		void operator()(const QString &id, Params... params)
		{
			bindable->waitVoidInternal(id, params...);
		}
	};

protected:
#ifdef DOXYGEN
	/**
	 * @brief Calls a callback by it's ID, taking threads etc. into account
	 * @param id  The callback ID to call, as previously bound using @ref bind
	 * @param ... The parameters to pass to the callback
	 * @returns The return value of the callback
	 * @see request
	 */
	template <typename Ret> Ret wait(const QString &id, ...);
#else
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		return wait_t<Ret, Params...>(this)(id, params...);
	}
#endif

#ifdef DOXYGEN
	/**
	 * @brief Creates a QFuture and returns immediately
	 * @warning If the receiver is in the same thread as the caller, this will still be a
	 * blocking request
	 * @param id  The callback ID to call, as previously bound using @ref bind
	 * @param ... The parameters to pass to the callback
	 * @see wait
	 */
	template <typename Ret> QFuture<Ret> request(const QString &id, ...);
#else
	template <typename Ret, typename... Params>
	QFuture<Ret> request(const QString &id, Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			return m_parent->request<Ret, Params...>(id, params...);
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		if (connectionType(binding.receiver) == Qt::DirectConnection)
		{
			QFutureInterface<Ret> *iface = new QFutureInterface<Ret>();
			QFuture<Ret> future = iface->future();
			iface->reportStarted();
			iface->reportResult(wait<Ret>(id, params...));
			iface->reportFinished();
			return future;
		}
		else
		{
			return (new RequestRunner<Ret, Params...>(id, this, params...))->start();
		}
	}
#endif
};

// used frequently
Q_DECLARE_METATYPE(bool *)

/**
 * @example main.cpp
 * @example Gui.h
 * @example Gui.cpp
 * @example Core.h
 * @example Core.cpp
 * @example tst_LogicalGui.cpp
 */
