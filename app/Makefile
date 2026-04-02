DEVICE          = stm32f103c8t6
OPENCM3_DIR    = ../libopencm3
LDSCRIPT       = linkerscript.ld

PREFIX         ?= arm-none-eabi
STFLASH        = st-flash

OPT            = -Os
DEBUG          = -ggdb3
CSTD           = -std=c11

BINARY         = firmware

SRC_DIR        = src
INC_DIR        = inc

OBJS           = $(SRC_DIR)/$(BINARY).o

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

CFLAGS         += $(OPT) $(CSTD) $(DEBUG)
CFLAGS         += -Wextra -Wshadow -Wimplicit-function-declaration
CFLAGS         += -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
CFLAGS         += -fno-common -ffunction-sections -fdata-sections

CPPFLAGS       += -MD -Wall -Wundef
CPPFLAGS       += -I$(INC_DIR)

LDFLAGS        += --static -nostartfiles
LDFLAGS        += -Wl,-Map=$(BINARY).map -Wl,--cref
LDFLAGS        += -Wl,--gc-sections

LDLIBS         += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

.PHONY: all flash clean rebuild list size

all: $(BINARY).elf $(BINARY).bin $(BINARY).hex

$(BINARY).elf: $(OBJS) $(LDSCRIPT) $(LIBDEPS)
	@printf "  LD      $(BINARY).elf\n"
	$(Q)$(LD) $(OBJS) $(LDLIBS) $(LDFLAGS) -T$(LDSCRIPT) $(ARCH_FLAGS) -o $@

$(BINARY).bin: $(BINARY).elf
	@printf "  OBJCOPY $(BINARY).bin\n"
	$(Q)$(OBJCOPY) -Obinary $< $@

$(BINARY).hex: $(BINARY).elf
	@printf "  OBJCOPY $(BINARY).hex\n"
	$(Q)$(OBJCOPY) -Oihex $< $@

$(BINARY).list: $(BINARY).elf
	@printf "  OBJDUMP $(BINARY).list\n"
	$(Q)$(OBJDUMP) -S $< > $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "  CC      $<\n"
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) $(ARCH_FLAGS) -o $@ -c $<

flash: $(BINARY).bin
	@printf "  STFLASH $(BINARY).bin\n"
	$(Q)$(STFLASH) write $(BINARY).bin 0x08000000

list: $(BINARY).list

size: $(BINARY).elf
	@printf "  SIZE    $(BINARY).elf\n"
	$(Q)$(SIZE) $<

clean:
	$(Q)$(RM) -f $(BINARY).elf $(BINARY).bin $(BINARY).hex $(BINARY).list $(BINARY).map
	$(Q)$(RM) -f $(OBJS) $(OBJS:.o=.d) generated.* $(DEVICE).ld

rebuild: clean all

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
