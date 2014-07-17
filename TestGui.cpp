#include "TestGui.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QThread>

#include "TestCore.h"
#include "LogicalGui.h"

class GetFileNameWatcher : public AbstractWatcher
{
public:
	QVariant call(const QVariantList &arguments) override
	{
		return QFileDialog::getOpenFileName(
					arguments[0].value<QWidget *>(),
				arguments[1].toString(),
				arguments[2].value<QDir>().absolutePath());
	}
};

Widget::Widget(QWidget *parent)
	: QWidget(parent)
{
	LogicalGui::instance()->registerWatcher("getFileName", new GetFileNameWatcher);

	QPushButton *button = new QPushButton("Push Me!");
	QPushButton *buttonThreaded = new QPushButton("Push Me! (Threaded)");
	QHBoxLayout *layout = new QHBoxLayout;
	layout->addWidget(button);
	layout->addWidget(buttonThreaded);
	setLayout(layout);
	connect(button, &QPushButton::clicked, this, &Widget::buttonPushed);
	connect(buttonThreaded, &QPushButton::clicked, this, &Widget::buttonPushedThread);
}

void Widget::buttonPushed()
{
	FileCopyTask task;
	task.run();
}

void Widget::buttonPushedThread()
{
	FileCopyTask *task = new FileCopyTask;
	QThread *thread = new QThread(this);
	task->moveToThread(thread);
	connect(thread, &QThread::started, task, &FileCopyTask::run);
	connect(task, &FileCopyTask::done, thread, &QThread::quit);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(thread, &QThread::finished, task, &QThread::deleteLater);
	thread->start();
}
