#if defined(__linux__) && !defined(_POSIX_DRAFT_1E)
#define _POSIX_DRAFT_1E
#endif

#if defined(_POSIX_DRAFT_1E) && !defined(CAPABILITIES_H)
#define CAPABILITIES_H

#include <sys/capability.h>
#include <sys/prctl.h>

#if defined(__linux__)
#include <linux/capability.h>

#if defined(_LINUX_CAPABILITY_VERSION_3)
# define CAPABILITY_VERSION _LINUX_CAPABILITY_VERSION_3
# define CAPABILITY_U32S    _LINUX_CAPABILITY_U32S_3
#elif defined(_LINUX_CAPABILITY_VERSION_2)
# define CAPABILITY_VERSION _LINUX_CAPABILITY_VERSION_2
# define CAPABILITY_U32S    _LINUX_CAPABILITY_U32S_2
#elif defined(_LINUX_CAPABILITY_VERSION_1)
# define CAPABILITY_VERSION _LINUX_CAPABILITY_VERSION_1
# define CAPABILITY_U32S    _LINUX_CAPABILITY_U32S_1
#elif defined(_LINUX_CAPABILITY_VERSION)
# define CAPABILITY_VERSION _LINUX_CAPABILITY_VERSION
# define CAPABILITY_U32S    _LINUX_CAPABILITY_U32S
#endif

#define CAPTYPE_COUNT  3
#endif

enum class capflag : uint32_t
{
  chown             = CAP_CHOWN,
  dac_override      = CAP_DAC_OVERRIDE,
  dac_read_search   = CAP_DAC_READ_SEARCH,
  fowner            = CAP_FOWNER,
  fsetid            = CAP_FSETID,
  kill              = CAP_KILL,
  setgid            = CAP_SETGID,
  setuid            = CAP_SETUID,
  setpcap           = CAP_SETPCAP,
  linux_immutable   = CAP_LINUX_IMMUTABLE,
  net_bind_service  = CAP_NET_BIND_SERVICE,
  net_broadcast     = CAP_NET_BROADCAST,
  net_admin         = CAP_NET_ADMIN,
  net_raw           = CAP_NET_RAW,
  ipc_lock          = CAP_IPC_LOCK,
  ipc_owner         = CAP_IPC_OWNER,
  sys_module        = CAP_SYS_MODULE,
  sys_rawio         = CAP_SYS_RAWIO,
  sys_chroot        = CAP_SYS_CHROOT,
  sys_ptrace        = CAP_SYS_PTRACE,
  sys_pacct         = CAP_SYS_PACCT,
  sys_admin         = CAP_SYS_ADMIN,
  sys_boot          = CAP_SYS_BOOT,
  sys_nice          = CAP_SYS_NICE,
  sys_resouce       = CAP_SYS_RESOURCE,
  sys_time          = CAP_SYS_TIME,
  sys_tty_config    = CAP_SYS_TTY_CONFIG,
  mknod             = CAP_MKNOD,
  lease             = CAP_LEASE,
  audit_write       = CAP_AUDIT_WRITE,
  audit_control     = CAP_AUDIT_CONTROL,
  setfcap           = CAP_SETFCAP,
  mac_override      = CAP_MAC_OVERRIDE,
  mac_admin         = CAP_MAC_ADMIN,
  syslog            = CAP_SYSLOG,
  wake_alarm        = CAP_WAKE_ALARM,
  block_suspend     = CAP_BLOCK_SUSPEND,
  audit_read        = CAP_AUDIT_READ,
};

struct capability_t
{
  uint32_t* data;
  capability_t(uint32_t* d) noexcept : data(d) { }

#if CAPABILITY_U32S == 2
  uint64_t value(void) const noexcept { return (uint64_t(data[0]) << 32) + data[CAPTYPE_COUNT]; }
#else
  uint64_t value(void) const noexcept { return *data; }
#endif

  inline bool          isset (capflag flag) const noexcept { return data[(uint32_t(flag) >> 5) * CAPTYPE_COUNT] &   (1 << (uint32_t(flag) & 0x1F)); }
  inline capability_t& set   (capflag flag) noexcept { data[(uint32_t(flag) >> 5) * CAPTYPE_COUNT] |=  (1 << (uint32_t(flag) & 0x1F)); return *this; }
  inline capability_t& unset (capflag flag) noexcept { data[(uint32_t(flag) >> 5) * CAPTYPE_COUNT] &= ~(1 << (uint32_t(flag) & 0x1F)); return *this; }
  inline capability_t& toggle(capflag flag) noexcept { data[(uint32_t(flag) >> 5) * CAPTYPE_COUNT] ^=  (1 << (uint32_t(flag) & 0x1F)); return *this; }
};

struct capability_data_t
{
  struct
  {
    uint32_t version;
    pid_t pid;
  } head;

  uint32_t data[CAPABILITY_U32S * CAPTYPE_COUNT];

  capability_t effective;
  capability_t permitted;
  capability_t inheritable;

  operator cap_user_header_t(void) noexcept { return reinterpret_cast<cap_user_header_t>(&head); }
  operator cap_user_data_t  (void) noexcept { return reinterpret_cast<cap_user_data_t  >(data ); }

  capability_data_t(void) noexcept
    : effective  (data + CAP_EFFECTIVE),
      permitted  (data + CAP_PERMITTED),
      inheritable(data + CAP_INHERITABLE)
  {
    std::memset(data, 0, sizeof(data)); // zero out memory
    head.version = CAPABILITY_VERSION; // set version number
    head.pid = 0;
    if(::capget(*this, nullptr)) // load the kernel-capability version???
      fprintf(stderr, "failed to init: %s\n", strerror(errno));
  }
};

#endif // CAPABILITIES_H
