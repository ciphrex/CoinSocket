///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// txproposal.h
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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
const uint32_t DEFAULT_TX_VERSION = 1;
const uint32_t DEFAULT_TX_LOCKTIME = 0;

class TxProposal
{
public:
    enum status_t { PENDING, APPROVED, CANCELED, REJECTED };

    TxProposal(const std::string& username, const std::string& account, CoinDB::txouts_t txouts, uint64_t fee = DEFAULT_TX_FEE)
        : username_(username), account_(account), txouts_(txouts), fee_(fee), status_(PENDING) { timestamp_ = time(NULL); }

    const bytes_t& hash() const { if (hash_.empty()) setHash(); return hash_; }

    status_t status() const { return status_; }
    void status(status_t status) { status_ = status; }

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
    status_t status_;

    void setHash() const;
};

typedef std::map<bytes_t, std::shared_ptr<TxProposal>> txproposal_map_t;
typedef std::vector<std::shared_ptr<TxProposal>> txproposals_t;


void                            addTxProposal(std::shared_ptr<TxProposal> txProposal);
std::shared_ptr<TxProposal>     getTxProposal(const bytes_t& hash);
txproposals_t                   getTxProposals();
void                            submitTxProposal(const bytes_t& hash);
void                            cancelTxProposal(const bytes_t& hash);
void                            clearTxProposals();
std::shared_ptr<TxProposal>     getTxSubmission(const bytes_t& hash);
txproposals_t                   getTxSubmissions();
void                            approveTxSubmission(const bytes_t& hash, const bytes_t& tx_unsigned_hash);
void                            cancelTxSubmission(const bytes_t& hash);
void                            rejectTxSubmission(const bytes_t& hash);
std::shared_ptr<TxProposal>     getProcessedTxSubmission(const bytes_t& tx_unsigned_hash);
txproposals_t                   getProcessedTxSubmissions();

}
