BINARY		= tsys01
BOOTPORT	?= /dev/ttyUSB0
BOOTSPEED	?= 57600
# MCU FAMILY
FAMILY		= F0
# MCU code
MCU			= F042x6
# hardware definitions
DEFS		:= -DUSARTNUM=1 -DI2CPINS=67
#DEFS		+= -DCHECK_TMOUT
#DEFS		+= -DEBUG
# change this linking script depending on particular MCU model,
# for example, if you have STM32F103VBT6, you should write:
LDSCRIPT	= stm32f042x6.ld

# autoincremental version & build date
VERSION_FILE := version.inc
NEXTVER := $(shell expr $$(awk '/#define BUILD_NUMBER/' $(VERSION_FILE) | tr -cd "[0-9]") + 1)
BUILDDATE := $(shell date +%Y-%m-%d)

INDEPENDENT_HEADERS=

FP_FLAGS	?= -msoft-float
ASM_FLAGS	= -mthumb -mcpu=cortex-m0 -march=armv6-m -mtune=cortex-m0
ARCH_FLAGS	= $(ASM_FLAGS) $(FP_FLAGS)

###############################################################################
# Executables
OPREFIX		?= /opt/bin/arm-none-eabi
#PREFIX		?= /usr/x86_64-pc-linux-gnu/arm-none-eabi/gcc-bin/7.3.0/arm-none-eabi
PREFIX		?= $(OPREFIX)

RM			:= rm -f
RMDIR		:= rmdir
CC			:= $(PREFIX)-gcc
LD			:= $(PREFIX)-gcc
AR			:= $(PREFIX)-ar
AS			:= $(PREFIX)-as
OBJCOPY		:= $(OPREFIX)-objcopy
OBJDUMP		:= $(OPREFIX)-objdump
GDB			:= $(OPREFIX)-gdb
SIZE        := $(PREFIX)-size
STFLASH		:= $(shell which st-flash)
STBOOT		:= $(shell which stm32flash)
DFUUTIL		:= $(shell which dfu-util)

###############################################################################
# Source files
OBJDIR 		= mk
LDSCRIPT	?= $(BINARY).ld
SRC			:= $(wildcard *.c)
OBJS		:= $(addprefix $(OBJDIR)/, $(SRC:%.c=%.o))
STARTUP		= $(OBJDIR)/startup.o
OBJS 		+= $(STARTUP)
DEPS		:= $(OBJS:.o=.d)

INC_DIR ?= ../inc

INCLUDE 	:= -I$(INC_DIR)/F0 -I$(INC_DIR)/cm
LIB_DIR		:= $(INC_DIR)/ld

###############################################################################
# C flags
CFLAGS		+= -O2 -g -D__thumb2__=1
CFLAGS		+= -Wall -Werror -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS		+= -Wredundant-decls $(INCLUDE)
# -Wmissing-prototypes -Wstrict-prototypes
CFLAGS		+= -fno-common -ffunction-sections -fdata-sections

###############################################################################
# Linker flags
LDFLAGS		+= --static -nostartfiles
#--specs=nano.specs
LDFLAGS		+= -L$(LIB_DIR)
LDFLAGS		+= -T$(LDSCRIPT)
LDFLAGS		+= -Wl,-Map=$(OBJDIR)/$(BINARY).map
LDFLAGS		+= -Wl,--gc-sections

###############################################################################
# Used libraries
#LDLIBS		+= -Wl,--start-group -lc -lgcc -Wl,--end-group
#LDLIBS		+= $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

DEFS		+= -DSTM32$(FAMILY) -DSTM32$(MCU)

#.SUFFIXES: .elf .bin .hex .srec .list .map .images
#.SECONDEXPANSION:
#.SECONDARY:

ELF		:= $(OBJDIR)/$(BINARY).elf
LIST	:= $(OBJDIR)/$(BINARY).list
BIN		:= $(BINARY).bin
HEX		:= $(BINARY).hex

all: bin list size

elf: $(ELF)
bin: $(BIN)
hex: $(HEX)
list: $(LIST)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

$(OBJDIR):
	mkdir $(OBJDIR)

$(STARTUP): $(INC_DIR)/startup/vector.c
	$(CC) $(CFLAGS) $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

$(VERSION_FILE): *.[ch]
	@echo "  Generate version: $(NEXTVER) for date $(BUILDDATE)"
	@sed -i "s/#define BUILD_NUMBER.*/#define BUILD_NUMBER \"$(NEXTVER)\"/" $(VERSION_FILE)
	@sed -i "s/#define BUILDNO.*/#define BUILDNO $(NEXTVER)/" $(VERSION_FILE)
	@sed -i "s/#define BUILD_DATE.*/#define BUILD_DATE \"$(BUILDDATE)\"/" $(VERSION_FILE)

$(OBJDIR)/proto.o: proto.c $(VERSION_FILE)

$(OBJDIR)/%.o: %.c
	@make $(VERSION_FILE)
	@echo "  CC	  $<"
	$(CC) $(CFLAGS) -MD $(DEFS) $(INCLUDE) $(ARCH_FLAGS) -o $@ -c $<

$(BIN): $(ELF)
	@echo "  OBJCOPY $(BIN)"
	$(OBJCOPY) -Obinary $(ELF) $(BIN)

$(HEX): $(ELF)
	@echo "  OBJCOPY $(HEX)"
	$(OBJCOPY) -Oihex $(ELF) $(HEX)

$(LIST): $(ELF)
	@echo "  OBJDUMP $(LIST)"
	$(OBJDUMP) -S $(ELF) > $(LIST)

$(ELF): $(OBJDIR) $(OBJS)
	@echo "  LD	  $(ELF)"
	$(LD) $(LDFLAGS) $(ARCH_FLAGS) $(OBJS) $(LDLIBS) -o $(ELF)
	@size $(ELF)

clean:
	@echo "  CLEAN"
	$(RM) $(OBJS) $(DEPS) $(ELF) $(HEX) $(LIST) $(OBJDIR)/*.map *.d
	@rmdir $(OBJDIR) 2>/dev/null || true

dfuboot:
	@echo "  LOAD  $(BIN) THROUGH DFU"
	$(DFUUTIL) -a0 -D $(BIN) -s 0x08000000

flash: 
	@echo "  FLASH  $(BIN)"
	$(STFLASH) write $(BIN) 0x8000000

size: $(ELF)
	$(SIZE) $(ELF)

boot: 
	@echo "  LOAD $(BIN) through bootloader"
	$(STBOOT) -b$(BOOTSPEED) $(BOOTPORT) -w $(BIN)

.PHONY: clean flash boot dfuboot size
