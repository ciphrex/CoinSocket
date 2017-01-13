///////////////////////////////////////////////////////////////////////////////
//
// CoinSocket
//
// alerts.h
//
// Copyright (c) 2015-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <SimpleSmtp/smtp.h>

namespace CoinSocket
{

void setSmtpTls(const std::string& username, const std::string& password, const std::string& url);
SmtpTls& getSmtpTls();

}
