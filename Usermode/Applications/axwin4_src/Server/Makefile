
include ../../Makefile.cfg

DIR := Apps/AxWin/4.0

CPPFLAGS += -Iinclude/ -I../Common/include/
OBJ := main.o ipc.o CConfig.o video.o input.o timing.o
OBJ += compositor.o CWindow.o
OBJ += Common__serialisation.o
OBJ += CClient.o
OBJ += CIPCChannel_AcessIPCPipe.o
OBJ += CRect.o CSurface.o
OBJ += draw_control.o
BIN := AxWinServer

LIBS += -lc++
#CXXFLAGS += -O3
USE_CXX_LINK = 1 

include ../../Makefile.tpl

$(_OBJPREFIX)Common__%.o: ../Common/%.cpp
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -MQ $@ -MP -MD -MF $(@:%.o=%.dep)

