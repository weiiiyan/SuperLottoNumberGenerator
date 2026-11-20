#ifndef MAINCATALOG_H
#define MAINCATALOG_H

#include <QVector>

#include "lottostylecatalog.h"

/*!
 * \brief 构建应用风格目录: 顺序即展示顺序, 首项为默认风格
 *
 * 具体风格类名只允许出现在 Main(本文件)中 —— ch26 要求具体类名与配置
 * 集中在最外层。核心库 lotto_ui 只认识 LottoStyleDefinition 类型。
 */
QVector<LottoStyleDefinition> buildStyleCatalog();

#endif // MAINCATALOG_H
