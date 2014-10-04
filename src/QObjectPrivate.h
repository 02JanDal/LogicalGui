#pragma once

#include <QtGlobal>

#if QT_VERSION == QT_VERSION_CHECK(5, 2, 0)
#include <5.2.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 2, 1)
#include <5.2.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 0)
#include <5.3.0/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 1)
#include <5.3.1/QtCore/private/qobject_p.h>
#elif QT_VERSION == QT_VERSION_CHECK(5, 3, 2)
#include <5.3.2/QtCore/private/qobject_p.h>
#else
#error Please add support for this version of Qt
#endif
