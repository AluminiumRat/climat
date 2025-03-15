#pragma once

enum ErrorCode
{
  NO_ERROR = 0,
  TEMPERATURE_SENSORS_NOT_FOUND = 1,
  INSIDE_TEMPERATURE_SENSOR_ERROR = 2,
  OUTSIDE_TEMPERATURE_SENSOR_ERROR = 3,
  SERVO_ERROR = 4
};

ErrorCode getError();
void setError(ErrorCode newValue);
