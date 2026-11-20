// UI 层测试公共辅助: 端口替身(偏好存储/风格宿主/视图)
// 各替身只实现接口, 不参与生产构建

#ifndef UITESTHELPERS_H
#define UITESTHELPERS_H

#include <QStringList>
#include <QVector>

#include "lottostylehost.h"
#include "lottostylestore.h"
#include "lottoview.h"

/*! 内存版风格偏好端口: 记录当前值与保存次数 */
class FakeLottoStyleStore : public LottoStyleStore
{
public:
    QString storedId;        /*!< 当前值, 空表示未设置 */
    int saveCount = 0;       /*!< saveStyleId() 调用次数 */

    QString loadStyleId() const override { return storedId; }
    void saveStyleId(const QString &id) override { storedId = id; ++saveCount; }
};

/*! 记录切换请求的宿主端口替身 */
class FakeStyleHost : public LottoStyleHost
{
public:
    QVector<LottoStyleInfo> styles;   /*!< 对外提供的风格清单 */
    QString currentId;                /*!< 当前风格 id */
    QStringList requested;            /*!< 历次 requestSwitchTo() 的 id */

    QVector<LottoStyleInfo> availableStyles() const override { return styles; }
    QString currentStyleId() const override { return currentId; }
    void requestSwitchTo(const QString &id) override { requested << id; }
};

/*! 无 Q_OBJECT 的视图替身: 记录渲染与通知, 可模拟就绪与点击 */
class FakeLottoView : public LottoView
{
public:
    FakeLottoView(LottoStyleHost *host, const QString &id, QWidget *parent = nullptr)
        : LottoView(parent), m_host(host), m_id(id) {}

    QString styleId() const override { return m_id; }
    void renderState(const LottoViewState &state) override
    {
        m_states.append(state);
        m_lastState = state;
    }
    bool isReady() const override { return m_ready; }
    void styleChanged(const QString &id) override
    {
        ++m_styleChangedCount;
        m_currentStyleId = id;
    }

    LottoStyleHost *host() const { return m_host; }
    int stateCount() const { return m_states.size(); }
    LottoViewState lastState() const { return m_lastState; }
    int styleChangedCount() const { return m_styleChangedCount; }
    QString currentStyleId() const { return m_currentStyleId; }
    void setReady(bool ready) { m_ready = ready; }
    void simulateGenerate() { emit generateRequested(); }
    void simulateLock() { emit lockRequested(); }

private:
    LottoStyleHost *m_host = nullptr;  /*!< 注入的宿主端口 */
    QString m_id;                      /*!< 本视图风格 id */
    bool m_ready = true;               /*!< isReady() 返回值 */
    QVector<LottoViewState> m_states;  /*!< 历次 renderState() 的状态 */
    LottoViewState m_lastState;        /*!< 最近一次状态 */
    int m_styleChangedCount = 0;       /*!< styleChanged() 调用次数 */
    QString m_currentStyleId;          /*!< 最近一次 styleChanged() 的 id */
};

#endif // UITESTHELPERS_H
