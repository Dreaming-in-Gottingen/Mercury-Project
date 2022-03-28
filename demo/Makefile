$(warning "-------<Mercury Project> demo build--------")

CXX = g++

TARGET_BIN = $(patsubst %.o, %, $(CXX_OBJ))

INC_DIRS = -I ../libutils/include -I ../foundation/include
CXX_FLAGS = -g -fPIC -Wno-multichar

#windows platform
#LD_FLAGS = -lpthread -L../libs libutils.so
#linux platform
#LD_FLAGS = -lpthread -L../libs -lutils -lfoundation -lcorkscrew
LD_FLAGS = -lpthread -L../libs -lutils -lfoundation

#CXX_SRC = $(wildcard *.cpp)
CXX_SRC = $(filter-out CallStackDemo.cpp, $(wildcard *.cpp))
CXX_OBJ = $(patsubst %.cpp, %.o, $(CXX_SRC))
OBJS = $(CXX_OBJ)

all:$(TARGET_BIN)

$(TARGET_BIN):%:%.o
	$(CXX) -o $@ $^ $(LD_FLAGS)

%.o:%.cpp
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_BIN)
