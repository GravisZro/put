#ifndef PROCSTAT_H
#define PROCSTAT_H

// STL
#include <string>

// POSIX
#include <sys/types.h>

enum ExecutionState : char
{
  Running = 'R',                // actively executing
  WaitingInterruptable = 'S',   // sleeping in an interruptible wait
  WaitingUninterruptable = 'D', // sleeping in an uninterruptible wait
  Zombie = 'Z',                 // zombie process
  Stopped = 'T',                // stopped/tracing execution
};
typedef int signals_t;

struct process_state_t
{
  pid_t process_id;         // process id
  std::string filename;     // filename of the executable
  ExecutionState state;     // process execution state
  pid_t parent_process_id;  // process id of the parent process
  pid_t process_group_id;   // process group id
  pid_t session_id;         // session id

  int tty; // number of the tty the process running on
//  pid_t tty_process_group_id;   // tty process group id

  signals_t signals_pending;
  signals_t signals_blocked;
  signals_t signals_ignored;
  signals_t signals_caught;

//  int priority_type;  // priority level
  int priority_value; // nice value
//  int policy;         // scheduling policy (man sched_setscheduler)

//  int thread_count;   // number of threads used by the process
//  int cpu_affinity;   // number of the cpu core used for execution

#if 0
  tty_nr        tty the process uses
  tty_pgrp      pgrp of the tty

  flags         task flags
  min_flt       number of minor faults
//  cmin_flt      number of minor faults with child's
  maj_flt       number of major faults
//  cmaj_flt      number of major faults with child's
  utime         user mode jiffies
  stime         kernel mode jiffies
//  cutime        user mode jiffies with child's
//  cstime        kernel mode jiffies with child's

  priority      priority level
  nice          nice level
  num_threads   number of threads

  it_real_value	(obsolete, always 0)

  start_time    time the process started after system boot
  vsize         virtual memory size
  rss           resident set memory size
  rsslim        current limit in bytes on the rss

  start_code    address above which program text can run
  end_code      address below which program text can run
  start_stack   address of the start of the main process stack
  esp           current value of ESP
  eip           current value of EIP

  pending       bitmap of pending signals
  blocked       bitmap of blocked signals
  sigign        bitmap of ignored signals
  sigcatch      bitmap of caught signals

  0		(place holder, used to be the wchan address, use /proc/PID/wchan instead)
  0             (place holder)
  0             (place holder)
  exit_signal   signal to send to parent thread on exit
  task_cpu      which CPU the task is scheduled on
  rt_priority   realtime priority
  policy        scheduling policy (man sched_setscheduler)
  blkio_ticks   time spent waiting for block IO
  gtime         guest time of the task in jiffies
  cgtime        guest time of the task children in jiffies
  start_data    address above which program data+bss is placed
  end_data      address below which program data+bss is placed
  start_brk     address above which program heap can be expanded with brk()

  arg_start     address above which program command line is placed
  arg_end       address below which program command line is placed
  env_start     address above which program environment is placed
  env_end       address below which program environment is placed

//  exit_code     the thread's exit_code in the form reported by the waitpid system call

  //ORLY?

  pid_t pid;


  typedef long long int num;

  num pid;
  char tcomm[PATH_MAX];
  char state;

  num ppid;
  num pgid;
  num sid;
  num tty_nr;
  num tty_pgrp;

  num flags;
  num min_flt;
  num cmin_flt;
  num maj_flt;
  num cmaj_flt;
  num utime;
  num stimev;

  num cutime;
  num cstime;
  num priority;
  num nicev;
  num num_threads;
  num it_real_value;

  unsigned long long start_time;

  num vsize;
  num rss;
  num rsslim;
  num start_code;
  num end_code;
  num start_stack;
  num esp;
  num eip;

  num pending;
  num blocked;
  num sigign;
  num sigcatch;
  num wchan;
  num zero1;
  num zero2;
  num exit_signal;
  num cpu;
  num rt_priority;
  num policy;

  long tickspersec;
#endif

};


bool procstat(pid_t pid, process_state_t& data) noexcept;

#endif // PROCSTAT_H
