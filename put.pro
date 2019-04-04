TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt
CONFIG += c++14
CONFIG += strict_c++
CONFIG += exceptions_off
CONFIG += rtti_off
# QT -= core gui


# FOR CLANG
#QMAKE_CXXFLAGS += -stdlib=libc++
#QMAKE_LFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -fconstexpr-depth=256
#QMAKE_CXXFLAGS += -fconstexpr-steps=900000000

# universal arguments
QMAKE_CXXFLAGS += -fno-rtti

QMAKE_CXXFLAGS_DEBUG += -O0 -g3
QMAKE_CXXFLAGS_RELEASE += -Os


#QMAKE_CXXFLAGS_RELEASE += -fno-threadsafe-statics
QMAKE_CXXFLAGS_RELEASE += -fno-asynchronous-unwind-tables
#QMAKE_CXXFLAGS_RELEASE += -fstack-protector-all
QMAKE_CXXFLAGS_RELEASE += -fstack-protector-strong
QMAKE_CXXFLAGS_RELEASE += -fstack-clash-protection

# optimizations
QMAKE_CXXFLAGS_RELEASE += -fdata-sections
QMAKE_CXXFLAGS_RELEASE += -ffunction-sections
QMAKE_LFLAGS_RELEASE += -Wl,--gc-sections

# libraries
LIBS += -lrt

# defines
QMAKE_CXXFLAGS_DEBUG += -DDEBUG
QMAKE_CXXFLAGS_RELEASE += -DRELEASE

#DEFINES += DISABLE_INTERRUPTED_WRAPPER
#DEFINES += SINGLE_THREADED_APPLICATION
#DEFINES += FORCE_POSIX_TIMERS
#DEFINES += FORCE_POSIX_POLL
#DEFINES += FORCE_POSIX_MUTEXES
#DEFINES += FORCE_PROCESS_POLLING

#LIBS += -lpthread
experimental {
#QMAKE_CXXFLAGS += -stdlib=libc++
QMAKE_CXXFLAGS += -nostdinc
INCLUDEPATH += /usr/include/x86_64-linux-musl
INCLUDEPATH += /usr/include/c++/v1
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/x86_64-linux-gnu
QMAKE_LFLAGS += -L/usr/lib/x86_64-linux-musl -dynamic-linker /lib/ld-musl-x86_64.so.1
LIBS += -lc++
}

HEADERS += \
    application.h \
    childprocess.h \
    object.h \
    socket.h \
    cxxutils/configmanip.h \
    cxxutils/cstringarray.h \
    cxxutils/error_helpers.h \
    cxxutils/hashing.h \
    cxxutils/pipedspawn.h \
    cxxutils/misc_helpers.h \
    cxxutils/posix_helpers.h \
    cxxutils/socket_helpers.h \
    cxxutils/syslogstream.h \
    cxxutils/nullable.h \
    cxxutils/sharedmem.h \
    cxxutils/pipedfork.h \
    cxxutils/signal_helpers.h \
    cxxutils/stringtoken.h \
    cxxutils/translate.h \
    cxxutils/vfifo.h \
    cxxutils/vterm.h \
    specialized/blockdevices.h \
    specialized/blockinfo.h \
    specialized/capabilities.h \
    specialized/fileevent.h \
    specialized/module.h \
    specialized/mountevent.h \
    specialized/mutex.h \
    specialized/osdetect.h \
    specialized/pollevent.h \
    specialized/proclist.h \
    specialized/eventbackend.h \
    specialized/fstable.h \
    specialized/mount.h \
    specialized/mountpoints.h \
    specialized/peercred.h \
    specialized/processevent.h \
    specialized/procstat.h \
    specialized/timerevent.h

SOURCES += \
    application.cpp \
    childprocess.cpp \
    socket.cpp \
    cxxutils/configmanip.cpp \
    cxxutils/stringtoken.cpp \
    cxxutils/translate.cpp \
    cxxutils/syslogstream.cpp \
    cxxutils/vfifo.cpp \
    specialized/blockdevices.cpp \
    specialized/blockinfo.cpp \
    specialized/eventbackend.cpp \
    specialized/fstable.cpp \
    specialized/mount.cpp \
    specialized/mountpoints.cpp \
    specialized/pollevent.cpp \
    specialized/proclist.cpp \
    specialized/fileevent.cpp \
    specialized/module.cpp \
    specialized/mountevent.cpp \
    specialized/mutex.cpp \
    specialized/peercred.cpp \
    specialized/processevent.cpp \
    specialized/procstat.cpp \
    specialized/timerevent.cpp
