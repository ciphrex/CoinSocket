///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// events.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
//

#pragma once

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
void sendStatusEvent(WebSocket::Server& wsServer, CoinDB::SynchedVault& synchedVault);

}
