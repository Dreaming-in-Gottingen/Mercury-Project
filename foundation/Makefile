$(warning "-------<Mercury Project> libutils build--------")

CXX = g++

TARGET_LIB = libfoundation.so
INSTALL_LIB_PATH = ../libs

INC_DIRS = -I include -I ../libutils/include
CXX_FLAGS = -fPIC
LD_FLAGS = -lpthread -L../libs -lutils

CXX_SRC = AAtomizer.cpp     \
          ABitReader.cpp    \
          ABuffer.cpp       \
          AHandler.cpp      \
          ALooper.cpp       \
          ALooperRoster.cpp \
          AMessage.cpp      \
          AString.cpp       \
          ParsedMessage.cpp \
          base64.cpp        \
          hexdump.cpp       \

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
