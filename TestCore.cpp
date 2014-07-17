#include "TestCore.h"

#include <QThread>
#include <QDir>

#include "LogicalGui.h"

FileCopyTask::FileCopyTask(QObject *parent)
	: QObject(parent)
{
}

void FileCopyTask::run()
{
	const QString filename = wait<QString>("getFileName", 0, tr("Choose file"), QDir::current());
	QThread::sleep(10); // work hard
	emit done();
}
