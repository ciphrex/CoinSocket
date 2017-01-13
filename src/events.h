///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// events.h
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include "txproposal.h"

#include <CoinDB/SynchedVault.h>
#include <WebSocketAPI/Server.h>

#include <string>
#include <set>
#include <map>

namespace CoinSocket
{

// TODO: Clean this up.
std::multimap<uint32_t, std::shared_ptr<CoinDB::Tx>>& getPendingTxs();
std::set<bytes_t>& getPendingTxHashes();

enum TxEventType { INSERTED, UPDATED, APPROVED, CANCELED, REJECTED, DELETED };
void sendTxJsonEvent(TxEventType type, WebSocket::Server& wsServer, websocketpp::connection_hdl hdl, CoinDB::SynchedVault& synchedVault, std::shared_ptr<CoinDB::Tx>& tx, bool fakeFinal = false);
void sendTxChannelEvent(TxEventType type, WebSocket::Server& wsServer, CoinDB::SynchedVault& synchedVault, std::shared_ptr<CoinDB::Tx>& tx, bool fakeFinal = false);
void sendTxChannelEvent(WebSocket::Server& wsServer, std::shared_ptr<TxProposal>& txProposal);
void sendStatusEvent(WebSocket::Server& wsServer, CoinDB::SynchedVault& synchedVault);

}
