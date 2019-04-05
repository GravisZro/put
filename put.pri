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

tui:HEADERS += \
    tui/event.h \
    tui/keyboard.h \
    tui/layout.h \
    tui/screen.h \
    tui/tuiutils.h \
    tui/sizepolicy.h \
    tui/layoutitem.h \
    tui/tuitypes.h \
    tui/widget.h \
    tui/window.h

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

tui:SOURCES += \
    tui/event.cpp \
    tui/keyboard.cpp \
    tui/layout.cpp \
    tui/screen.cpp \
    tui/tuiutils.cpp \
    tui/layoutitem.cpp \
    tui/widget.cpp \
    tui/window.cpp
