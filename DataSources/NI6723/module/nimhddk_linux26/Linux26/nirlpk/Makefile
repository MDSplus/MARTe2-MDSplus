#
#  Makefile
#     Makefile for NI MHDDK Kernel driver (nirplk)
#
#  (c) Copyright 2006,
#  National Instruments Corporation.
#  All Rights reserved.
#
#-------------------------------------------------

#-------------------------------------------------
# kernel configuration
#-------------------------------------------------

-include Makefile.in

#-------------------------------------------------
# build configuration
#-------------------------------------------------

OBJDIR := objects
PWD    := $(shell pwd)

MODULE_EXT    := .ko
MODULE_NAME   := nirlpk
MODULE_TARGET := $(MODULE_NAME)$(MODULE_EXT)

SOURCES := \
    $(OBJDIR)/nirlpk.c \
    $(OBJDIR)/nirlpk.h \
    $(OBJDIR)/ioctl.h

#-------------------------------------------------
# rules
#-------------------------------------------------

.PHONY:	all
all: mkobjdir $(OBJDIR)/$(MODULE_TARGET)

$(OBJDIR)/$(MODULE_TARGET) : $(SOURCES) $(OBJDIR)/Makefile
	@echo ""
	@$(MAKE) -C $(KERNELDIRECTORY) SUBDIRS=$(PWD)/$(OBJDIR) modules

$(OBJDIR)/%.c : %.c
	@echo "$@ <- $<"
	@cp $< $@

$(OBJDIR)/%.h : %.h
	@echo "$@ <- $<"
	@cp $< $@

$(OBJDIR)/Makefile :
	@echo "$@"
	@echo "" > $@
	@echo "EXTRA_CFLAGS += $(nirlpk_DEFINES)" >> $@
	@echo "obj-m := $(MODULE_NAME).o" >> $@
	@echo " " >> $@ 
	@echo "nirlpk-objs := "  >> $@
	@echo " " >> $@

Makefile.in:
	@echo "Makefile.in not found. Running ./configure ..."
	@./configure

.PHONY: mkobjdir
mkobjdir:
	$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

.PHONY: install
install:
	./install.sh $(OBJDIR)

.PHONY: uninstall
uninstall:
	@./uninstall.sh

.PHONY: clean
clean:
	$(RM) -rf $(OBJDIR)
	$(RM) Makefile.in
