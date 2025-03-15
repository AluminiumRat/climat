#include "error.hpp"

static ErrorCode errorCode = NO_ERROR;

ErrorCode getError()
{
    return errorCode;
}

void setError(ErrorCode newValue)
{
    // Сохраняем только изначальную ошибку
    if(errorCode == NO_ERROR)
    {
        errorCode = newValue;
    }
}