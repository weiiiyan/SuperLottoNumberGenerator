#ifndef LOTTOPRESENTER_H
#define LOTTOPRESENTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "lottopresenterport.h"

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

Q_DECLARE_METATYPE(LottoViewState)

/*!
 * \brief LottoPresenter 输出侧适配器(展示器): 实现用例层输出边界并产生视图展示状态
 *
 * 谦卑对象模式: 所有可测试的展示推导(号码格式化/占位符/按钮文字与互斥
 * 状态/时间显示格式)集中于此, 不依赖任何 Widget。实现 LottoPresenterPort
 * (用例层拥有的输出端口): 用例交互器在状态变化后调用 present(), 本类
 * 推导后重发 viewStateChanged(LottoViewState) 信号供视图消费——视图只感知
 * LottoViewState, 不感知领域实体。buildViewState() 为静态纯函数, 供单元
 * 测试直接调用。
 */
class LottoPresenter : public QObject, public LottoPresenterPort
{
    Q_OBJECT
public:
    explicit LottoPresenter(QObject *parent = nullptr);

    // 视图展示字符串常量(单来源, 视图与测试引用同一来源)
    static constexpr const char *TIME_PREFIX        = "🕐 生成时间：";
    static constexpr const char *TIME_PLACEHOLDER   = "年-月-日 时:分:秒";
    static constexpr const char *TIME_FORMAT        = "yyyy-MM-dd HH:mm:ss";
    static constexpr const char *LOCK_TEXT_LOCKED   = "🔒 解锁生成";
    static constexpr const char *LOCK_TEXT_UNLOCKED = "🔓 锁定号码";
    static constexpr const char *NUMBER_PLACEHOLDER = "?";
    static constexpr const char *GENERATE_TEXT      = "🎲 生成号码";

    // 视图结构常量(别名领域规则, 视图只依赖本层词汇而非领域实体)
    static constexpr int GROUP_COUNT = LottoTicket::GROUP_COUNT;
    static constexpr int FRONT_COUNT = LottoTicket::FRONT_COUNT;
    static constexpr int BACK_COUNT  = LottoTicket::BACK_COUNT;

    /*! 实现输出边界: 将用例输出的票据状态适配为视图展示状态并推送 */
    void present(const LottoTicket &ticket) override;
    /*! 将票据转换为视图展示状态(纯函数, 供单元测试直接调用) */
    static LottoViewState buildViewState(const LottoTicket &ticket);
    /*! 初始(空)展示状态, 供视图构建初始控件文本 */
    static LottoViewState initialState();

signals:
    /*! 用例状态变化后的视图展示状态, 视图唯一渲染入口 */
    void viewStateChanged(const LottoViewState &state);
};

#endif // LOTTOPRESENTER_H
