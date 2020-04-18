#include "Context.h"
#include "Constants.h"
#include <common/Constants.h>
#include <QtNetwork>

Context* Context::s_instance = nullptr;

Context::Context(QObject* parent) : QObject(parent) {
    Q_ASSERT(!s_instance);
    s_instance = this;
    m_manager = new QNetworkAccessManager(this);
}

QNetworkAccessManager* Context::networkAccessManager() {
    return s_instance->m_manager;
}

QString Context::downloadPageUrl() {
    return QString(Const::App::DownloadUrl) + "/install/" + Common::Constants::CurrentOS;
}

QString Context::installerUrl(const QString& version) {
#if defined Q_OS_LINUX
    QString installerName = QString("%1-%2.tar.gz").arg(qApp->applicationName(), version);
#elif defined Q_OS_WIN
    QString installerName = QString("%1-%2.zip").arg(qApp->applicationName(), version);
#elif defined Q_OS_MACOS
    QString installerName = QString("%1-%2.dmg").arg(qApp->applicationName(), version);
#endif

    return downloadPageUrl() + "/" + installerName;
}
