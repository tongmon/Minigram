#ifndef HEADER__FILE__RUNGUARD
#define HEADER__FILE__RUNGUARD

#include <QCryptographicHash>
#include <QSharedMemory>
#include <QString>
#include <QSystemSemaphore>

class RunGuard
{
    Q_DISABLE_COPY(RunGuard)

    const QString m_key;
    const QString m_mem_lock_key;
    const QString m_shared_mem_key;

    QSharedMemory m_shared_mem;
    QSystemSemaphore m_mem_lock;

    QString GenerateKeyHash(const QString &key, const QString &salt);
    bool IsAlreadyRunning();
    void Release();

  public:
    RunGuard(const QString &key);
    ~RunGuard();

    bool TryRun();
};

#endif /* HEADER__FILE__RUNGUARD */
