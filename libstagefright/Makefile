$(warning "-------<Mercury Project> libstagefright build--------")

CXX = g++

TARGET_LIB = libstagefright.so
INSTALL_LIB_PATH = ../libs

INC_DIRS = -I include -I ../libutils/include -I ../foundation/include
CXX_FLAGS = -fPIC -Wno-multichar
LD_FLAGS = -lpthread -L../libs -lutils -lfoundation

CXX_SRC = MetaData.cpp        \
          MediaBuffer.cpp     \
          ESDS.cpp            \
          Utils.cpp           \

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
