#include <QApplication>

#include <QDir>

#include "TestGui.h"
#include "LogicalGui.h"

int main(int argc, char **argv)
{
	QApplication app(argc, argv);

	qRegisterMetaType<QDir>("QDir");

	Widget widget;
	widget.show();

	return app.exec();
}
