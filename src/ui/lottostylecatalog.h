#ifndef LOTTOSTYLECATALOG_H
#define LOTTOSTYLECATALOG_H

#include <QVector>

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

/*!
 * \brief 应用风格目录: 顺序即展示顺序, 首项为默认风格
 *
 * 新增风格只需在此追加一条(并在视图中实现 LottoView)。
 */
QVector<LottoStyleDefinition> lottoStyleCatalog();

#endif // LOTTOSTYLECATALOG_H
