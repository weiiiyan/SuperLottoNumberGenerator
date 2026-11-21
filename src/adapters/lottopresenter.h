#ifndef LOTTOPRESENTER_H
#define LOTTOPRESENTER_H

#include <QObject>
#include <QString>
#include <QVector>

#include "lottopresenterport.h"
#include "lottoviewstate.h"

/*!
 * \brief LottoPresenter 输出侧适配器(展示器): 实现用例层输出边界并产生视图展示状态
 *
 * 谦卑对象模式: 所有可测试的展示推导(号码格式化/占位符/按钮文字与互斥
 * 状态/时间显示格式)集中于此, 不依赖任何 Widget。实现 LottoPresenterPort
 * (用例层拥有的输出端口): 用例交互器在状态变化后调用 present(), 本类
 * 推导后更新 state() 缓存并重发 viewStateChanged(LottoViewState) 信号供视图
 * 消费——视图只感知 LottoViewState, 不感知领域实体。buildViewState() 为
 * 静态纯函数, 供单元测试直接调用。
 */
class LottoPresenter : public QObject, public LottoPresenterPort
{
    Q_OBJECT
public:
    explicit LottoPresenter(QObject *parent = nullptr);

    /*! \brief 号码格式化: 零填充两位数字(如 3 → "03") */
    static inline QString formatNumber(int n)
    {
        return QString::number(n).rightJustified(2, '0');
    }

    /*! 实现输出边界: 将用例输出的票据状态适配为视图展示状态并推送 */
    void present(const LottoTicket &ticket) override;
    /*! 将票据转换为视图展示状态(纯函数, 供单元测试直接调用) */
    static LottoViewState buildViewState(const LottoTicket &ticket);
    /*! 初始(空)展示状态, 供视图构建初始控件文本 */
    static LottoViewState initialState();
    /*! 最近一次推送的展示状态(懒创建视图的首帧来源) */
    const LottoViewState &state() const;
    /*! 票号推导: 生成时间有效且含号码时取 "NO. yyMMdd NNNN", 否则为占位 */
    static QString buildSerial(const QDateTime &time, bool hasNumbers);

private:
    LottoViewState m_lastState;   /*!< 最近一次展示状态, 构造时以 initialState() 播种 */

signals:
    /*! 用例状态变化后的视图展示状态, 视图唯一渲染入口 */
    void viewStateChanged(const LottoViewState &state);
};

#endif // LOTTOPRESENTER_H
