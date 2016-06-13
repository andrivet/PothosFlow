// Copyright (c) 2013-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "PothosGuiUtils.hpp"
#include <Pothos/System.hpp>
#include <Poco/Path.h>
#include <Poco/SingletonHolder.h>
#include <QSplashScreen>
#include <QApplication>
#include <QFormLayout>
#include <QDir>

QString makeIconPath(const QString &name)
{
    Poco::Path path(Pothos::System::getDataPath());
    path.append("icons");
    if (not name.isEmpty()) path.append(name.toStdString());
    return QDir::fromNativeSeparators(QString::fromStdString(path.toString()));
}

QIcon makeIconFromTheme(const QString &name)
{
    return QIcon::fromTheme(name, QIcon(makeIconPath(name+".png")));
}

void postStatusMessage(const QString &msg)
{
    getSplashScreen()->showMessage(msg, Qt::AlignLeft | Qt::AlignBottom);
    QApplication::instance()->processEvents();
}

QSplashScreen *getSplashScreen(void)
{
    static QSplashScreen *splash = nullptr;
    if (splash == nullptr)
    {
        QPixmap pixmap(makeIconPath("PothosSplash.png"));
        splash = new QSplashScreen(pixmap);
    }
    return splash;
}

QFormLayout *makeFormLayout(QWidget *parent)
{
    auto layout = new QFormLayout(parent);
    layout->setRowWrapPolicy(QFormLayout::DontWrapRows);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->setLabelAlignment(Qt::AlignLeft);
    return layout;
}
