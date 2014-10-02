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
#include <QThread>
#include <QCoreApplication>
#include <QSemaphore>

#if QT_VERSION == QT_VERSION_CHECK(5, 2, 0)
#include <5.2.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 2, 1)
#include <5.2.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 0)
#include <5.3.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 1)
#include <5.3.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 2)
#include <5.3.2/QtCore/private/qobject_p.h>
#else
#error Please add support for this version of Qt
#endif

/**
 * @brief Inherit from Bindable in a logic class to be able to call GUI code from it
 *
 * @par Terminology
 *
 * * Callback - A QObject slot, member function, lambda, static member function, functor, global function etc.
 * * Callback ID - A string identifying a callback. Used by @ref wait and @ref waitVoid to look-up callbacks set with @ref bind
 * * Binding - A mapping between a callback ID and a callback. Set using @ref bind and unset using @ref unbind
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
 * 2. Inside @a MyClass, use @ref wait and @ref waitVoid
 * @code
 * bool res = wait<bool>("Continue?", "Do you want to continue?");
 * @endcode
 * 3. When creating an instance of @a MyClass, use one of the @ref bind forms
 * @code
 * MyClass *obj = new MyClass;
 * obj->bind("Continue?", [](const QString &question) {
 *     // display the question to the user, return true or false depending on what the user answered
 * });
 * @endcode
 * 4. You can also create a standalone @ref Bindable object, and use it as a binding container
 * @code
 * Bindable *container = new Bindable;
 * container->bind("Continue?", [](const QString &question) {
 *     // display the question to the user, return true or false depending on what the user answered
 * });
 *
 * MyClass *obj = new MyClass;
 * obj->setBindableParent(container);
 * @endcode
 * You could also create a constructor for MyClass that takes a Bindable *, and then pass that to the Bindable::Bindable constructor
 *
 * @par Unit Testing
 *
 * LogicalGui is also useful for unit testing. Just bind callback IDs to placeholder callbacks, that for example return test data.
 * @code
 * MyClass *classUnderTest = new MyClass;
 * classUnderTest->bind("Continue?", [](QString) { return true; });
 * @endcode
 *
 */
class Bindable
{
	friend class tst_LogicalGui;

public:
	/**
	 * @param parent This instance of Bindable will inherit bindings from it's parent
	 * @see setBindableParent
	 */
	Bindable(Bindable *parent = 0) : m_parent(parent)
	{
	}
	virtual ~Bindable()
	{
	}

	/**
	 * @brief Lets this instance inherit bindings from parent
	 * @param parent This instance of Bindable will inherit bindings from it's parent
	 *
	 * You can still add bindings to the parent after you've called @a setBindableParent, and they'll be available to this instance
	 */
	void setBindableParent(Bindable *parent)
	{
		m_parent = parent;
	}

	/**
	 * @brief Bind an old-style slot (using SLOT(...) syntax) to a callback ID
	 * @param id              The callback ID, as will be given to Bindable::wait or Bindable::waitVoid
	 * @param receiver        The QObject instance on which the callback will be called
	 * @param methodSignature The signature of the callback, as given by SLOT(...)
	 */
	void bind(const QString &id, const QObject *receiver, const char *methodSignature)
	{
		auto mo = receiver->metaObject();
		Q_ASSERT_X(mo, "Bindable::bind",
				   "Invalid metaobject. Did you forget the QObject macro?");
		const QMetaMethod method = mo->method(mo->indexOfMethod(
			QMetaObject::normalizedSignature(methodSignature + 1).constData()));
		Q_ASSERT_X(method.isValid(), "Bindable::bind", "Invalid method signature");
		m_bindings.insert(id, Binding(receiver, method));
	}
	/**
	 * @brief Bind a member function to a callback ID
	 * @param id       The callback ID, as will be given to Bindable::wait or Bindable::waitVoid
	 * @param receiver The QObject instance on which the callback will be called
	 * @param slot     The member function that will be called
	 */
	template <typename Func>
	void bind(const QString &id,
			  const typename QtPrivate::FunctionPointer<Func>::Object *receiver, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id,
			Binding(receiver, new QtPrivate::QSlotObject<Func, typename SlotType::Arguments,
														 typename SlotType::ReturnType>(slot)));
	}
	/**
	 * @brief Bind a lambda, static member, functor or similar to a callback ID
	 * @param id   The callback ID, as will be given to Bindable::wait and Bindable::waitVoid
	 * @param slot The lambda, static member, functor or similar that will be called
	 */
	template <typename Func> void bind(const QString &id, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id,
			Binding(nullptr, new QtPrivate::QSlotObject<Func, typename SlotType::Arguments,
														typename SlotType::ReturnType>(slot)));
	}

	/**
	 * @brief Remove the binding with the given ID
	 * @param id The callback ID of the binding to remove
	 */
	void unbind(const QString &id)
	{
		m_bindings.remove(id);
	}

private:
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
	QMap<QString, Binding> m_bindings;

	Bindable *m_parent;

private:
	inline Qt::ConnectionType connectionType(const QObject *receiver)
	{
		return receiver == nullptr ? Qt::DirectConnection
								   : (QThread::currentThread() == receiver->thread()
										  ? Qt::DirectConnection
										  : Qt::BlockingQueuedConnection);
	}
	void callSlotObject(Binding binding, void **args)
	{
		if (connectionType(binding.receiver) == Qt::BlockingQueuedConnection)
		{
			QSemaphore semaphore;
			QMetaCallEvent *ev =
				new QMetaCallEvent(binding.object, nullptr, -1, 0, 0, args, &semaphore);
			QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
			semaphore.acquire();
		}
		else
		{
			binding.object->call(const_cast<QObject *>(binding.receiver), args);
		}
	}

protected:
	/**
	 * @brief Calls a callback by it's ID, taking threads etc. into account
	 * @param id     The callback ID to call, as previously bound using Bindable::bind
	 * @param params The parameters to pass to the callback
	 * @returns The return value of the callback
	 * @see waitVoid
	 */
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		static_assert(!std::is_same<Ret, void>::value, "You need to use Bindable::waitVoid");

		if (!m_bindings.contains(id) && m_parent)
		{
			return m_parent->wait<Ret, Params...>(id, params...);
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		Ret ret;
		if (binding.object)
		{
			void *args[] = {&ret,
							const_cast<void *>(reinterpret_cast<const void *>(&params))...};
			callSlotObject(binding, args);
		}
		else
		{
			const QMetaMethod method = binding.method;
			Q_ASSERT_X(method.parameterCount() == sizeof...(params), "Bindable::wait",
					   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
									  .arg(method.parameterCount(), sizeof...(params))));
			Q_ASSERT_X(
				qMetaTypeId<Ret>() != QMetaType::UnknownType, "Bindable::wait",
				"Requested return type is not registered, please use the Q_DECLARE_METATYPE "
				"macro to make it known to Qt's meta-object system");
			Q_ASSERT_X(
				method.returnType() == qMetaTypeId<Ret>() ||
					QMetaType::hasRegisteredConverterFunction(method.returnType(),
															  qMetaTypeId<Ret>()),
				"Bindable::wait",
				qPrintable(
					QString(
						"Requested return type (%1) is incompatible method return type (%2)")
						.arg(QMetaType::typeName(qMetaTypeId<Ret>()),
							 QMetaType::typeName(method.returnType()))));
			const auto retArg = QReturnArgument<Ret>(
				QMetaType::typeName(qMetaTypeId<Ret>()),
				ret); // because Q_RETURN_ARG doesn't work with templates...
			method.invoke(const_cast<QObject *>(binding.receiver), connectionType(binding.receiver), retArg,
						  Q_ARG(Params, params)...);
		}
		return ret;
	}
	/**
	 * @brief Calls a callback by it's ID, taking threads etc. into account
	 * @param id     The callback ID to call, as previously bound using Bindable::bind
	 * @param params The parameters to pass to the callback
	 * @see wait
	 */
	template <typename... Params> void waitVoid(const QString &id, Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			m_parent->waitVoid<Params...>(id, params...);
			return;
		}
		Q_ASSERT(m_bindings.contains(id));
		const auto binding = m_bindings[id];
		if (binding.object)
		{
			void *args[] = {0, const_cast<void *>(reinterpret_cast<const void *>(&params))...};
			callSlotObject(binding, args);
		}
		else
		{
			const QMetaMethod method = binding.method;
			Q_ASSERT_X(method.parameterCount() == sizeof...(params), "Bindable::wait",
					   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
									  .arg(method.parameterCount(), sizeof...(params))));
			method.invoke(const_cast<QObject *>(binding.receiver), connectionType(binding.receiver),
						  Q_ARG(Params, params)...);
		}
	}
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
