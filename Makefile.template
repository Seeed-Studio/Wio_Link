#  copyright (c) 2015 seeed.cc
#

COMPILE?=gcc
BOOT?=new
APP?=1
SPI_SPEED?=40
SPI_MODE?=QIO
SPI_SIZE_MAP?=5
NODE_NAME?=esp8266_node
SERVER_IP?=
BUILD_FROM_SRC?=

FUNCTION_KEY?=
STATUS_LED?=
GROVE_POWER_SWITCH?=

#GROVES ?=  #it's a list of grove driver directions, passed in by caller
GROVES ?= grove_example #it's a list of grove driver directions, passed in by caller

BASE_DIR = ../..
SDK_DIR = $(BASE_DIR)/esp8266_sdk
LD_DIR = $(SDK_DIR)/ld
ARDUINO_DIR = $(BASE_DIR)/esp8266_arduino
SULI_DIR = $(BASE_DIR)/suli
GROVES_DIR = $(BASE_DIR)/grove_drivers
RPC_SERVER_DIR = $(BASE_DIR)/rpc_server
MAIN_DIR = $(BASE_DIR)/node_main
BUILD_DIR = .



###############################################################################
### DO NOT CHANGE THE FOLLOWING CONTENT
###############################################################################

boot = new

ifeq ($(APP), 1)
    app = 1
else
    ifeq ($(APP), 2)
        app = 2
    else
        app = 0
    endif
endif

ifeq ($(SPI_SPEED), 26.7)
    freqdiv = 1
else
    ifeq ($(SPI_SPEED), 20)
        freqdiv = 2
    else
        ifeq ($(SPI_SPEED), 80)
            freqdiv = 15
        else
            freqdiv = 0
        endif
    endif
endif


ifeq ($(SPI_MODE), QOUT)
    mode = 1
else
    ifeq ($(SPI_MODE), DIO)
        mode = 2
    else
        ifeq ($(SPI_MODE), DOUT)
            mode = 3
        else
            mode = 0
        endif
    endif
endif

addr = 0x01000

ifeq ($(SPI_SIZE_MAP), 1)
  size_map = 1
  flash = 256
else
  ifeq ($(SPI_SIZE_MAP), 2)
    size_map = 2
    flash = 1024
    ifeq ($(app), 2)
      addr = 0x81000
    endif
  else
    ifeq ($(SPI_SIZE_MAP), 3)
      size_map = 3
      flash = 2048
      ifeq ($(app), 2)
        addr = 0x81000
      endif
    else
      ifeq ($(SPI_SIZE_MAP), 4)
        size_map = 4
        flash = 4096
        ifeq ($(app), 2)
          addr = 0x81000
        endif
      else
        ifeq ($(SPI_SIZE_MAP), 5)
          size_map = 5
          flash = 2048
          ifeq ($(app), 2)
            addr = 0x101000
          endif
        else
          ifeq ($(SPI_SIZE_MAP), 6)
            size_map = 6
            flash = 4096
            ifeq ($(app), 2)
              addr = 0x101000
            endif
          else
            size_map = 0
            flash = 512
            ifeq ($(app), 2)
              addr = 0x41000
            endif
          endif
        endif
      endif
    endif
  endif
endif

LD_FILE = $(LD_DIR)/eagle.app.v6.ld

ifneq ($(boot), none)
ifneq ($(app),0)
    ifeq ($(size_map), 6)
      LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).2048.ld
    else
      ifeq ($(size_map), 5)
        LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).2048.ld
      else
        ifeq ($(size_map), 4)
          LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).1024.app$(app).ld
        else
          ifeq ($(size_map), 3)
            LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).1024.app$(app).ld
          else
            ifeq ($(size_map), 2)
              LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).1024.app$(app).ld
            else
              ifeq ($(size_map), 0)
                LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).512.app$(app).ld
              endif
            endif
	      endif
	    endif
	  endif
	endif
	BIN_NAME = user$(app).$(flash).$(boot).$(size_map)
endif
else
    app = 0
endif

#LD_FILE = $(LD_DIR)/eagle.app.v6.$(boot).$(flash).app$(app).ld
#LD_FILE = $(LD_DIR)/eagle.flash.4m.ld
BIN_NAME = user$(app)#.$(flash).$(boot)
OUTPUT_BIN = $(BIN_NAME)
OUTPUT = $(BUILD_DIR)/$(OUTPUT_BIN)

### Now scan all the source files recursively

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
#ALL_HTMLS := $(call rwildcard,foo/,*.html)


#arduino_pack.a
C_SRC_ARDUINO = $(notdir $(call rwildcard,$(ARDUINO_DIR),*.c))
CPP_SRC_ARDUINO = $(notdir $(call rwildcard,$(ARDUINO_DIR),*.cpp))
s_SRC_ARDUINO = $(notdir $(call rwildcard,$(ARDUINO_DIR),*.s))
OBJECTS_ARDUINO = $(C_SRC_ARDUINO:%.c=%.o) $(s_SRC_ARDUINO:%.s=%.o) $(CPP_SRC_ARDUINO:%.cpp=%.cpp.o)

#grove_drivers_pack.a
ifneq ($(BUILD_FROM_SRC), )   #"Build from source code."
    C_SRC_GROVE = $(foreach g,$(GROVES), $(notdir $(call wildcard,$(GROVES_DIR)/$(g)/*.c)))
    CPP_SRC_GROVE = $(foreach g,$(GROVES), $(notdir $(call wildcard,$(GROVES_DIR)/$(g)/*.cpp)))
    OBJECTS_GROVE = $(C_SRC_GROVE:%.c=%.o) $(CPP_SRC_GROVE:%.cpp=%.cpp.o)
else
    C_SRC_GROVE = $(notdir $(call rwildcard,$(GROVES_DIR),*.c))
    CPP_SRC_GROVE = $(notdir $(call rwildcard,$(GROVES_DIR),*.cpp))
    OBJECTS_GROVE = $(C_SRC_GROVE:%.c=%.o) $(CPP_SRC_GROVE:%.cpp=%.cpp.o)
endif


#framework_pack.a
C_SRC_FRMWK = $(notdir $(call rwildcard,$(MAIN_DIR),*.c))
CPP_SRC_FRMWK = $(notdir $(call rwildcard,$(MAIN_DIR),*.cpp))
CPP_SRC_FRMWK += suli2.cpp rpc_server.cpp rpc_stream.cpp
OBJECTS_FRMWK = $(C_SRC_FRMWK:%.c=%.o) $(CPP_SRC_FRMWK:%.cpp=%.cpp.o)

#user_code_pack.a
C_SRC_USER = $(notdir $(call rwildcard,./,*.c))
CPP_SRC_USER = $(patsubst %_gen.cpp,,$(notdir $(call rwildcard,./,*.cpp)))
CPP_SRC_USER += $(foreach g,$(GROVES), $g_gen.cpp)
OBJECTS_USER = $(C_SRC_USER:%.c=%.o) $(CPP_SRC_USER:%.cpp=%.cpp.o)



ifneq ($(BUILD_FROM_SRC), )   #"Build from source code."
    OBJECTS_ELF = $(OBJECTS_ARDUINO) $(OBJECTS_GROVE) $(OBJECTS_FRMWK) $(OBJECTS_USER)
else
    OBJECTS_ELF = $(OBJECTS_USER)
endif


### search the include paths of user dir
ALL_USER_SRC = $(dir $(call rwildcard,./,*.h))
ALL_USER_SRC += $(dir $(call rwildcard,./,*.c))
ALL_USER_SRC += $(dir $(call rwildcard,./,*.cpp))
ALL_USER_SRC_DIR = $(sort $(ALL_USER_SRC))

$(info "all user's src dirs:")
$(info $(ALL_USER_SRC_DIR))


ifneq ($(BUILD_FROM_SRC), )
    GROVES_DIR_LIST = $(foreach g,$(GROVES),$(GROVES_DIR)/$(g))
    $(info "groves:")
    $(info $(GROVES))
else
    ALL_GROVE_FILE_DIR = $(dir $(call rwildcard,$(GROVES_DIR),*.h))
    GROVES_DIR_LIST = $(sort $(ALL_GROVE_FILE_DIR))
    $(info "all grove dirs:")
    $(info $(GROVES_DIR_LIST))
endif




INCLUDE_PATHS = -I. -I$(BASE_DIR) -I$(SDK_DIR)/include -I$(SULI_DIR) -I$(RPC_SERVER_DIR) -I$(ARDUINO_DIR) -I$(ARDUINO_DIR)/spiffs
INCLUDE_PATHS += -I$(MAIN_DIR) -I$(MAIN_DIR)/polarssl
INCLUDE_PATHS += $(addprefix -I,$(GROVES_DIR_LIST))
INCLUDE_PATHS += $(addprefix -I,$(ALL_USER_SRC_DIR))

$(info "include paths:")
$(info $(INCLUDE_PATHS))

LIBRARY_PATHS = -L$(SDK_DIR)/lib -L$(SDK_DIR)/ld -L../
LIBRARIES = -lm -lgcc \
		-lhal \
		-lphy	\
		-lpp	\
		-lnet80211 \
		-llwip	\
		-lwpa	\
		-lmain	\
		-lupgrade

ifeq ($(BUILD_FROM_SRC), )
    LIBRARIES := -lprebuilt $(LIBRARIES)
endif


GCC_PATH ?=

AR      = $(GCC_PATH)xtensa-lx106-elf-ar
CC      = $(GCC_PATH)xtensa-lx106-elf-gcc
CPP     = $(GCC_PATH)xtensa-lx106-elf-g++
LD      = $(GCC_PATH)xtensa-lx106-elf-gcc
OBJCOPY = $(GCC_PATH)xtensa-lx106-elf-objcopy
OBJDUMP = $(GCC_PATH)xtensa-lx106-elf-objdump
SIZE 	= $(GCC_PATH)xtensa-lx106-elf-size

CC_FLAGS = -c -Os -O2 -Wpointer-arith -Wno-implicit-function-declaration -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -falign-functions=4 -MMD -std=c99
CPP_FLAGS = -c -Os -O2 -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -fpermissive -std=c++11 -MMD
S_FLAGS = -c -g -x assembler-with-cpp -MMD

LD_FLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

### defines
DEFINES = -DICACHE_FLASH -DF_CPU=80000000 -D__ets__ -U__STRICT_ANSI__ -DARDUINO=10604 -DARDUINO_ESP8266_ESP12 -DARDUINO_ARCH_ESP8266 -DESP8266 -DESP8266_SEEED_NODE -DNODE_NAME="\"$(NODE_NAME)\""
ifneq ($(SERVER_IP), )
    DEFINES +=  -D'SERVER_IP={$(SERVER_IP)}'
endif

ifneq ($(FUNCTION_KEY), )
    DEFINES +=  -DFUNCTION_KEY=$(FUNCTION_KEY)
endif

ifneq ($(STATUS_LED), )
    DEFINES +=  -DSTATUS_LED=$(STATUS_LED)
endif

ifneq ($(GROVE_POWER_SWITCH), )
    DEFINES +=  -DSWITCH_GROVE_POWER=$(GROVE_POWER_SWITCH)
endif

###
GET_FILESIZE ?= stat -f%z



#########################
# All ready, Let's Go
#########################

ifneq ($(BUILD_FROM_SRC), )
    $(info =========================)
    $(info Build from source code.)
else
    $(info =========================)
    $(info Build from pre-built libraries.)
endif

$(info "all objects:")
$(info $(OBJECTS_ELF))
$(info "")

VPATH = $(BASE_DIR):$(SULI_DIR):$(ARDUINO_DIR):$(ARDUINO_DIR)/spiffs:$(MAIN_DIR):$(MAIN_DIR)/polarssl:$(RPC_SERVER_DIR):$(BUILD_DIR):$(GROVES_DIR_LIST):$(ALL_USER_SRC_DIR)

.PHONY: all libs clean clean_libs

all: $(OUTPUT).bin #$(OUTPUT).hex

clean:
	rm -f *.elf *.hex *.d *.o *.a *.dump *.S

clean_libs:
	rm -f ../*.a *.o

libs: ../libprebuilt.a

%.o: %.s
	$(CC) $(CC_FLAGS) -o $@ $<

#%.o: %.S
#	$(CC) $(CC_FLAGS) -D__ASSEMBLER__ -o $@ $<

%.o: %.c
	$(CC)  $(CC_FLAGS) $(DEFINES) $(INCLUDE_PATHS) -o $@ $<

%.cpp.o: %.cpp
	$(CPP) $(CPP_FLAGS) $(DEFINES) $(INCLUDE_PATHS) -o $@ $<


../libprebuilt.a: $(OBJECTS_FRMWK) $(OBJECTS_GROVE) $(OBJECTS_ARDUINO)
	$(AR) cru $@ $(OBJECTS_FRMWK) $(OBJECTS_GROVE) $(OBJECTS_ARDUINO)


$(OUTPUT).elf: $(OBJECTS_ELF)
	$(AR) cru user_code_pack.a $(OBJECTS_ELF)
	$(LD) $(LD_FLAGS) $(LIBRARY_PATHS) -T$(LD_FILE) -Wl,--start-group user_code_pack.a $(LIBRARIES) -Wl,--end-group -o $@

	@echo ""
	@echo "**********************"
	@echo "     Link Done"
	@echo ""
	@$(SIZE) $@
	@echo "**********************"

$(OUTPUT).bin: $(OUTPUT).elf
#	@rm -rf $(OUTPUT).S $(OUTPUT).dump
#	@$(OBJDUMP) -x -s $< > $(OUTPUT).dump
#	@$(OBJDUMP) -S $< > $(OUTPUT).S

	@$(OBJCOPY) --only-section .text -O binary $< eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data -O binary $< eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata -O binary $< eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $< eagle.app.v6.irom0text.bin

	@echo ".irom0.text size: `$(GET_FILESIZE) eagle.app.v6.irom0text.bin`"
	@echo ".text size: `$(GET_FILESIZE) eagle.app.v6.text.bin`"

	@echo ""
	@echo "**********************"
	@echo " Gen Sections' Binary"
	@echo "**********************"
ifeq ($(boot), new)
	@export COMPILE=$(COMPILE) && python $(SDK_DIR)/tools/gen_appbin.py $< 2 $(mode) $(freqdiv) $(size_map) $(app)

    ifeq ($(size_map), 6)
	@echo "Support boot_v1.4 and +"
    else
        ifeq ($(size_map), 5)
	@echo "Support boot_v1.4 and +"
        else
	@echo "Support boot_v1.2 and +"
        endif
    endif
else
	@export COMPILE=$(COMPILE) && python $(SDK_DIR)/tools/gen_appbin.py $< 1 $(mode) $(freqdiv) $(size_map) $(app)
	@echo "Support boot_v1.1 and +"
endif
	@mv -f eagle.app.flash.bin $(OUTPUT).bin
	@rm eagle.app.v6.*
	@echo "Generate $(BIN_NAME).bin successully."
	@echo "boot.bin------------>0x00000"
	@echo "$(BIN_NAME).bin--->$(addr)"
	@rm *.elf

$(OUTPUT).hex: $(OUTPUT).elf
	@$(OBJCOPY) -O ihex $< $@

$(OUTPUT).lst: $(OUTPUT).elf
	@$(OBJDUMP) -Sdh $< > $@

lst: $(OUTPUT).lst

size:
	$(SIZE) $(OUTPUT).elf

DEPS = $(OBJECTS_ELF:.o=.d)
-include $(DEPS)
