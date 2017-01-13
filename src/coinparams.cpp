///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// coinparams.cpp
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "coinparams.h"

using namespace CoinSocket;

static CoinQ::CoinParams g_coinParams;

void CoinSocket::setCoinParams(const CoinQ::CoinParams& coinParams)
{
    g_coinParams = coinParams;
}

const CoinQ::CoinParams& CoinSocket::getCoinParams()
{
    return g_coinParams;
}
