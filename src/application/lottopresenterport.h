#ifndef LOTTOPRESENTERPORT_H
#define LOTTOPRESENTERPORT_H

#include "lottoticket.h"

/*!
 * \brief LottoPresenterPort 用例层输出边界端口, 由展示器(适配器层)实现
 *
 * 依赖反转: 用例交互器依赖本接口(归用例层所有), 具体展示器实现它。
 * 交互器在状态变化后调用 present() 推送输出数据, 由展示器适配为
 * 视图展示状态。与 TicketRepository(存储端口)同构, 同属用例层的 DIP 端口。
 */
class LottoPresenterPort
{
public:
    virtual ~LottoPresenterPort() = default;

    /*! 将用例输出(票据状态)交给展示器 */
    virtual void present(const LottoTicket &ticket) = 0;
};

#endif // LOTTOPRESENTERPORT_H
