ODB_DB = \
    -DDATABASE_SQLITE

CXX_FLAGS += -Wall
ifdef DEBUG
    CXX_FLAGS += -g
else
    CXX_FLAGS += -O3
endif

ifndef NO_USE_TLS
    CXX_FLAGS += -DUSE_TLS=1
endif

ifndef SYSROOT
    SYSROOT = /usr/local
endif

INCLUDE_PATH = \
    -Isrc

ifndef OS
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S), Linux)
        OS = linux
    else ifeq ($(UNAME_S), Darwin)
        OS = osx
    endif
endif

ifeq ($(OS), linux)
    CXX = g++
    CC = gcc
    CXX_FLAGS += -Wno-unknown-pragmas -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    ARCHIVER = ar

    PLATFORM_LIBS += \
        -lpthread

else ifeq ($(OS), mingw64)
    CXX =  x86_64-w64-mingw32-g++
    CC =  x86_64-w64-mingw32-gcc
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-strict-aliasing -std=c++0x -DBOOST_SYSTEM_NOEXCEPT=""

    MINGW64_ROOT = /usr/x86_64-w64-mingw32

    INCLUDE_PATH += -I$(MINGW64_ROOT)/include

    ARCHIVER = x86_64-w64-mingw32-ar

    EXE_EXT = .exe

else ifeq ($(OS), osx)
    CXX = clang++
    CC = clang
    CXX_FLAGS += -Wno-unknown-pragmas -Wno-unneeded-internal-declaration -std=c++11 -stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DMAC_OS_X_VERSION_MIN_REQUIRED=MAC_OS_X_VERSION_10_6 -mmacosx-version-min=10.7

    INCLUDE_PATH += -I/usr/local/include

    BOOST_SUFFIX = -mt

    ARCHIVER = ar

else ifneq ($(MAKECMDGOALS), clean)
    $(error OS must be set to linux, mingw64, or osx)
endif

LIBS = \
    -lWebSocketServer \
    -lCoinDB \
    -lCoinQ \
    -lCoinCore \
    -llogger \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_SUFFIX) \
    -lboost_serialization$(BOOST_SUFFIX) \
    -lboost_program_options$(BOOST_SUFFIX) \
    -lcrypto \
    -lodb-sqlite \
    -lodb

ifndef NO_USE_TLS
    LIBS += -lssl
endif

OBJS = \
    obj/commands.o

all: build/coinsocketd$(EXE_EXT)

build/coinsocketd$(EXE_EXT): src/main.cpp src/config.h $(OBJS)
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) $< $(OBJS) -o $@ $(LIBS) $(PLATFORM_LIBS)

obj/commands.o: src/commands.cpp src/commands.h src/jsonobjects.h
	$(CXX) $(CXX_FLAGS) $(INCLUDE_PATH) -c $< -o $@

install:
	-mkdir -p $(SYSROOT)/bin
	-cp build/coinsocketd$(EXE_EXT) $(SYSROOT)/bin/

remove:
	-rm $(SYSROOT)/bin/coinsocketd$(EXE_EXT)

clean:
	-rm -f build/coinsocketd$(EXE_EXT)

clean-all:
	-rm -f build/coinsocketd$(EXE_EXT) obj/*.o
