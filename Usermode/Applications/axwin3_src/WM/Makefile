# Project: Acess2 GUI v3 Window Manager

-include ../../Makefile.cfg


DIR := Apps/AxWin/3.0
BIN := AxWinWM
OBJ := main.o input.o video.o ipc_acess.o
include common.mk

LDFLAGS += -lnet

-include ../../Makefile.tpl

.PHONY: install_resources

install: install_resources

install_%.sif: resources/.%.sif ;
	@echo [xCP] $(DISTROOT)/$(DIR)/$*.sif
	@$(xCP) $< $(DISTROOT)/$(DIR)/$*.sif

install_resources: $(IMG2SIF) $(addprefix install_,$(PNGIMAGES:%.png=%.sif))
