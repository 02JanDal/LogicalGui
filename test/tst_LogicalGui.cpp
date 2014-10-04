#include <QTest>
#include <QThread>
#include <QFuture>
#include <QMutex>

#include <LogicalGui.h>

class TestTarget : public QObject
{
	Q_OBJECT
public:
	explicit TestTarget(QObject *parent = nullptr) : QObject(parent)
	{
	}

	int numHits = 0;
	QMutex mutex;

	void reset()
	{
		numHits = 0;
	}

public slots:
	void hit()
	{
		QMutexLocker locker(&mutex);
		numHits++;
	}
	void hitMultiple(int num)
	{
		QMutexLocker locker(&mutex);
		numHits += num;
	}
	int hitAndReturn()
	{
		QMutexLocker locker(&mutex);
		numHits++;
		return numHits;
	}
	int hitMultipleAndReturn(int num)
	{
		QMutexLocker locker(&mutex);
		numHits += num;
		return numHits;
	}
};

class tst_LogicalGui : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase()
	{
	}
	void cleanupTestCase()
	{
	}

	void usingSLOT()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, SLOT(hit()));
		bindable->bind("HitMultiple", target, SLOT(hitMultiple(int)));
		bindable->bind("HitAndReturn", target, SLOT(hitAndReturn()));
		bindable->bind("HitMultipleAndReturn", target, SLOT(hitMultipleAndReturn(int)));

		QCOMPARE(target->numHits, 0);
		bindable->wait<void>("Hit");
		QCOMPARE(target->numHits, 1);
		bindable->wait<void>("HitMultiple", 42);
		QCOMPARE(target->numHits, 43);
		QCOMPARE(bindable->wait<int>("HitAndReturn"), target->numHits);
		QCOMPARE(target->numHits, 44);
		QCOMPARE(bindable->wait<int>("HitMultipleAndReturn", 42), target->numHits);
		QCOMPARE(target->numHits, 86);

		delete bindable, target;
	}
	void usingPointer()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, &TestTarget::hit);
		bindable->bind("HitMultiple", target, &TestTarget::hitMultiple);
		bindable->bind("HitAndReturn", target, &TestTarget::hitAndReturn);
		bindable->bind("HitMultipleAndReturn", target, &TestTarget::hitMultipleAndReturn);

		QCOMPARE(target->numHits, 0);
		bindable->wait<void>("Hit");
		QCOMPARE(target->numHits, 1);
		bindable->wait<void>("HitMultiple", 42);
		QCOMPARE(target->numHits, 43);
		QCOMPARE(bindable->wait<int>("HitAndReturn"), target->numHits);
		QCOMPARE(target->numHits, 44);
		QCOMPARE(bindable->wait<int>("HitMultipleAndReturn", 42), target->numHits);
		QCOMPARE(target->numHits, 86);

		delete bindable, target;
	}
	void usingPointerDifferentThread()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, &TestTarget::hit);
		bindable->bind("HitMultiple", target, &TestTarget::hitMultiple);
		bindable->bind("HitAndReturn", target, &TestTarget::hitAndReturn);
		bindable->bind("HitMultipleAndReturn", target, &TestTarget::hitMultipleAndReturn);

		QThread *thread = new QThread;
		thread->start();
		target->moveToThread(thread);

		QCOMPARE(target->numHits, 0);
		bindable->wait<void>("Hit");
		QCOMPARE(target->numHits, 1);
		bindable->wait<void>("HitMultiple", 42);
		QCOMPARE(target->numHits, 43);
		QCOMPARE(bindable->wait<int>("HitAndReturn"), target->numHits);
		QCOMPARE(target->numHits, 44);
		QCOMPARE(bindable->wait<int>("HitMultipleAndReturn", 42), target->numHits);
		QCOMPARE(target->numHits, 86);

		delete bindable, thread, target;
	}

	// FIXME using lambdas currently doesn't work
	/*
	void usingLambda()
	{
		Bindable *bindable = new Bindable;
		int numHits = 0;
		bindable->bind("Hit", [&numHits](){numHits++;});
		bindable->bind("HitMultiple", [&numHits](int num){numHits+=num;});
		bindable->bind("HitAndReturn", [&numHits](){numHits++;return numHits;});
		bindable->bind("HitMultipleAndReturn", [&numHits](int num){numHits+=num;return
	numHits;});

		QCOMPARE(numHits, 0);
		bindable->waitVoid("Hit");
		QCOMPARE(numHits, 1);
		bindable->waitVoid("HitMultiple", 42);
		QCOMPARE(numHits, 43);
		QCOMPARE(bindable->wait<int>("HitAndReturn"), numHits);
		QCOMPARE(numHits, 44);
		QCOMPARE(bindable->wait<int>("HitMultipleAndReturn", 42), numHits);
		QCOMPARE(numHits, 86);

		delete bindable;
	}*/

	void parentBindings()
	{
		Bindable *bindable1 = new Bindable;
		Bindable *bindable2 = new Bindable(bindable1);
		Bindable *bindable3 = new Bindable;
		bindable3->setBindableParent(bindable2);
		Bindable *bindable4 = new Bindable(bindable3);

		TestTarget *target = new TestTarget;
		bindable1->bind("Hit", target, SLOT(hit()));
		bindable1->bind("HitMultiple", target, SLOT(hitMultiple(int)));
		bindable1->bind("HitMultipleAndReturn", target, SLOT(hitMultipleAndReturn(int)));

		QCOMPARE(target->numHits, 0);
		QCOMPARE(bindable4->wait<int>("HitMultipleAndReturn", 2), target->numHits);
		QCOMPARE(target->numHits, 2);
		bindable4->wait<void>("Hit");
		QCOMPARE(target->numHits, 3);
		bindable4->wait<void>("HitMultiple", 3);
		QCOMPARE(target->numHits, 6);

		delete bindable1, bindable2, bindable3, bindable4, target;
	}

	void asyncRequests()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, &TestTarget::hit);
		bindable->bind("HitMultiple", target, &TestTarget::hitMultiple);
		bindable->bind("HitAndReturn", target, &TestTarget::hitAndReturn);
		bindable->bind("HitMultipleAndReturn", target, &TestTarget::hitMultipleAndReturn);

		QCOMPARE(target->numHits, 0);
		QCOMPARE(bindable->request<int>("HitAndReturn").result(), target->numHits);
		QCOMPARE(target->numHits, 1);
		QCOMPARE(bindable->request<int>("HitMultipleAndReturn", 42).result(), target->numHits);
		QCOMPARE(target->numHits, 43);
	}
	void asyncRequestsDifferentThread()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, &TestTarget::hit);
		bindable->bind("HitMultiple", target, &TestTarget::hitMultiple);
		bindable->bind("HitAndReturn", target, &TestTarget::hitAndReturn);
		bindable->bind("HitMultipleAndReturn", target, &TestTarget::hitMultipleAndReturn);

		QThread *thread = new QThread;
		thread->start();
		target->moveToThread(thread);

		QCOMPARE(target->numHits, 0);
		QCOMPARE(bindable->request<int>("HitAndReturn").result(), target->numHits);
		QCOMPARE(target->numHits, 1);
		QCOMPARE(bindable->request<int>("HitMultipleAndReturn", 42).result(), target->numHits);
		QCOMPARE(target->numHits, 43);
	}
	void asyncRequestsFuture()
	{
		Bindable *bindable = new Bindable;
		TestTarget *target = new TestTarget;
		bindable->bind("Hit", target, &TestTarget::hit);
		bindable->bind("HitMultiple", target, &TestTarget::hitMultiple);
		bindable->bind("HitAndReturn", target, &TestTarget::hitAndReturn);
		bindable->bind("HitMultipleAndReturn", target, &TestTarget::hitMultipleAndReturn);

		QThread *thread = new QThread;
		thread->start();
		target->moveToThread(thread);

		QCOMPARE(target->numHits, 0);
		target->mutex.lock();
		QFuture<int> f1 = bindable->request<int>("HitAndReturn");
		QVERIFY(f1.isStarted());
		QVERIFY(f1.isRunning());
		QVERIFY(!f1.isFinished());
		QVERIFY(!f1.isCanceled());
		target->mutex.unlock();
		f1.waitForFinished();
		QVERIFY(!f1.isRunning());
		QVERIFY(!f1.isCanceled());
		QVERIFY(f1.isFinished());
		QCOMPARE(f1.result(), 1);
	}
};

QTEST_GUILESS_MAIN(tst_LogicalGui)

#include "tst_LogicalGui.moc"
