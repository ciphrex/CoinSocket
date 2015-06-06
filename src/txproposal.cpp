///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// txproposal.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
//

#include "txproposal.h"

#include <mutex>

using namespace CoinSocket;
using namespace std;

static mutex g_mutex;
static txproposals_t g_txProposals;

void TxProposal::setHash() const
{
    hash_ = bytes_t(32, 1);
}

void CoinSocket::addTxProposal(std::shared_ptr<TxProposal> txProposal)
{
    lock_guard<mutex> lock(g_mutex);
 
    auto it = g_txProposals.find(txProposal->hash());
    if (it != g_txProposals.end()) return;

    g_txProposals[txProposal->hash()] = txProposal;
}

std::shared_ptr<TxProposal> CoinSocket::getTxProposal(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_txProposals.find(hash);
    if (it == g_txProposals.end()) throw runtime_error("Transaction proposal not found.");

    return it->second;
}

void CoinSocket::eraseTxProposal(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_txProposals.find(hash);
    if (it == g_txProposals.end()) throw runtime_error("Transaction proposal not found.");

    g_txProposals.erase(hash);
}

void CoinSocket::clearTxProposals()
{
    lock_guard<mutex> lock(g_mutex);

    g_txProposals.clear();
}
