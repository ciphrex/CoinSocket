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

#pragma once

#include <CoinDB/Schema.h>

#include <ctime>
#include <stdexcept>
#include <string>
#include <map>

namespace CoinSocket
{

// TODO: Real fee calculation heuristics for different coins.
const uint64_t DEFAULT_TX_FEE = 20000;

class TxProposal
{
public:
    TxProposal(const std::string& username, const std::string& account, CoinDB::txouts_t txouts, uint64_t fee = DEFAULT_TX_FEE)
        : username_(username), account_(account), txouts_(txouts), fee_(fee) { timestamp_ = time(NULL); }

    const bytes_t& hash() const { if (hash_.empty()) setHash(); return hash_; }

    const std::string& username() const { return username_; }
    const std::string& account() const { return account_; }
    const CoinDB::txouts_t& txouts() const { return txouts_; }
    uint64_t fee() const { return fee_; }
    uint64_t timestamp() const { return timestamp_; }

private:
    mutable bytes_t hash_;

    std::string username_;
    std::string account_;
    CoinDB::txouts_t txouts_;
    uint64_t fee_;
    uint64_t timestamp_;

    void setHash() const;
};

typedef std::map<bytes_t, std::shared_ptr<TxProposal>> txproposals_t;


void                            addTxProposal(std::shared_ptr<TxProposal> txProposal);
std::shared_ptr<TxProposal>     getTxProposal(const bytes_t& hash);
void                            eraseTxProposal(const bytes_t& hash);
void                            clearTxProposals();

}
