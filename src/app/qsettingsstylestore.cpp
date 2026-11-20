#include "qsettingsstylestore.h"

namespace {
constexpr const char *STYLE_KEY = "ui/styleId";
}

QSettingsStyleStore::QSettingsStyleStore(QSettings *settings)
    : m_settings(settings)
{
}

QString QSettingsStyleStore::loadStyleId() const
{
    if (!m_settings)
        return QString();
    return m_settings->value(QString::fromLatin1(STYLE_KEY)).toString();
}

void QSettingsStyleStore::saveStyleId(const QString &id)
{
    if (!m_settings)
        return;
    m_settings->setValue(QString::fromLatin1(STYLE_KEY), id);
    m_settings->sync();
}
