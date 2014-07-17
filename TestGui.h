#pragma once

#include <QWidget>

class QDir;

class Widget : public QWidget
{
	Q_OBJECT
public:
	explicit Widget(QWidget *parent = 0);

public slots:
	void buttonPushed();
	void buttonPushedThread();

private slots:
	QString getFileName(const QString &title, const QDir &dir);
};
