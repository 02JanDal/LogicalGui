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
	task.bind("getFileName", this, &Widget::getFileName);
	task.run();
}

void Widget::buttonPushedThread()
{
	FileCopyTask *task = new FileCopyTask;
	task->bind("getFileName", this, &Widget::getFileName);
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
