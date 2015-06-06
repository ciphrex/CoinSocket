///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// alerts.cpp
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.
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
