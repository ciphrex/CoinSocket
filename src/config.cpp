///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// config.cpp
//
// Copyright (c) 2014-2015 Eric Lombrozo
//
// All Rights Reserved.
//

#include "config.h"

static CoinSocketConfig g_config;
void initConfig(int argc, char* argv[]) { g_config.init(argc, argv); }
CoinSocketConfig& getConfig() { return g_config; }
