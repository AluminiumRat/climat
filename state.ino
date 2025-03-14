#include "state.hpp"

static ErrorCode errorCode = NO_ERROR;

ErrorCode getError()
{
    return errorCode;
}

void setError(ErrorCode newValue)
{
    errorCode = newValue;
}