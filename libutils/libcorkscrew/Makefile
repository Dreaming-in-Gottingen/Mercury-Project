$(warning "-------<Mercury Project> libutils/libcorkscrew build--------")

CXX = g++

TARGET_ARCH = x86
TARGET_LIB = libcorkscrew.so
INSTALL_LIB_PATH = ../../libs


INC_DIRS = -I ../include
CXX_FLAGS = -fPIC -fpermissive
LD_FLAGS = -lpthread -ldl

GENERIC_SRC_FILES = \
        backtrace.c \
        backtrace-helper.c \
        demangle.c \
        map_info.c \
        ptrace.c \
        symbol_table.c \

ARM_SRC_FILES := \
        arch-arm/backtrace-arm.c \
        arch-arm/ptrace-arm.c \

X86_SRC_FILES := \
        arch-x86/backtrace-x86.c \
        arch-x86/ptrace-x86.c \

MIPS_SRC_FILES := \
        arch-mips/backtrace-mips.c \
        arch-mips/ptrace-mips.c \

ifeq ($(TARGET_ARCH), arm)
GENERIC_SRC_FILES += $(ARM_SRC_FILES)
CXX_FLAGS += -DCORKSCREW_HAVE_ARCH -D__arm__
endif

ifeq ($(TARGET_ARCH), x86)
GENERIC_SRC_FILES += $(X86_SRC_FILES)
# default add -D__x86_64__ by gcc on x86_64 platform
#CXX_FLAGS += -DCORKSCREW_HAVE_ARCH -D__x86_64__ -D__i386__
CXX_FLAGS += -DCORKSCREW_HAVE_ARCH -D__i386__
endif

ifeq ($(TARGET_ARCH), mips)
GENERIC_SRC_FILES += $(MIPS_SRC_FILES)
CXX_FLAGS += -DCORKSCREW_HAVE_ARCH -D__mips__
endif

CXX_FLAGS += -DANDROID_SMP=0

CXX_OBJ = $(patsubst %.c, %.o, $(GENERIC_SRC_FILES))
OBJS = $(CXX_OBJ)

all:$(TARGET_LIB)
	#$(CXX) $(INC_DIRS) -o test test.cpp $(TARGET_LIB)
	mkdir -p $(INSTALL_LIB_PATH)
	mv $(TARGET_LIB) $(INSTALL_LIB_PATH)

$(TARGET_LIB):$(OBJS)
	$(CXX) -shared -o $@ $^ $(LD_FLAGS)

#$(OBJS):$(GENERIC_SRC_FILES)
%.o:%.c
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_LIB)
