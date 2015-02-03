TARGETS    = watch
watch_OBJS = main.o

ifndef TOPDIR
  TOPDIR = .
  include $(TOPDIR)/Makefile.include
endif
