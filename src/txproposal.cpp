///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// txproposal.cpp
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txproposal.h"

#include <stdutils/uchar_vector.h>

#include <CoinCore/hash.h>

#include <mutex>

using namespace CoinSocket;
using namespace std;

static mutex g_mutex;
static txproposal_map_t g_pendingTxProposals;
static txproposal_map_t g_submittedTxProposals;
static txproposal_map_t g_processedTxProposals;

void TxProposal::setHash() const
{
    // TODO: Better serialization resistant to malleability
    uchar_vector serialized;
    uint64_t v;

    for (auto c: username_)     { serialized.push_back((unsigned char)c); }
    serialized.push_back(0x00);

    for (auto c: account_)      { serialized.push_back((unsigned char)c); }
    serialized.push_back(0x00);

    for (auto& txout: txouts_)
    {
        for (auto c: txout->sending_label()) { serialized.push_back((unsigned char)c); }
        serialized.push_back(0x00);

        serialized += txout->script();

        v = txout->value();
        serialized.push_back((unsigned char)((v >> 56) & 0xff));
        serialized.push_back((unsigned char)((v >> 48) & 0xff));
        serialized.push_back((unsigned char)((v >> 40) & 0xff));
        serialized.push_back((unsigned char)((v >> 32) & 0xff));
        serialized.push_back((unsigned char)((v >> 24) & 0xff));
        serialized.push_back((unsigned char)((v >> 16) & 0xff));
        serialized.push_back((unsigned char)((v >> 8) & 0xff));
        serialized.push_back((unsigned char)(v & 0xff));
    }

    v = fee_;
    serialized.push_back((unsigned char)((v >> 56) & 0xff));
    serialized.push_back((unsigned char)((v >> 48) & 0xff));
    serialized.push_back((unsigned char)((v >> 40) & 0xff));
    serialized.push_back((unsigned char)((v >> 32) & 0xff));
    serialized.push_back((unsigned char)((v >> 24) & 0xff));
    serialized.push_back((unsigned char)((v >> 16) & 0xff));
    serialized.push_back((unsigned char)((v >> 8) & 0xff));
    serialized.push_back((unsigned char)(v & 0xff));

    v = timestamp_;
    serialized.push_back((unsigned char)((v >> 56) & 0xff));
    serialized.push_back((unsigned char)((v >> 48) & 0xff));
    serialized.push_back((unsigned char)((v >> 40) & 0xff));
    serialized.push_back((unsigned char)((v >> 32) & 0xff));
    serialized.push_back((unsigned char)((v >> 24) & 0xff));
    serialized.push_back((unsigned char)((v >> 16) & 0xff));
    serialized.push_back((unsigned char)((v >> 8) & 0xff));
    serialized.push_back((unsigned char)(v & 0xff));
        
    hash_ = sha256_2(serialized);
}

void CoinSocket::addTxProposal(std::shared_ptr<TxProposal> txProposal)
{
    lock_guard<mutex> lock(g_mutex);
 
    auto it = g_pendingTxProposals.find(txProposal->hash());
    if (it != g_pendingTxProposals.end()) return;

    g_pendingTxProposals[txProposal->hash()] = txProposal;
}

std::shared_ptr<TxProposal> CoinSocket::getTxProposal(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_pendingTxProposals.find(hash);
    if (it == g_pendingTxProposals.end()) throw runtime_error("Transaction proposal not found.");

    return it->second;
}

txproposals_t CoinSocket::getTxProposals()
{
    txproposals_t txProposals;

    lock_guard<mutex> lock(g_mutex);

    for (auto& pair: g_pendingTxProposals)
    {
        txProposals.push_back(pair.second);
    }

    return txProposals;
}

void CoinSocket::cancelTxProposal(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_pendingTxProposals.find(hash);
    if (it == g_pendingTxProposals.end()) throw runtime_error("Transaction proposal not found.");

    g_pendingTxProposals.erase(hash);
}

void CoinSocket::submitTxProposal(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_pendingTxProposals.find(hash);
    if (it == g_pendingTxProposals.end()) throw runtime_error("Transaction proposal not found.");

    g_submittedTxProposals[hash] = it->second;
    g_pendingTxProposals.erase(hash);
}

std::shared_ptr<TxProposal> CoinSocket::getTxSubmission(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_submittedTxProposals.find(hash);
    if (it == g_submittedTxProposals.end()) throw runtime_error("Transaction submission not found.");

    return it->second;
}

txproposals_t CoinSocket::getTxSubmissions()
{
    txproposals_t txProposals;

    lock_guard<mutex> lock(g_mutex);

    for (auto& pair: g_submittedTxProposals)
    {
        txProposals.push_back(pair.second);
    }

    return txProposals;
}

void CoinSocket::approveTxSubmission(const bytes_t& hash, const bytes_t& tx_unsigned_hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_submittedTxProposals.find(hash);
    if (it == g_submittedTxProposals.end()) throw runtime_error("Transaction submission not found.");

    g_processedTxProposals[tx_unsigned_hash] = it->second;
    g_processedTxProposals[tx_unsigned_hash]->status(TxProposal::APPROVED);
    g_submittedTxProposals.erase(hash);
}

void CoinSocket::cancelTxSubmission(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_submittedTxProposals.find(hash);
    if (it == g_submittedTxProposals.end()) throw runtime_error("Transaction submission not found.");

    g_processedTxProposals[hash] = it->second;
    g_processedTxProposals[hash]->status(TxProposal::CANCELED);
    g_submittedTxProposals.erase(hash);
}

void CoinSocket::rejectTxSubmission(const bytes_t& hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_submittedTxProposals.find(hash);
    if (it == g_submittedTxProposals.end()) throw runtime_error("Transaction submission not found.");

    g_processedTxProposals[hash] = it->second;
    g_processedTxProposals[hash]->status(TxProposal::REJECTED);
    g_submittedTxProposals.erase(hash);
}

std::shared_ptr<TxProposal> CoinSocket::getProcessedTxSubmission(const bytes_t& tx_unsigned_hash)
{
    lock_guard<mutex> lock(g_mutex);

    auto it = g_processedTxProposals.find(tx_unsigned_hash);
    if (it == g_processedTxProposals.end()) return nullptr;

    return it->second;
}

txproposals_t CoinSocket::getProcessedTxSubmissions()
{
    txproposals_t txProposals;

    lock_guard<mutex> lock(g_mutex);

    for (auto& pair: g_processedTxProposals)
    {
        txProposals.push_back(pair.second);
    }

    return txProposals;
}

void CoinSocket::clearTxProposals()
{
    lock_guard<mutex> lock(g_mutex);

    g_pendingTxProposals.clear();
}
