///////////////////////////////////////////////////////////////////////////////
//
// CoinSocketExceptions.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.
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
    CONFIG_MISSING_DBNAME = 1101,
    CONFIG_INVALID_DATA_DIR_EXCEPTION,

    // Command  errors - these errors imply an error in a submitted command
    COMMAND_INVALID_METHOD = 1201,
    COMMAND_INVALID_PARAMETERS,

    // Operation errors - these errors imply an error in the execution of command
    OPERATION_TRANSACTION_NOT_INSERTED = 1301
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

class ConfigMissingDBNameException : public ConfigException
{
public:
    explicit ConfigMissingDBNameException() : ConfigException("No dbname specified.", CONFIG_MISSING_DBNAME) { }
};

class ConfigInvalidDataDirException : public ConfigException
{
public:
    explicit ConfigInvalidDataDirException() : ConfigException("Invalid data dir.", CONFIG_INVALID_DATA_DIR_EXCEPTION) { }
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

}
