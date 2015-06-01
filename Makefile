INCLUDE_PATH += -Isrc

include mk/os.mk mk/cxx_flags.mk mk/boost_suffix.mk mk/odb.mk

ifndef DISABLE_TLS
    CXX_FLAGS += -DUSE_TLS=1
endif

LIBS = \
    -lSimpleSmtp \
    -lWebSocketServer \
    -lJsonRpc \
    -lCoinDB \
    -lCoinQ \
    -lCoinCore \
    -llogger \
    -lsysutils \
    -lboost_system$(BOOST_SUFFIX) \
    -lboost_filesystem$(BOOST_SUFFIX) \
    -lboost_regex$(BOOST_SUFFIX) \
    -lboost_thread$(BOOST_THREAD_SUFFIX)$(BOOST_SUFFIX) \
    -lboost_serialization$(BOOST_SUFFIX) \
    -lboost_program_options$(BOOST_SUFFIX) \
    -lcrypto \
    -lodb-$(DB) \
    -lodb \
    -lcurl

ifndef DISABLE_TLS
    LIBS += -lssl
endif

OBJS = \
    obj/config.o \
    obj/jsonobjects.o \
    obj/commands.o \
    obj/channels.o

all: build/coinsocketd$(EXE_EXT)

build/coinsocketd$(EXE_EXT): src/CoinSocket.cpp src/config.h $(OBJS)
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) $< $(OBJS) -o $@ $(LIBS) $(PLATFORM_LIBS)

obj/config.o: src/config.cpp src/config.h
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

obj/jsonobjects.o: src/jsonobjects.cpp src/jsonobjects.h
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

obj/commands.o: src/commands.cpp src/commands.h src/jsonobjects.h
	$(CXX) $(CXX_FLAGS) $(ODB_DB) $(INCLUDE_PATH) -c $< -o $@

obj/channels.o: src/channels.cpp src/channels.h
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
