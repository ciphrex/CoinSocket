///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// config.cpp
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "config.h"

static CoinSocketConfig g_config;
void initConfig(int argc, char* argv[]) { g_config.init(argc, argv); }
CoinSocketConfig& getConfig() { return g_config; }
