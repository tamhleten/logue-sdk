##############################################################################
# Common project definitions

MKFILE_PATH := $(realpath $(lastword $(MAKEFILE_LIST)))
PROJECT_ROOT ?= $(dir $(MKFILE_PATH))
COMMON_INC_PATH ?= $(realpath $(PROJECT_ROOT)/../common/)
COMMON_SRC_PATH ?= $(realpath $(PROJECT_ROOT)/../common/)
INSTALLDIR ?= $(PROJECT_ROOT)
TOOLSDIR ?= $(PROJECT_ROOT)/../../../tools
EXTDIR ?= $(PROJECT_ROOT)/../../ext
CMSISDIR ?= $(EXTDIR)/CMSIS/CMSIS
LDDIR ?= $(PROJECT_ROOT)/../ld

include config.mk

PROJECT ?= my_unit
MCU := cortex-m7
MCU_MODEL := STM32H725xE
GCC_TARGET := arm-none-eabi-
GCC_BIN_PATH ?= $(TOOLSDIR)/gcc/gcc-arm-none-eabi-10.3-2021.10/bin

CC    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc
CXXC  := $(GCC_BIN_PATH)/$(GCC_TARGET)g++
LD    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc
CP    := $(GCC_BIN_PATH)/$(GCC_TARGET)objcopy
AS    := $(GCC_BIN_PATH)/$(GCC_TARGET)gcc -x assembler-with-cpp
AR    := $(GCC_BIN_PATH)/$(GCC_TARGET)ar
OD    := $(GCC_BIN_PATH)/$(GCC_TARGET)objdump
SZ    := $(GCC_BIN_PATH)/$(GCC_TARGET)size
STRIP := $(GCC_BIN_PATH)/$(GCC_TARGET)strip
HEX   := $(CP) -O ihex
BIN   := $(CP) -O binary

RULESPATH := $(LDDIR)
LDSCRIPT := $(LDDIR)/unit.ld
DLIBS := -lc
DADEFS := -D$(MCU_MODEL) -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7
DDEFS := -D$(MCU_MODEL) -DCORTEX_USE_FPU=TRUE -DARM_MATH_CM7 -D__FPU_PRESENT
COPT := -fPIC -std=c11 -fno-exceptions
CXXOPT := -fPIC -fno-use-cxa-atexit -std=c++11 -fno-rtti -fno-exceptions -fno-non-call-exceptions
LDOPT := -shared --entry=0 -specs=nano.specs -specs=nosys.specs
CWARN := -W -Wall -Wextra
CXXWARN :=
FPU_OPTS := -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -fcheck-new
OPT := -g -Os -mlittle-endian $(FPU_OPTS)
TOPT := -mthumb -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -DTHUMB_PRESENT

PRODUCT := $(PROJECT).nts3unit
BUILDDIR := $(PROJECT_ROOT)/build
OBJDIR := $(BUILDDIR)/obj
LSTDIR := $(BUILDDIR)/lst
ASMSRC := $(UASMSRC)
ASMXSRC := $(UASMXSRC)
CSRC := $(UCSRC) $(realpath $(COMMON_SRC_PATH)/_unit_base.c)
CXXSRC := $(UCXXSRC)

vpath %.s $(sort $(dir $(ASMSRC)))
vpath %.S $(sort $(dir $(ASMXSRC)))
vpath %.c $(sort $(dir $(CSRC)))
vpath %.cc $(sort $(dir $(CXXSRC)))

ASMOBJS := $(addprefix $(OBJDIR)/, $(notdir $(ASMSRC:.s=.o)))
ASMXOBJS := $(addprefix $(OBJDIR)/, $(notdir $(ASMXSRC:.S=.o)))
COBJS := $(addprefix $(OBJDIR)/, $(notdir $(CSRC:.c=.o)))
CXXOBJS := $(addprefix $(OBJDIR)/, $(notdir $(CXXSRC:.cc=.o)))
OBJS := $(ASMXOBJS) $(ASMOBJS) $(COBJS) $(CXXOBJS)
DINCDIR := $(COMMON_INC_PATH) $(CMSISDIR)/Include
INCDIR := $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))
DEFS := $(DDEFS) $(UDEFS)
ADEFS := $(DADEFS) $(UADEFS)
LIBS := $(DLIBS) $(ULIBS)
LIBDIR := $(patsubst %,-I%,$(DLIBDIR) $(ULIBDIR))
CFLAGS = $(MCFLAGS) $(TOPT) $(OPT) $(COPT) $(CWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.c=.lst)) $(DEFS)
CXXFLAGS = $(MCFLAGS) $(TOPT) $(OPT) $(CXXOPT) $(CXXWARN) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.cc=.lst)) $(DEFS)
MCFLAGS := -mcpu=$(MCU)
ODFLAGS := -x --syms
ASFLAGS = $(MCFLAGS) -g $(TOPT) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.s=.lst)) $(ADEFS)
ASXFLAGS = $(MCFLAGS) -g $(TOPT) -Wa,-alms=$(LSTDIR)/$(notdir $(<:.S=.lst)) $(ADEFS)
LDFLAGS := $(MCFLAGS) $(TOPT) $(OPT) -nostartfiles $(LIBDIR) -Wl,-z,max-page-size=128,-Map=$(BUILDDIR)/$(PROJECT).map,--cref,--no-warn-mismatch,--library-path=$(RULESPATH),--script=$(LDSCRIPT) $(LDOPT)

OUTFILES := $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PROJECT).hex $(BUILDDIR)/$(PROJECT).bin $(BUILDDIR)/$(PROJECT).dmp $(BUILDDIR)/$(PROJECT).list

all: PRE_ALL $(OBJS) $(OUTFILES) POST_ALL
	@echo Done
	@echo
PRE_ALL:
POST_ALL:
$(OBJS): | $(BUILDDIR) $(OBJDIR) $(LSTDIR)
$(BUILDDIR) $(OBJDIR) $(LSTDIR):
	@mkdir -p $@
$(ASMOBJS) : $(OBJDIR)/%.o : %.s Makefile
	@$(AS) -c $(ASFLAGS) -I. $(INCDIR) $< -o $@
$(ASMXOBJS) : $(OBJDIR)/%.o : %.S Makefile
	@$(CC) -c $(ASXFLAGS) -I. $(INCDIR) $< -o $@
$(COBJS) : $(OBJDIR)/%.o : %.c Makefile
	@$(CC) -c $(CFLAGS) -I. $(INCDIR) $< -o $@
$(CXXOBJS) : $(OBJDIR)/%.o : %.cc Makefile
	@$(CXXC) -c $(CXXFLAGS) -I. $(INCDIR) $< -o $@
$(BUILDDIR)/%.elf: $(OBJS) $(LDSCRIPT)
	@$(LD) $(OBJS) $(LDFLAGS) $(LIBS) -o $@
%.hex: %.elf
	@$(HEX) $< $@
%.bin: %.elf
	@$(BIN) $< $@
%.dmp: %.elf
	@$(OD) $(ODFLAGS) $< > $@
%.list: %.elf
	@$(OD) -S $< > $@
	@$(SZ) $<

clean:
	-rm -fR $(PROJECT_ROOT)/.dep $(BUILDDIR) $(PROJECT_ROOT)/$(PRODUCT)

$(BUILDDIR)/$(PRODUCT): | $(OBJS) $(OUTFILES)
	@cp -a $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PRODUCT)
	@$(STRIP) $(BUILDDIR)/$(PRODUCT)

install: $(BUILDDIR)/$(PRODUCT)
	@mv $(BUILDDIR)/$(PRODUCT) $(INSTALLDIR)/$(PRODUCT)
