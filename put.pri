!defined(PUTPATH, var) {
PUTPATH = $$_PRO_FILE_PWD_/put
}

QMAKE_INCDIR += $$PUTPATH

HEADERS += \
    $$PUTPATH/application.h \
    $$PUTPATH/childprocess.h \
    $$PUTPATH/object.h \
    $$PUTPATH/socket.h \
    $$PUTPATH/cxxutils/configmanip.h \
    $$PUTPATH/cxxutils/cstringarray.h \
    $$PUTPATH/cxxutils/error_helpers.h \
    $$PUTPATH/cxxutils/hashing.h \
    $$PUTPATH/cxxutils/pipedspawn.h \
    $$PUTPATH/cxxutils/misc_helpers.h \
    $$PUTPATH/cxxutils/posix_helpers.h \
    $$PUTPATH/cxxutils/socket_helpers.h \
    $$PUTPATH/cxxutils/syslogstream.h \
    $$PUTPATH/cxxutils/nullable.h \
    $$PUTPATH/cxxutils/sharedmem.h \
    $$PUTPATH/cxxutils/pipedfork.h \
    $$PUTPATH/cxxutils/signal_helpers.h \
    $$PUTPATH/cxxutils/stringtoken.h \
    $$PUTPATH/cxxutils/translate.h \
    $$PUTPATH/cxxutils/vfifo.h \
    $$PUTPATH/cxxutils/vterm.h \
    $$PUTPATH/specialized/blockdevices.h \
    $$PUTPATH/specialized/blockinfo.h \
    $$PUTPATH/specialized/capabilities.h \
    $$PUTPATH/specialized/fileevent.h \
    $$PUTPATH/specialized/module.h \
    $$PUTPATH/specialized/mountevent.h \
    $$PUTPATH/specialized/mutex.h \
    $$PUTPATH/specialized/osdetect.h \
    $$PUTPATH/specialized/pollevent.h \
    $$PUTPATH/specialized/proclist.h \
    $$PUTPATH/specialized/eventbackend.h \
    $$PUTPATH/specialized/fstable.h \
    $$PUTPATH/specialized/mount.h \
    $$PUTPATH/specialized/mountpoints.h \
    $$PUTPATH/specialized/peercred.h \
    $$PUTPATH/specialized/processevent.h \
    $$PUTPATH/specialized/procstat.h \
    $$PUTPATH/specialized/timerevent.h

tui:HEADERS += \
    $$PUTPATH/tui/event.h \
    $$PUTPATH/tui/keyboard.h \
    $$PUTPATH/tui/layout.h \
    $$PUTPATH/tui/screen.h \
    $$PUTPATH/tui/tuiutils.h \
    $$PUTPATH/tui/sizepolicy.h \
    $$PUTPATH/tui/layoutitem.h \
    $$PUTPATH/tui/tuitypes.h \
    $$PUTPATH/tui/widget.h \
    $$PUTPATH/tui/window.h

SOURCES += \
    $$PUTPATH/application.cpp \
    $$PUTPATH/childprocess.cpp \
    $$PUTPATH/socket.cpp \
    $$PUTPATH/cxxutils/configmanip.cpp \
    $$PUTPATH/cxxutils/stringtoken.cpp \
    $$PUTPATH/cxxutils/translate.cpp \
    $$PUTPATH/cxxutils/syslogstream.cpp \
    $$PUTPATH/cxxutils/vfifo.cpp \
    $$PUTPATH/specialized/blockdevices.cpp \
    $$PUTPATH/specialized/blockinfo.cpp \
    $$PUTPATH/specialized/eventbackend.cpp \
    $$PUTPATH/specialized/fstable.cpp \
    $$PUTPATH/specialized/mount.cpp \
    $$PUTPATH/specialized/mountpoints.cpp \
    $$PUTPATH/specialized/pollevent.cpp \
    $$PUTPATH/specialized/proclist.cpp \
    $$PUTPATH/specialized/fileevent.cpp \
    $$PUTPATH/specialized/module.cpp \
    $$PUTPATH/specialized/mountevent.cpp \
    $$PUTPATH/specialized/mutex.cpp \
    $$PUTPATH/specialized/peercred.cpp \
    $$PUTPATH/specialized/processevent.cpp \
    $$PUTPATH/specialized/procstat.cpp \
    $$PUTPATH/specialized/timerevent.cpp

tui:SOURCES += \
    $$PUTPATH/tui/event.cpp \
    $$PUTPATH/tui/keyboard.cpp \
    $$PUTPATH/tui/layout.cpp \
    $$PUTPATH/tui/screen.cpp \
    $$PUTPATH/tui/tuiutils.cpp \
    $$PUTPATH/tui/layoutitem.cpp \
    $$PUTPATH/tui/widget.cpp \
    $$PUTPATH/tui/window.cpp
