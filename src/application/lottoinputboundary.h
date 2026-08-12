#ifndef LOTTOINPUTBOUNDARY_H
#define LOTTOINPUTBOUNDARY_H

/*!
 * \brief LottoInputBoundary 用例层输入边界接口, 由用例交互器实现
 *
 * 依赖反转: 输入侧适配器(Controller)依赖本接口(归用例层所有),
 * 用例交互器实现它。Controller 把外部输入翻译为对本接口的用例调用,
 * 不触碰具体交互器。与 LottoPresenterPort(输出边界)对称, 共同构成
 * 书图 22.2 中 Controller / Presenter 两侧的端口。
 */
class LottoInputBoundary
{
public:
    virtual ~LottoInputBoundary() = default;

    /*! 用例: 生成 5 组新号码(锁定时拒绝) */
    virtual void generateNewTicket() = 0;
    /*! 用例: 切换锁定状态, 状态变化即保存 */
    virtual void toggleLock() = 0;
    /*! 用例: 设置锁定状态, 状态未变化时不做任何事 */
    virtual void setLocked(bool locked) = 0;
    /*! 用例: 异步恢复持久化的票据(singleShot(0), 不阻塞 Android 启动) */
    virtual void load() = 0;
};

#endif // LOTTOINPUTBOUNDARY_H
