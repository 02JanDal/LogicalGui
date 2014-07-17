#pragma once

#include <QObject>
#include <QDir>

class FileCopyTask : public QObject
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
