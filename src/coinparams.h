///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// coinparams.h
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_coinparams.h>

namespace CoinSocket
{

void setCoinParams(const CoinQ::CoinParams& coinParams);
const CoinQ::CoinParams& getCoinParams();

}
