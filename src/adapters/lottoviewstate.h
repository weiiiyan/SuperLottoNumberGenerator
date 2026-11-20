#ifndef LOTTOVIEWSTATE_H
#define LOTTOVIEWSTATE_H

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

#include "lottogroup.h"

/*!
 * \brief LottoViewState 票据的视图展示状态(展示器输出, 与具体控件无关)
 *
 * 跨 适配器 → 框架与驱动 边界传输的简单数据结构: 只含展示字符串与布尔
 * 开关, 不含任何业务实体。视图只消费本结构, 不感知 LottoTicket。
 */
struct LottoViewState {
    /*! 每组展示文本: 前区 FRONT_COUNT 个 + 后区 BACK_COUNT 个, 缺项为占位符 */
    QVector<QStringList> groups;
    QString timeLabelText;             /*!< 时间标签标题, 如 "生成时间：" */
    QString timeValueText;             /*!< 时间值或占位, 如 "2026-01-01 08:00:00" */
    QString generateButtonText;        /*!< 生成按钮文字 */
    QString numberPlaceholder;         /*!< 无号码时的占位文本 */
    QString separatorText;             /*!< 前后区之间的分隔符文本 */
    bool    generateEnabled    = true; /*!< 生成按钮可用性(锁定时禁用) */
    QString lockButtonText;            /*!< 锁定按钮文字 */
    bool    lockButtonEnabled  = false;/*!< 锁定按钮可用性(含号码才可锁定) */
    bool    lockButtonChecked  = false;/*!< 锁定按钮选中状态 */
};

Q_DECLARE_METATYPE(LottoViewState)

/*!
 * \brief 视图展示文案常量(单来源, 视图与测试引用同一来源)
 *
 * 文案中不得出现 emoji 或图标字符; 图标属装饰, 由各风格视图自选。
 */
namespace LottoViewText {
constexpr const char *TIME_LABEL         = "生成时间：";
constexpr const char *TIME_PLACEHOLDER   = "年-月-日 时:分:秒";
constexpr const char *TIME_FORMAT        = "yyyy-MM-dd HH:mm:ss";
constexpr const char *LOCK_TEXT_LOCKED   = "解锁生成";
constexpr const char *LOCK_TEXT_UNLOCKED = "锁定号码";
constexpr const char *NUMBER_PLACEHOLDER = "?";
constexpr const char *SEPARATOR_TEXT     = "+";
constexpr const char *GENERATE_TEXT      = "生成号码";
} // namespace LottoViewText

/*!
 * \brief 视图结构常量(别名领域规则, 视图只依赖本层词汇而非领域实体)
 *
 * GROUP_COUNT 是票据级规则(LottoTicket 所有); LottoTicket 是领域实体,
 * 为保证本头不把实体带进视图作用域, 这里写为字面量, 并由
 * lottopresenter.cpp 的 static_assert 与 LottoTicket::GROUP_COUNT 绑定。
 */
namespace LottoViewShape {
constexpr int GROUP_COUNT = 5;
constexpr int FRONT_COUNT = LottoGroup::FRONT_COUNT;
constexpr int BACK_COUNT  = LottoGroup::BACK_COUNT;
} // namespace LottoViewShape

#endif // LOTTOVIEWSTATE_H
