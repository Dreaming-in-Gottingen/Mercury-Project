$(warning "-------<Mercury Project> libstagefright build--------")

CXX = g++

TARGET_LIB = libstagefright.so
INSTALL_LIB_PATH = ../libs

INC_DIRS = -I include -I ../libutils/include -I ../foundation/include
CXX_FLAGS = -fPIC -Wno-multichar
LD_FLAGS = -lpthread -L../libs -lutils -lfoundation -L../libs -lstagefright_mpeg2ts

CXX_SRC = MetaData.cpp        \
          MediaBuffer.cpp     \
          ESDS.cpp            \
          Utils.cpp           \
          MediaDefs.cpp       \
          MediaSource.cpp     \
          MPEG2TSWriter.cpp   \
          MPEG4Writer.cpp     \
          DataSource.cpp      \
          FileSource.cpp      \
          TinyCacheSource.cpp \
          MediaExtractor.cpp  \
          SampleTable.cpp     \
          SampleIterator.cpp  \
          MediaBufferGroup.cpp\
          MPEG4Extractor.cpp  \
          avc_utils.cpp       \

CXX_OBJ = $(patsubst %.cpp, %.o, $(CXX_SRC))
OBJS = $(CXX_OBJ)

all:$(TARGET_LIB)
	mv $(TARGET_LIB) $(INSTALL_LIB_PATH)

$(TARGET_LIB):$(OBJS)
	$(CXX) -shared -o $@ $^ $(LD_FLAGS)

%.o:%.cpp
	$(CXX) $(INC_DIRS) -c $^ -o $@ $(CXX_FLAGS)

clean:
	rm -f $(OBJS)
	rm -f $(TARGET_LIB)
