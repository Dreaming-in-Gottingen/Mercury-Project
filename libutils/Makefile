$(warning "-------<Mercury Project> libutils build--------")

CXX = g++

TARGET_LIB = libutils.so
INSTALL_LIB_PATH = ../libs

INC_DIRS = -I include
CXX_FLAGS = -fPIC
LD_FLAGS = -lpthread

CXX_SRC = RefBase.cpp \
          Timers.cpp \
          Debug.cpp \
          TextOutput.cpp \
          SharedBuffer.cpp \
          String8.cpp \
          String16.cpp \
          Unicode.cpp \
          Static.cpp \
          VectorImpl.cpp \
          Threads.cpp \
          Printer.cpp \
          CallStack.cpp \

CXX_OBJ = $(patsubst %.cpp, %.o, $(CXX_SRC))
OBJS = $(CXX_OBJ)

all:$(TARGET_LIB)
	mkdir -p $(INSTALL_LIB_PATH)
	mv $(TARGET_LIB) $(INSTALL_LIB_PATH)

$(TARGET_LIB):$(OBJS)
	$(CXX) -shared -o $@ $^ $(LD_FLAGS)

%.o:%.cpp
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_LIB)
	rm -rf $(INSTALL_LIB_PATH)
