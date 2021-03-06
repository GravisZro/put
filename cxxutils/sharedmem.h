#ifndef SHAREDMEM_H
#define SHAREDMEM_H

// POSIX
#include <sys/ipc.h>
#include <sys/shm.h>

// PUT
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/vterm.h>



#if _XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L

class sharedmem_t
{
public:
  sharedmem_t(posix::size_t size)
    : m_size(size)
  {
    m_shm_id = ::shmget(IPC_PRIVATE, m_size, IPC_CREAT | SHM_R | SHM_W);
    m_mem = reinterpret_cast<uint8_t*>(::shmat(m_shm_id, NULL, 0));
    m_rofile = ::fmemopen(::shmat(m_shm_id, NULL, SHM_RDONLY), m_size, "r");
  }

  ~sharedmem_t(void)
  {
    posix::fclose(m_rofile);
    ::shmctl(m_shm_id, IPC_RMID, NULL);
  }

  operator posix::fd_t(void) { return ::fileno(m_rofile); }

  uint8_t& operator [](posix::size_t pos)
  {
    flaw(pos >= m_size,
         terminal::critical,
         posix::exit(int(posix::errc::bad_address)),
         m_mem[0],
        "Attempted to access memory out of bounds of the shared memory type!")
    return m_mem[pos];
  }

private:
  posix::fd_t m_shm_id;
  posix::size_t m_size;
  uint8_t* m_mem;
  posix::FILE* m_rofile;
};

#endif

#endif // SHAREDMEM_H

