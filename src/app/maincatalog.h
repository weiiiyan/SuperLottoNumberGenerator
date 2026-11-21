#ifndef MAINCATALOG_H
#define MAINCATALOG_H

#include <QVector>

#include "lottostylecatalog.h"

class LottoController;
class LottoPresenter;

/*!
 * \brief 构建应用风格目录: 顺序即展示顺序, 首项为默认风格
 *
 * 具体风格类名只允许出现在 Main(本文件)中 —— ch26 要求具体类名与配置
 * 集中在最外层。核心库 lotto_ui 只认识 LottoStyleDefinition 类型。
 * 本函数返回未接线的原始目录; 接线由 wireStyleCatalog() 统一完成。
 */
QVector<LottoStyleDefinition> buildStyleCatalog();

/*!
 * \brief 为目录中的每个视图工厂接上展示器与控制器
 *
 * 每个工厂被包装为: 创建视图 → 连接 viewStateChanged → 连接两个请求信号
 * → 用 presenter->state() 渲染首帧。接线只允许出现在 Main。
 */
QVector<LottoStyleDefinition> wireStyleCatalog(
    QVector<LottoStyleDefinition> catalog,
    LottoController *controller,
    LottoPresenter *presenter);

#endif // MAINCATALOG_H
