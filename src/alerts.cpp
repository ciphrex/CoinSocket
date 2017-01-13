///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// alerts.cpp
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "alerts.h"

using namespace CoinSocket;

static SmtpTls g_smtpTls;

void CoinSocket::setSmtpTls(const std::string& username, const std::string& password, const std::string& url)
{
    g_smtpTls.set(username, password, url);
}

SmtpTls& CoinSocket::getSmtpTls()
{
    return g_smtpTls;
}
