# run `make DEF=...` to add extra defines
PROGRAM := tsys_daemon
LDFLAGS := -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,--discard-all
LDFLAGS += -lm -pthread
SRCS := $(wildcard *.c)
DEFINES := $(DEF) -D_GNU_SOURCE  -D_XOPEN_SOURCE=1111
#DEFINES += -DEBUG
OBJDIR := mk
CFLAGS += -Wall -Wextra -Werror -O2 -Wno-trampolines -std=gnu99
OBJS := $(addprefix $(OBJDIR)/, $(SRCS:%.c=%.o))
CC  = gcc
CPP = g++


all : $(OBJDIR) $(PROGRAM)

$(PROGRAM) : $(OBJS)
	@echo -e "\t\tLD $(PROGRAM)"
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROGRAM)

$(OBJDIR):
	mkdir $(OBJDIR)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

$(OBJDIR)/%.o: %.c
	@echo -e "\t\tCC $<"
	$(CC) -MD -c $(LDFLAGS) $(CFLAGS) $(DEFINES) -o $@ $<

clean:
	@echo -e "\t\tCLEAN"
	@rm -f $(OBJS) $(DEPS)
	@rmdir $(OBJDIR) 2>/dev/null || true

xclean: clean
	@rm -f $(PROGRAM)

gentags:
	CFLAGS="$(CFLAGS) $(DEFINES)" geany -g $(PROGRAM).c.tags *[hc] 2>/dev/null

.PHONY: gentags clean xclean
