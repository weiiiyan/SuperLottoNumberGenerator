#ifndef QSETTINGSSTYLESTORE_H
#define QSETTINGSSTYLESTORE_H

#include <QSettings>

#include "lottostylestore.h"

/*!
 * \brief QSettingsStyleStore 风格偏好端口的 QSettings 实现
 *
 * 键名 ui/styleId 与票据持久化共用同一个 INI 文件, 不拥有 QSettings
 * 的所有权(组合根管理其生命周期)。
 */
class QSettingsStyleStore : public LottoStyleStore
{
public:
    /*! 构造, \a settings 须非空且在存储体生命周期内有效 */
    explicit QSettingsStyleStore(QSettings *settings);

    /*! 读取 "ui/styleId", 未设置时为空串 */
    QString loadStyleId() const override;
    /*! 写入 "ui/styleId" 并 sync */
    void saveStyleId(const QString &id) override;

private:
    QSettings *m_settings = nullptr;   /*!< 设置的存储后端(不持有所有权) */
};

#endif // QSETTINGSSTYLESTORE_H
