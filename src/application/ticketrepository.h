#ifndef TICKETREPOSITORY_H
#define TICKETREPOSITORY_H

#include "lottoticket.h"

/*!
 * \brief TicketRepository 票据持久化接口(用例层定义, 适配器层实现)
 *
 * 通过依赖反转将持久化细节与业务逻辑解耦:
 * 用例只依赖本接口, 具体存储(QSettings、文件、数据库)由外层适配器实现。
 */
class TicketRepository
{
public:
    virtual ~TicketRepository() = default;

    /*!
     * \brief 保存票据
     *
     * 锁定时写全量数据; 未锁定时仅写 isLocked=false(磁盘残留旧号码不参与恢复)。
     */
    virtual void save(const LottoTicket &ticket) = 0;

    /*! 加载票据; 未锁定时返回默认票据(空号码, 无效时间) */
    virtual LottoTicket load() = 0;

    /*! 清空全部持久化数据(测试辅助) */
    virtual void clear() = 0;
};

#endif // TICKETREPOSITORY_H
