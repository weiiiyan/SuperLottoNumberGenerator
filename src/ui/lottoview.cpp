#include "lottoview.h"

#include <QWindow>

LottoView::LottoView(QWidget *parent)
    : QWidget(parent)
{
}

bool LottoView::isReady() const
{
    return true;
}

void LottoView::styleChanged(const QString &currentStyleId)
{
    Q_UNUSED(currentStyleId)
}

QMargins LottoView::windowSafeArea() const
{
    const QWidget *topLevel = window();
    QWindow *windowHandle = topLevel ? topLevel->windowHandle() : nullptr;
    return windowHandle ? windowHandle->safeAreaMargins() : QMargins();
}

QPixmap LottoView::snapshot() const
{
    // QWidget::grab() 为非 const(会重绘控件), 快照语义上是只读操作
    return const_cast<LottoView *>(this)->grab();
}
