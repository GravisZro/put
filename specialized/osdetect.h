#ifndef OSDETECT_H
#define OSDETECT_H

# include <unistd.h> // for POSIX option detection

/*
 * Free open source OS detection macros
 *--------------------------------------------------------
 * __linux__          // Linux          (version support)
 * __kfreebsd__       // kFreeBSD       (version support)
 * __FreeBSD__        // FreeBSD        (version support)
 * __NetBSD__         // NetBSD         (version support)
 * __OpenBSD__        // OpenBSD        (version support)
 * __DragonFly__      // DragonFly BSD
 * __minix__          // MINIX
 * __plan9__          // Plan 9
 * __ecos__           // eCos
 * __SYLLABLE__       // Syllable
 *
 *
 * Commercial open source OS detection macros
 *--------------------------------------------------------
 * __darwin__         // Darwin
 * __sunos__          // SunOS
 * __solaris__        // Solaris
 * __sco_openserver__ // SCO OpenServer
 * __bsdi__           // BSD/OS         (version support)
 *
 *
 * Commercial closed source OS detection macros
 *--------------------------------------------------------
 * __zos__            // z/OS
 * __tru64__          // Tru64 (OSF/1)
 * __hpux__           // HP-UX
 * __irix__           // IRIX
 * __unixware__       // UnixWare
 * __dynix__          // DYNIX
 * __mpeix__          // MPE/iX
 * __vms__            // VMS
 * __aix__            // AIX            (version support)
 * __dcosx__          // DC/OSx
 * __reliant_unix__   // Reliant UNIX
 * __interix__        // Interix
 * __OS2__            // OS/2
 * __ultrix__         // Ultrix
 * __dgux__           // DG/UX
 * __amigaos__        // AmigaOS
 * __QNX__            // QNX
 * __BEOS__           // BeOS
 * __MORPHOS__        // MorphOS
 * __Lynx__           // LynxOS
 * __nucleus__        // Nucleus RTOS
 * __VOS__            // Stratus VOS
 *
 *
 * OS version support macros
 *-------------------------------
 * KERNEL_VERSION_CODE      // value of the current kernel version
 * KERNEL_VERSION(x, y, z)  // generate value of a specific kernel version
 *
 */

// === Free open source ===

#if !defined(__linux__) && (defined(__linux) || defined (linux)) // Linux
# define __linux__

#elif !defined(__kfreebsd__) && defined(__FreeBSD_kernel__) && defined(__GLIBC__) // kFreeBSD
# define __kfreebsd__

#elif !defined(__minix__) && defined(__minix) // MINIX
# define __minix__

#elif !defined(__plan9__) && defined(EPLAN9) // Plan 9
# define __plan9__

#elif !defined(__ecos__) && defined(__ECOS) // eCos
# define __ecos__

// === Commercial open source ===
#elif !defined(__darwin__) && defined(__APPLE__) && defined(__MACH__) // Darwin
# define __darwin__
# define __unix__

#elif defined(__sun) || defined(sun)
# if !defined(__solaris__) && (defined(__SVR4) || defined(__svr4__)) // Solaris
#  define __solaris__
# elif !defined(__sunos__) // SunOS
#  define __sunos__
# endif

#elif !defined(__sco_openserver__) && defined(_SCO_DS) // SCO OpenServer
# define __sco_openserver__

// === Commercial closed source ===
#elif !defined(__zos__) && (defined(__MVS__) || defined(__HOS_MVS__) || defined(__TOS_MVS__)) // z/OS
# define __zos__

#elif !defined(__tru64__) && defined(__osf__) || defined(__osf) // Tru64 (OSF/1)
# define __tru64__

#elif !defined(__hpux__) && (defined(__hpux) || defined(hpux)) // HP-UX
# define __hpux__

#elif !defined(__irix__) && (defined(__sgi) || defined(sgi)) // IRIX
# define __irix__

#elif !defined(__unixware__) && (defined(_UNIXWARE7) || defined(sco)) // UnixWare
# define __unixware__

#elif !defined(__dynix__) && (defined(_SEQUENT_) || defined(sequent)) // DYNIX
# define __dynix__

#elif !defined(__mpeix__) && (defined(__mpexl) || defined(mpeix)) // MPE/iX
# define __mpeix__

#elif !defined(__vms__) && (defined(__VMS) || defined(VMS)) // VMS
# define __vms__

#elif !defined(__aix__) && defined(_AIX) // AIX
# define __aix__

#elif !defined(__dcosx__) && defined(pyr) // DC/OSx
# define __dcosx__

#elif !defined(__reliant_unix__) && defined(sinux) // Reliant UNIX
# define __reliant_unix__

#elif !defined(__interix__) && defined(__INTERIX) // Interix
# define __interix__

// redundant
#elif !defined(__OS2__) && (defined(__TOS_OS2__) || defined(_OS2) || defined(OS2)) // OS/2
# define __OS2__

#elif !defined(__ultrix__) && (defined(__ultrix) || defined(ultrix)) // Ultrix
# define __ultrix__

#elif !defined(__dgux__) && (defined(__DGUX__) || defined(DGUX)) // DG/UX
# define __dgux__

#elif !defined(__amigaos__) && defined(AMIGA) // AmigaOS
# define __amigaos__

#elif !defined(__QNX__) && defined(__QNXNTO__) // QNX
# define __QNX__

#endif

#if !defined(__unix__) && defined(__unix) // generic unix
# define __unix__
#endif

// version support

#if defined(__linux__) // Linux
# include <linux/version.h>
# define KERNEL_VERSION_CODE LINUX_VERSION_CODE

#elif defined(__aix__) // AIX
# define KERNEL_VERSION(a,b,c) (((a) * 10) + (b))
# if defined(_AIX43)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,3,0)
# elif defined(_AIX41)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,1,0)
# elif defined(_AIX3) || defined(_AIX32)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,2,0)
# endif

#elif defined(__FreeBSD__) // FreeBSD
# include <sys/param.h>
# if __FreeBSD_version >= 500000
#  define KERNEL_VERSION(a,b,c) (((a) * 100000) + ((b) * 1000) + ((c) * 10))
# else
#  define KERNEL_VERSION(a,b,c) (((a) * 100000) + ((b) * 10000) + ((c) * 1000))
# endif

# if __FreeBSD_version >= 220000
#  define KERNEL_VERSION_CODE __FreeBSD_version
# elif __FreeBSD_version >= 199511
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif __FreeBSD_version >= 199511
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif __FreeBSD_version >= 199511
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif __FreeBSD_version >= 199511
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif __FreeBSD_version >= 199612
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,7)
# elif __FreeBSD_version >= 199607
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,5)
# elif __FreeBSD_version >= 199511
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif __FreeBSD_version >= 199504
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,0,5)
# elif __FreeBSD_version >= 119411
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,0,0)
# endif

#elif defined(__NetBSD__) // NetBSD
# include <sys/param.h>
# define KERNEL_VERSION(a,b,c) ((((a) * 100000000) + (b) * 1000000) + (c) * 100)
# if defined(__NetBSD_Version__)
#  define KERNEL_VERSION_CODE __NetBSD_Version__
# elif defined(NetBSD1_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,4,0)
# elif defined(NetBSD1_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,3,0)
# elif defined(NetBSD1_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,2,0)
# elif defined(NetBSD1_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,1,0)
# elif defined(NetBSD1_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,0,0)
# elif defined(NetBSD0_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(0,9,0)
# elif defined(NetBSD0_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(0,8,0)
# elif defined(NetBSD0_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(0,7,0)
# elif defined(NetBSD0_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(0,6,0)
# elif defined(NetBSD0_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(0,5,0)
# endif

#elif defined(__OpenBSD__) // OpenBSD
# include <sys/param.h>
# define KERNEL_VERSION(a,b,c) (((a) * 10) + (b))
# if defined(OpenBSD7_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(7,0,0)
# elif defined(OpenBSD6_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,9,0)
# elif defined(OpenBSD6_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,8,0)
# elif defined(OpenBSD6_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,7,0)
# elif defined(OpenBSD6_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,6,0)
# elif defined(OpenBSD6_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,5,0)
# elif defined(OpenBSD6_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,4,0)
# elif defined(OpenBSD6_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,3,0)
# elif defined(OpenBSD6_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,2,0)
# elif defined(OpenBSD6_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,1,0)
# elif defined(OpenBSD6_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(6,0,0)
# elif defined(OpenBSD5_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,9,0)
# elif defined(OpenBSD5_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,8,0)
# elif defined(OpenBSD5_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,7,0)
# elif defined(OpenBSD5_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,6,0)
# elif defined(OpenBSD5_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,5,0)
# elif defined(OpenBSD5_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,4,0)
# elif defined(OpenBSD5_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,3,0)
# elif defined(OpenBSD5_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,2,0)
# elif defined(OpenBSD5_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,1,0)
# elif defined(OpenBSD5_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(5,0,0)
# elif defined(OpenBSD4_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,9,0)
# elif defined(OpenBSD4_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,8,0)
# elif defined(OpenBSD4_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,7,0)
# elif defined(OpenBSD4_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,6,0)
# elif defined(OpenBSD4_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,5,0)
# elif defined(OpenBSD4_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,4,0)
# elif defined(OpenBSD4_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,3,0)
# elif defined(OpenBSD4_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,2,0)
# elif defined(OpenBSD4_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,1,0)
# elif defined(OpenBSD4_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,0,0)
# elif defined(OpenBSD3_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,9,0)
# elif defined(OpenBSD3_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,8,0)
# elif defined(OpenBSD3_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,7,0)
# elif defined(OpenBSD3_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,6,0)
# elif defined(OpenBSD3_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,5,0)
# elif defined(OpenBSD3_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,4,0)
# elif defined(OpenBSD3_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,3,0)
# elif defined(OpenBSD3_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,2,0)
# elif defined(OpenBSD3_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,1,0)
# elif defined(OpenBSD3_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,0,0)
# elif defined(OpenBSD2_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,9,0)
# elif defined(OpenBSD2_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,8,0)
# elif defined(OpenBSD2_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,7,0)
# elif defined(OpenBSD2_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,6,0)
# elif defined(OpenBSD2_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,5,0)
# elif defined(OpenBSD2_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,4,0)
# elif defined(OpenBSD2_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,3,0)
# elif defined(OpenBSD2_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,2,0)
# elif defined(OpenBSD2_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,1,0)
# elif defined(OpenBSD2_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(2,0,0)
# elif defined(OpenBSD1_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(1,2,0)
# endif

#elif defined(__bsdi__) // BSD/OS
# define KERNEL_VERSION(a,b,c) (((a) * 10) + (b))
# if defined(BSD4_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,4,0)
# elif defined(BSD4_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,3,0)
# elif defined(BSD4_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,2,0)
# endif

#endif


#if !defined(KERNEL_VERSION_CODE)
# define KERNEL_VERSION_CODE 0
#endif

#if !defined(KERNEL_VERSION)
# define KERNEL_VERSION(a,b,c) 1
#endif


#endif // OSDETECT_H