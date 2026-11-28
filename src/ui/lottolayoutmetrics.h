#ifndef LOTTOLAYOUTMETRICS_H
#define LOTTOLAYOUTMETRICS_H

#include <QMargins>

/*!
 * \brief 两套主题共用的骨架尺寸令牌(390 × 844 竖屏基准)
 *
 * 票据视图直接引用本头; QML 侧在 NeumoTheme.qml 中镜像同一组数值。
 * 两处若出现漂移, 以 doc/界面设计/design-spec.html 的尺寸令牌表为准。
 */
namespace LottoLayout {

inline constexpr int CanvasWidth   = 390;  /*!< 设计画布宽 */
inline constexpr int CanvasHeight  = 844;  /*!< 设计画布高 */
inline constexpr int PadX          = 22;   /*!< 左右安全边距 */
inline constexpr int PadTop        = 40;   /*!< 上安全边距 */
inline constexpr int PadBottom     = 34;   /*!< 下安全边距 */

inline constexpr int TopBarHeight  = 66;   /*!< 顶栏(品牌 + 风格按钮) */
inline constexpr int MetaHeight    = 60;   /*!< 时间条 */
inline constexpr int PerfHeight    = 26;   /*!< 骑缝线 */
inline constexpr int StubHeight    = 105;  /*!< 两行票根 */
inline constexpr int ActionsHeight = 81;   /*!< 操作区(含上内边距) */
inline constexpr int ActionHeight  = 56;   /*!< 按钮高 */
inline constexpr double GenWidthRatio = 1.32; /*!< 生成按钮相对锁定按钮的宽度比 */

inline constexpr int BallSize      = 40;   /*!< 球径上限(390 宽下) */
inline constexpr int BallSizeMin   = 24;   /*!< 窄屏压缩下限 */
inline constexpr int BallGap       = 4;    /*!< 球间距 */
inline constexpr int SeparatorWidth = 10;  /*!< 前后区之间的 "+" 槽位宽 */
inline constexpr int RowIndexWidth = 18;   /*!< 行号列宽 */
inline constexpr int RowHeight     = 76;   /*!< 行高上限 */
inline constexpr int RowHeightMin  = 68;   /*!< 窗口变矮时的行高下限 */
inline constexpr int StubBarcodeWidth  = 92;
inline constexpr int StubBarcodeHeight = 20;

/*!
 * \brief 视图可压缩到的下限尺寸
 *
 * 票据视图的左右居中边距由当前宽度推导, 其布局最小尺寸会随之≈当前宽度。
 * 若把该值上报给宿主, 页面创建时(390 x 844)会把整个窗口的最小宽度抬到
 * 超过手机屏幕, 页面被布局到屏幕之外(右侧内容被裁), 且该页隐藏后仍留在
 * 视图栈中持续抬高下限。视图的最小尺寸必须是与当前尺寸无关的常量。
 */
inline constexpr int MinimumWidth  = 320;  /*!< 最小宽度(最窄手机) */
inline constexpr int MinimumHeight = 500;  /*!< 最小高度(压缩后的可行高) */

/*!
 * \brief 有效上/下安全边距 = 设计令牌与窗口安全区取较大者
 *
 * Android 全屏(edge-to-edge)时状态栏与手势条会覆盖窗口上下边缘。
 * 设计骨架的 40/34 是常规机型的经验值, 安全区更大时以安全区为准,
 * 避免内容被系统 UI 遮挡; 桌面端安全区为 0, 结果即设计值。
 */
inline int effectivePadTop(const QMargins &safeArea)
{
    return qMax(PadTop, safeArea.top());
}

inline int effectivePadBottom(const QMargins &safeArea)
{
    return qMax(PadBottom, safeArea.bottom());
}

} // namespace LottoLayout

#endif // LOTTOLAYOUTMETRICS_H
