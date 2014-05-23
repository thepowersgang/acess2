
include ../../Makefile.cfg

CPPFLAGS += -Iinclude/
OBJ := main.o ipc.o CConfig.o video.o input.o timing.o
OBJ += compositor.o CWindow.o
OBJ += serialisation.o CClient.o
OBJ += CIPCChannel_AcessIPCPipe.o
OBJ += CRect.o CSurface.o
BIN := AxWinServer

LDFLAGS += -lc++

include ../../Makefile.tpl
