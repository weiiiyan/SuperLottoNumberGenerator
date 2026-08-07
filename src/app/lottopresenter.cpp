#include "lottopresenter.h"

LottoViewState LottoPresenter::present(const LottoTicket &ticket)
{
    LottoViewState state;

    // 每组: 前区 FRONT_COUNT 个 + 后区 BACK_COUNT 个展示文本, 缺项填占位符
    for (int g = 0; g < LottoTicket::GROUP_COUNT; ++g) {
        const LottoGroup result = ticket.groupAt(g);
        QStringList row;
        for (int i = 0; i < LottoGroup::FRONT_COUNT; ++i) {
            row << (i < result.front.size() ? formatNumber(result.front.at(i))
                                            : QString::fromUtf8(NUMBER_PLACEHOLDER));
        }
        for (int i = 0; i < LottoGroup::BACK_COUNT; ++i) {
            row << (i < result.back.size() ? formatNumber(result.back.at(i))
                                           : QString::fromUtf8(NUMBER_PLACEHOLDER));
        }
        state.groups.append(row);
    }

    // 时间标签: 有效时间按固定格式, 无效显示占位
    const QDateTime time = ticket.generateTime();
    if (time.isValid()) {
        state.timeText = QString::fromUtf8(TIME_PREFIX)
            + time.toString(QString::fromUtf8(TIME_FORMAT));
    } else {
        state.timeText = QString::fromUtf8(TIME_PREFIX) + QString::fromUtf8(TIME_PLACEHOLDER);
    }

    // 按钮互斥状态与文字: 含号码才可锁定, 锁定时禁用生成
    state.lockButtonText = ticket.isLocked() ? QString::fromUtf8(LOCK_TEXT_LOCKED)
                                             : QString::fromUtf8(LOCK_TEXT_UNLOCKED);
    state.lockButtonEnabled = ticket.hasNumbers();
    state.lockButtonChecked = ticket.isLocked();
    state.generateEnabled   = !ticket.isLocked();

    return state;
}
