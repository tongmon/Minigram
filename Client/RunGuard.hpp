#ifndef HEADER__FILE__RUNGUARD
#define HEADER__FILE__RUNGUARD

#include <QCryptographicHash>
#include <QFile>
#include <QSharedMemory>
#include <QString>
#include <QSystemSemaphore>

#include <Windows.h>

class RunGuard
{
    Q_DISABLE_COPY(RunGuard)

    const QString m_key;
    const QString m_mem_lock_key;
    const QString m_shared_mem_key;

    QSharedMemory m_shared_mem;
    QSystemSemaphore m_mem_lock;

    QFile m_file_map;
    HWND *m_hwnd_ptr;

    QString GenerateKeyHash(const QString &key, const QString &salt);
    bool IsAlreadyRunning();
    void Release();

  public:
    RunGuard(const QString &key);
    ~RunGuard();

    bool TryRun();
    void SetUniqueHwnd(HWND hwnd);
};

#endif /* HEADER__FILE__RUNGUARD */
