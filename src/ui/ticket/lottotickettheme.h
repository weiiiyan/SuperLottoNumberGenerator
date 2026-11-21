#ifndef LOTTOTICKETTHEME_H
#define LOTTOTICKETTHEME_H

#include <QColor>

/*!
 * \brief 票据主题色令牌(与 style.qss 中的同名色值保持一致)
 *
 * QSS 不能引用 C++ 常量, 因此自绘控件(骑缝线/条形码/印章/纸面)统一从
 * 本头取色; 修改配色时两处需同步。
 */
namespace LottoTicketTheme {

inline const QColor Stage(0xE4, 0xDE, 0xD1);      /*!< 应用外的展示底 */
inline const QColor Paper(0xF7, 0xF2, 0xE6);      /*!< 票面主色 */
inline const QColor Paper2(0xEF, 0xE7, 0xD6);     /*!< 票面渐暗 */
inline const QColor PaperTop(0xFB, 0xF7, 0xEC);   /*!< 票面顶部 */
inline const QColor PaperDeep(0xF2, 0xEB, 0xDA);  /*!< 票面底部 */
inline const QColor PaperGrain(120, 100, 60, 8);  /*!< 纸纹(每 3px 一条) */
inline const QColor Ink(0x1C, 0x1B, 0x19);        /*!< 标题 / 边框 / 正文 */
inline const QColor InkSoft(0x6A, 0x65, 0x5B);    /*!< 标签 / 票号 */
inline const QColor Rule(0xD6, 0xCC, 0xB6);       /*!< 虚线分隔 */
inline const QColor Front(0xC1, 0x12, 0x1F);      /*!< 前区球 / 主按钮 */
inline const QColor Back(0x0B, 0x6F, 0xA4);       /*!< 后区球 / 焦点环 */

} // namespace LottoTicketTheme

#endif // LOTTOTICKETTHEME_H
