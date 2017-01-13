///////////////////////////////////////////////////////////////////////////////
//
// CoinSocketExceptions.h
//
// Copyright (c) 2014-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <stdutils/customerror.h>
#include <string>

namespace CoinSocket
{

enum ErrorCodes
{
    // Internal errors - these errors should never occur and indicate bugs
    INTERNAL_TX_JSON_INVALID = 1001, // CoinDB reserves error codes 100 - 999

    // Config errors - these errors result from invalid program options
    CONFIG_MISSING_NETWORK = 1101,
    CONFIG_MISSING_DBNAME,
    CONFIG_INVALID_DATA_DIR,
    CONFIG_MISSING_SMTP_USER,
    CONFIG_MISSING_SMTP_PASSWORD,
    CONFIG_MISSING_SMTP_URL,
    CONFIG_MISSING_SMTP_FROM,

    // Command  errors - these errors imply an error in a submitted command
    COMMAND_INVALID_METHOD = 1201,
    COMMAND_INVALID_PARAMETERS,
    COMMAND_INVALID_CHANNELS,

    // Operation errors - these errors imply an error in the execution of command
    OPERATION_TRANSACTION_NOT_INSERTED = 1301,
    OPERATION_TRANSACTION_NOT_DELETED,

    // Data format errors - these errors imply an error with the way parameter data is formatted
    DATA_FORMAT_INVALID_ADDRESS = 1401
};

// INTERNAL EXCEPTIONS
class InternalException : public stdutils::custom_error
{
public:
    virtual ~InternalException() throw() { }

protected:
    explicit InternalException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class InternalTxJsonInvalidException : public InternalException
{
public:
    explicit InternalTxJsonInvalidException() : InternalException("Internal error - invalid tx json.", INTERNAL_TX_JSON_INVALID) { }
};

// CONFIG EXCEPTIONS
class ConfigException : public stdutils::custom_error
{
public:
    virtual ~ConfigException() throw() { }

protected:
    explicit ConfigException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class ConfigMissingNetworkException : public ConfigException
{
public:
    explicit ConfigMissingNetworkException() : ConfigException("No network specified.", CONFIG_MISSING_NETWORK) { }
};

class ConfigMissingDBNameException : public ConfigException
{
public:
    explicit ConfigMissingDBNameException() : ConfigException("No dbname specified.", CONFIG_MISSING_DBNAME) { }
};

class ConfigInvalidDataDirException : public ConfigException
{
public:
    explicit ConfigInvalidDataDirException() : ConfigException("Invalid data dir.", CONFIG_INVALID_DATA_DIR) { }
};

class ConfigMissingSmtpUserException : public ConfigException
{
public:
    explicit ConfigMissingSmtpUserException() : ConfigException("No smtpuser specified.", CONFIG_MISSING_SMTP_USER) { }
};

class ConfigMissingSmtpPasswordException : public ConfigException
{
public:
    explicit ConfigMissingSmtpPasswordException() : ConfigException("No smtppasswd specified.", CONFIG_MISSING_SMTP_PASSWORD) { }
};

class ConfigMissingSmtpUrlException : public ConfigException
{
public:
    explicit ConfigMissingSmtpUrlException() : ConfigException("No smtpurl specified.", CONFIG_MISSING_SMTP_URL) { }
};

class ConfigMissingSmtpFromException : public ConfigException
{
public:
    explicit ConfigMissingSmtpFromException() : ConfigException("No smtpfrom specified.", CONFIG_MISSING_SMTP_FROM) { }
};

// COMMAND EXCEPTIONS
class CommandException : public stdutils::custom_error
{
public:
    virtual ~CommandException() throw() { }

protected:
    explicit CommandException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class CommandInvalidMethodException : public CommandException
{
public:
    explicit CommandInvalidMethodException() : CommandException("Invalid method.", COMMAND_INVALID_METHOD) { }
};

class CommandInvalidParametersException : public CommandException
{
public:
    explicit CommandInvalidParametersException() : CommandException("Invalid parameters.", COMMAND_INVALID_PARAMETERS) { }
};

class CommandInvalidChannelsException : public CommandException
{
public:
    explicit CommandInvalidChannelsException() : CommandException("Invalid subscription channels.", COMMAND_INVALID_CHANNELS) { }
};

// OPERATION EXCEPTIONS
class OperationException : public stdutils::custom_error
{
public:
    virtual ~OperationException() throw() { }

protected:
    explicit OperationException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class OperationTransactionNotInsertedException : public OperationException
{
public:
    explicit OperationTransactionNotInsertedException() : OperationException("Transaction not inserted.", OPERATION_TRANSACTION_NOT_INSERTED) { }
};

class OperationTransactionNotDeletedException : public OperationException
{
public:
    explicit OperationTransactionNotDeletedException() : OperationException("Transaction not deleted.", OPERATION_TRANSACTION_NOT_DELETED) { }
};

// DATA FORMAT EXCEPTIONS
class DataFormatException : public stdutils::custom_error
{
public:
    virtual ~DataFormatException() throw() { }

protected:
    explicit DataFormatException(const std::string& what, int code) : stdutils::custom_error(what, code) { }
};

class DataFormatInvalidAddressException : public DataFormatException
{
public:
    explicit DataFormatInvalidAddressException() : DataFormatException("Invalid address.", DATA_FORMAT_INVALID_ADDRESS) { }
};

}
