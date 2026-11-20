#ifndef LOTTOSTYLESTORE_H
#define LOTTOSTYLESTORE_H

#include <QString>

/*!
 * \brief LottoStyleStore 风格偏好端口: 持久化"用户选择了哪套 UI"
 *
 * 风格选择属呈现层偏好, 不是应用业务规则, 因此归视图层所有, 由
 * 组合根注入具体实现(QSettings)。用例层不感知本端口。
 */
class LottoStyleStore
{
public:
    virtual ~LottoStyleStore() = default;

    /*! 读取上次选择的风格 id, 空串表示从未选择 */
    virtual QString loadStyleId() const = 0;
    /*! 保存风格 id */
    virtual void saveStyleId(const QString &id) = 0;
};

#endif // LOTTOSTYLESTORE_H
