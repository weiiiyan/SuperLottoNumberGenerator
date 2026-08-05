#ifndef LOTTOPRESENTER_H
#define LOTTOPRESENTER_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "lottoticket.h"

/// 号码格式化: 零填充两位数字(如 3 → "03"), 供展示器与测试复用
inline QString formatNumber(int n)
{
    return QString::number(n).rightJustified(2, '0');
}

/*!
 * \brief LottoViewState 票据的视图展示状态(展示器输出, 与具体控件无关)
 */
struct LottoViewState {
    QVector<QStringList> groups;       /*!< 每组展示文本: 前区 FRONT_COUNT 个 + 后区 BACK_COUNT 个, 缺项为占位符 */
    QString timeText;                  /*!< 时间标签完整文本 */
    QString lockButtonText;            /*!< 锁定按钮文字 */
    bool lockButtonEnabled = false;    /*!< 锁定按钮可用性(含号码才可锁定) */
    bool lockButtonChecked  = false;   /*!< 锁定按钮选中状态 */
    bool generateEnabled    = true;    /*!< 生成按钮可用性(锁定时禁用) */
};

/*!
 * \brief LottoPresenter 展示器: 将领域票据转为视图展示状态
 *
 * 谦卑对象模式: 所有可测试的展示逻辑(号码格式化/占位符/按钮文字与互斥
 * 状态/时间显示格式)集中于此, MainWindow 只做机械的控件写入,
 * 不包含任何可测试的展示推导。本类不依赖任何 Widget, 可独立测试。
 */
class LottoPresenter
{
public:
    static constexpr const char *TIME_PREFIX        = "🕐 生成时间：";
    static constexpr const char *TIME_PLACEHOLDER   = "年-月-日 时:分:秒";
    static constexpr const char *TIME_FORMAT        = "yyyy-MM-dd HH:mm:ss";
    static constexpr const char *LOCK_TEXT_LOCKED   = "🔒 解锁生成";
    static constexpr const char *LOCK_TEXT_UNLOCKED = "🔓 锁定号码";
    static constexpr const char *NUMBER_PLACEHOLDER = "?";

    /*! 将票据转换为视图展示状态 */
    static LottoViewState present(const LottoTicket &ticket);
};

#endif // LOTTOPRESENTER_H
