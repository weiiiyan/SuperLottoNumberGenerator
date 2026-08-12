#ifndef LOTTOCONTROLLER_H
#define LOTTOCONTROLLER_H

#include <QObject>

class LottoInputBoundary;

/*!
 * \brief LottoController 输入侧适配器(控制器): 把外部输入翻译为用例调用
 *
 * 按《架构整洁之道》图 22.2, Controller 属接口适配器层, 负责把外部输入
 * (HTTP 请求/UI 事件)打包成用例理解的调用。本类依赖用例层输入边界
 * LottoInputBoundary(由用例交互器实现), 把视图的按钮点击翻译为对边界
 * 的用例调用, 不含业务判断——锁定时拒绝生成等应用特定规则仍在用例层
 * (此处判定是权威)。不依赖任何 Widget, 可独立测试。
 */
class LottoController : public QObject
{
    Q_OBJECT
public:
    /*!
     * \brief 构造输入侧适配器
     * \param interactor 输入边界(由用例交互器实现, 不持有所有权, 须非空)
     * \param parent QObject 父对象
     */
    explicit LottoController(LottoInputBoundary *interactor, QObject *parent = nullptr);

public slots:
    /*! 视图 [生成] 点击 → 调用生成用例(锁定时由用例层拒绝) */
    void onGenerateRequested();
    /*! 视图 [锁定/解锁] 点击 → 调用锁定切换用例 */
    void onLockRequested();

private:
    LottoInputBoundary *m_interactor = nullptr;  /*!< 输入边界(不持有所有权) */
};

#endif // LOTTOCONTROLLER_H
