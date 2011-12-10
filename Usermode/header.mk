# Push State
x := $(x).x
DIR_$(x) := $(DIR)
DIR := $(dir $(lastword $(filter-out %/header.mk,$(MAKEFILE_LIST))))
PDIR := $(DIR_$(x))
EXTRA_DEP-$(DIR) := $(EXTRA_DEP-$(PDIR))

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR))
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR))
