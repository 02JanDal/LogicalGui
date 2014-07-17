#include "TestCore.h"

#include <QThread>
#include <QDir>

#include "LogicalGui.h"

FileCopyTask::FileCopyTask(QObject *parent)
	: Task(parent)
{
}

void FileCopyTask::run()
{
	const QString filename = wait<QString>("getFileName", tr("Choose file"), QDir::current());
	QThread::sleep(10); // work hard
	qDebug("Result: %s", qPrintable(filename));
	emit done();
}
