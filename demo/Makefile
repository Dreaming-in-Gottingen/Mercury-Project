$(warning "-------<Mercury Project> demo build--------")

CXX = g++

TARGET_BIN = $(patsubst %.o, %, $(CXX_OBJ))

INC_DIRS = -I ../libutils/include
CXX_FLAGS = -fPIC
LD_FLAGS = -lpthread -L../libs -lutils

CXX_SRC = $(wildcard *.cpp)
CXX_OBJ = $(patsubst %.cpp, %.o, $(CXX_SRC))
OBJS = $(CXX_OBJ)

all:$(TARGET_BIN)

$(TARGET_BIN):%:%.o
	$(CXX) $^ -o $@ $(LD_FLAGS)

%.o:%.cpp
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_BIN)
