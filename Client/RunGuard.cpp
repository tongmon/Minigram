#include "RunGuard.hpp"

RunGuard::RunGuard(const QString &key)
    : m_key(key),
      m_mem_lock_key(GenerateKeyHash(key, "minigram_mem_lock_key")),
      m_shared_mem_key(GenerateKeyHash(key, "minigram_shared_mem_key")),
      m_shared_mem(m_shared_mem_key),
      m_mem_lock(m_mem_lock_key, 1),
      m_hwnd_ptr{nullptr}
{
    m_mem_lock.acquire();
    {
        QSharedMemory fix(m_shared_mem_key);
        fix.attach();
    }
    m_mem_lock.release();
}

RunGuard::~RunGuard()
{
    Release();
}

void RunGuard::SetUniqueHwnd(HWND hwnd)
{
    *m_hwnd_ptr = hwnd;
}

bool RunGuard::IsAlreadyRunning()
{
    if (m_shared_mem.isAttached())
        return false;

    m_mem_lock.acquire();
    bool is_running = m_shared_mem.attach();
    if (is_running)
        m_shared_mem.detach();
    m_mem_lock.release();

    return is_running;
}

bool RunGuard::TryRun()
{
    if (IsAlreadyRunning())
        return false;

    m_mem_lock.acquire();
    bool ret = m_shared_mem.create(sizeof(quint64));

    m_file_map.setFileName("minigram/process_id");
    m_file_map.open(QIODevice::ReadWrite);
    if (ret)
        m_hwnd_ptr = reinterpret_cast<HWND *>(m_file_map.map(0, sizeof(HWND)));
    else
    {
        HWND *hwnd_ptr = nullptr;
        auto ret = m_file_map.read(reinterpret_cast<char *>(hwnd_ptr), sizeof(HWND));
        if (ret > 0)
        {
            ShowWindow(*hwnd_ptr, SW_SHOW);
            SendMessage(*hwnd_ptr, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        }
    }
    m_file_map.close();
    m_mem_lock.release();

    if (!ret)
    {
        Release();
        return false;
    }

    return true;
}

void RunGuard::Release()
{
    m_mem_lock.acquire();
    if (m_shared_mem.isAttached())
        m_shared_mem.detach();
    if (m_hwnd_ptr)
        m_file_map.unmap(reinterpret_cast<uchar *>(m_hwnd_ptr));
    m_mem_lock.release();
}

QString RunGuard::GenerateKeyHash(const QString &key, const QString &salt)
{
    QByteArray data;
    data.append(key.toUtf8());
    data.append(salt.toUtf8());
    data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    return data;
}
