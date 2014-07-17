#pragma once

#include "LogicalGui.h"

#include <QDir>

class FileCopyTask : public Task
{
	Q_OBJECT
public:
	FileCopyTask(QObject *parent = 0);

public slots:
	void run();

signals:
	void done();
};

Q_DECLARE_METATYPE(QDir)
