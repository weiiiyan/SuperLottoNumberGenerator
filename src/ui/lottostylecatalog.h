#ifndef LOTTOSTYLECATALOG_H
#define LOTTOSTYLECATALOG_H

#include <functional>

#include "lottostylehost.h"

class LottoView;
class QWidget;

/*!
 * \brief LottoStyleDefinition 一种风格的元信息与视图工厂
 *
 * create 接收宿主端口与父控件, 返回该风格的视图实例。目录顺序即视图
 * 展示顺序, 首项为默认风格。
 */
struct LottoStyleDefinition {
    LottoStyleInfo info;   /*!< 风格标识与展示名 */
    std::function<LottoView *(LottoStyleHost *, QWidget *)> create;  /*!< 视图工厂 */
};

// 具体目录的构造属于 Main 的职责(ch26): 见 src/app/maincatalog.h。

#endif // LOTTOSTYLECATALOG_H
