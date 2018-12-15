#ifndef OSDETECT_H
#define OSDETECT_H

# include <unistd.h> // for POSIX option detection

/*
 * Free open source OS detection macros
 *-------------------------------------------------------*
 * __linux__           | Linux          | VS | *nix      |
 * __kfreebsd__        | kFreeBSD       | VS | BSD       |
 * __FreeBSD__         | FreeBSD        | VS | BSD       |
 * __NetBSD__          | NetBSD         | VS | BSD       |
 * __OpenBSD__         | OpenBSD        | VS | BSD       |
 * __DragonFly__       | DragonFly BSD  |    | BSD       |
 * __minix__           | MINIX          |    | *nix      |
 * __plan9__           | Plan 9         |    | *nix      |
 * __ecos__            | eCos           |    | alien/PC  |
 * __SYLLABLE__        | Syllable       |    | alien     |
 *-------------------------------------------------------*
 *
 * Commercial open source OS detection macros
 *-------------------------------------------------------*
 * __darwin__          | Darwin         | VS | BSD       |
 * __sunos__           | SunOS          |    | BSD       |
 * __solaris__         | Solaris        |    | SysV      |
 * __sco_openserver__  | SCO OpenServer |    | SysV->BSD |
 * __bsdi__            | BSD/OS         | VS | *BSD      |
 *-------------------------------------------------------*
 *
 * Commercial closed source OS detection macros
 *-------------------------------------------------------*
 * __zos__             | z/OS           |    | alien/PC  |
 * __tru64__           | Tru64 (OSF/1)  |    | BSD       |
 * __hpux__            | HP-UX          |    | SysV      |
 * __irix__            | IRIX           |    | SysV/BSD  |
 * __unixware__        | UnixWare       |    | SysV      |
 * __dynix__           | DYNIX          |    | BSD->SysV |
 * __mpeix__           | MPE/iX         |    | alien/PC  |
 * __vms__             | VMS            |    | alien/PC  |
 * __aix__             | AIX            | VS | SysV/BSD  |
 * __dcosx__           | DC/OSx         |    | SysV      |
 * __reliant_unix__    | Reliant UNIX   |    | SysV      |
 * __interix__         | Interix        |    | alien/PC  |
 * __OS2__             | OS/2           |    | alien     |
 * __ultrix__          | Ultrix         |    | SysV/BSD  |
 * __dgux__            | DG/UX          |    | SysV/BSD  |
 * __amigaos__         | AmigaOS        |    | alien     |
 * __QNX__             | QNX            |    | alien/PC  |
 * __BEOS__            | BeOS           |    | alien     |
 * __MORPHOS__         | MorphOS        |    | alien     |
 * __Lynx__            | LynxOS         |    | *nix      |
 * __nucleus__         | Nucleus RTOS   |    | alien/PC  |
 * __VOS__             | Stratus VOS    |    | alien     |
 *-------------------------------------------------------*
 *
 * Notes
 *-------------------------------------------------------------
 * "VS"        - Version support indicating the ability to use the OS version support
 *               perprocessor macros in applications
 *
 * "*nix"      - An OS modeled after UNIX a.k.a. a UNIX clone
 * "SysV"      - Based on UNIX System V
 * "BSD"       - Based on BSD
 * "SysV/BSD"  - Based on UNIX System V but adds BSD extensions
 * "BSD->SysV" - Earlier versions based on BSD but later version based on UNIX System V
 * "SysV->BSD" - Earlier versions based on UNIX System V but later version based on BSD
 * "alien"     - An OS not based on or modeled after UNIX
 * "alien/PC"  - An OS not based on or modeled after UNIX but has a POSIX compatibility
 *               layer that enables POSIX functionality
 *
 *
 * OS version support macros
 *-------------------------------------------------------------
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

#elif defined(__darwin__) // Darwin
# include <AvailabilityMacros.h>
# define KERNEL_VERSION(a,b,c) (((a) * 100) + (b*10))
# if defined(MAC_OS_X_VERSION_10_9)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,9,0)
# elif defined(MAC_OS_X_VERSION_10_8)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,8,0)
# elif defined(MAC_OS_X_VERSION_10_7)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,7,0)
# elif defined(MAC_OS_X_VERSION_10_6)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,6,0)
# elif defined(MAC_OS_X_VERSION_10_5)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,5,0)
# elif defined(MAC_OS_X_VERSION_10_4)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,4,0)
# elif defined(MAC_OS_X_VERSION_10_3)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,3,0)
# elif defined(MAC_OS_X_VERSION_10_2)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,2,0)
# elif defined(MAC_OS_X_VERSION_10_1)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,1,0)
# elif defined(MAC_OS_X_VERSION_10_0)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(10,0,0)
# endif

#elif defined(__aix__) // AIX
# define KERNEL_VERSION(a,b,c) (((a) * 10) + (b))
# if defined(_AIX43)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,3,0)
# elif defined(_AIX41)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(4,1,0)
# elif defined(_AIX3) || defined(_AIX32)
#  define KERNEL_VERSION_CODE KERNEL_VERSION(3,2,0)
# endif

#elif defined(UNAME_KERNEL_MAJOR) && defined(UNAME_KERNEL_MINOR) && defined(UNAME_KERNEL_RELEASE)
# define KERNEL_VERSION(a,b,c) ((((a) * 1000000) + (b) * 1000) + (c))
# define KERNEL_VERSION_CODE  KERNEL_VERSION(UNAME_KERNEL_MAJOR, UNAME_KERNEL_MINOR, UNAME_KERNEL_RELEASE)
#endif


#if !defined(KERNEL_VERSION_CODE)
# define KERNEL_VERSION_CODE 0
#endif

#if !defined(KERNEL_VERSION)
# define KERNEL_VERSION(a,b,c) 1
#endif


#endif // OSDETECT_H
