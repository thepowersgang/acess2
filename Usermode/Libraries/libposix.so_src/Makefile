# Acess2 POSIX Emulation
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Wextra
ASFLAGS  +=
LDFLAGS  += -nostdlib
PRELINK  += $(CRTI) $(CRTBEGINS) $(CRT0S)
LIBS     += -lc $(CRTENDS) $(CRTN)

OBJ  = main.o unistd.o dirent.o stat.o utmpx.o termios.o
OBJ += pwd.o syslog.o sys_time.o sys_ioctl.o sys_resource.o
OBJ += fcntl.o clocks.o sys_wait.o unistd_crypt.o
OBJ += grp.o pty.o mktemp.o utime.o getopt.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libposix.so

include ../Makefile.tpl

