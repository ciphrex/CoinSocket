///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// coinparams.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
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
