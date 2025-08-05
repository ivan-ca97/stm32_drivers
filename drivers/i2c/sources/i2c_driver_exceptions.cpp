#include "i2c_driver_exceptions.hpp"

I2cException::I2cException(const std::string& message) : CustomException(message)
{

}

I2cException::I2cException() : CustomException("A I2C driver exception has occurred")
{

}