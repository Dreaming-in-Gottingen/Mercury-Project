$(warning "-------<Mercury Project> libstagefright_mpeg2ts build--------")

CXX = g++
AR = ar

TARGET_LIB = libstagefright_mpeg2ts.a
INSTALL_LIB_PATH = ../../libs

INC_DIRS = -I ../include -I ../../libutils/include -I ../../foundation/include
CXX_FLAGS = -fPIC -Wno-multichar

CXX_SRC = AnotherPacketSource.cpp \
          ATSParser.cpp           \
          ESQueue.cpp             \
          MPEG2PSExtractor.cpp    \
          MPEG2TSExtractor.cpp    \

CXX_OBJ = $(patsubst %.cpp, %.o, $(CXX_SRC))
OBJS = $(CXX_OBJ)

all:$(TARGET_LIB)
	mv $(TARGET_LIB) $(INSTALL_LIB_PATH)

$(TARGET_LIB):$(OBJS)
	$(AR) r -o $@ $^

%.o:%.cpp
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_LIB)
