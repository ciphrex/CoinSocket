///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// coinparams.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

#include <CoinQ/CoinQ_coinparams.h>

namespace CoinSocket
{

void setCoinParams(const CoinQ::CoinParams& coinParams);
const CoinQ::CoinParams& getCoinParams();

}
