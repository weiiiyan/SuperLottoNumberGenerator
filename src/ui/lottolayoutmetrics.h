#ifndef LOTTOLAYOUTMETRICS_H
#define LOTTOLAYOUTMETRICS_H

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

} // namespace LottoLayout

#endif // LOTTOLAYOUTMETRICS_H
