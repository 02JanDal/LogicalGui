#include "TestGui.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDebug>
#include <QThread>

#include "TestCore.h"
#include "LogicalGui.h"

Widget::Widget(QWidget *parent)
	: QWidget(parent)
{
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
	task.bind("getFileName", this, SLOT(getFileName(QString,QDir)));
	task.run();
}

void Widget::buttonPushedThread()
{
	FileCopyTask *task = new FileCopyTask;
	task->bind("getFileName", this, SLOT(getFileName(QString,QDir)));
	QThread *thread = new QThread(this);
	task->moveToThread(thread);
	connect(thread, &QThread::started, task, &FileCopyTask::run);
	connect(task, &FileCopyTask::done, thread, &QThread::quit);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(thread, &QThread::finished, task, &QThread::deleteLater);
	thread->start();
}

QString Widget::getFileName(const QString &title, const QDir &dir)
{
	qDebug() << title << dir.absolutePath();
	return QFileDialog::getOpenFileName(this, title, dir.absolutePath());
}
