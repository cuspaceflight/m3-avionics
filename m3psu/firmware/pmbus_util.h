/*
 * Utils for converting PMBus data format to C format
 */

#ifndef PMBUS_UTIL_H_
#define PMBUS_UTIL_H_

#include <math.h>

#define LinearExponentBitCount  5
#define LinearMantissaBitCount  11

#define LinearExponentLargestPositiveValue  ((1 << (LinearExponentBitCount-1)) - 1)
#define LinearExponentLargestNegativeValue  ((1 << (LinearExponentBitCount-1)) * -1)
#define LinearMantissaLargestPositiveValue  ((1 << (LinearMantissaBitCount-1)) - 1)
#define LinearMantissaLargestNegativeValue  ((1 << (LinearMantissaBitCount-1)) * -1)

float L11_to_float(uint16_t input_val){
  int8_t exponent = input_val >> 11;
  int16_t mantissa = (input_val & 0x7ff);
  // sign-extend 5 bits
  if ((uint8_t)exponent > 0x0F)
    exponent |= 0xE0;
  // sign-extend 11 bits
  if (mantissa > 0x3FF)
    mantissa |= 0xF800;
  return mantissa * pow(2, exponent);
}

// Taken from https://bitbucket.org/ericssonpowermodules/c-pmbus-numeric-conversions/src/a45f5bcabe21ef80e45db48f678084f79d05da24/src/PMBusNumericConversions.c
uint16_t float_to_L11 (float input) {
  signed char pmbusExponent = 0;
  signed short pmbusMantissa = 0;
  float mantissaBeforeRounding;
  uint8_t pmbusLinearLowByte, pmbusLinearHighByte;

  if (input > 0.0) {
    pmbusExponent = (signed char) ceilf( log10f( input / (float) LinearMantissaLargestPositiveValue) / log10f( 2.0 ) );
  } else if (input < 0.0) {
    pmbusExponent = (signed char) ceilf( log10f( input / (float) LinearMantissaLargestNegativeValue) / log10f( 2.0 ) );
  } else {
    pmbusExponent = 0;
  }

  //saturate exponent to boundaries
  if (pmbusExponent > LinearExponentLargestPositiveValue)
    pmbusExponent = LinearExponentLargestPositiveValue;
  if (pmbusExponent < LinearExponentLargestNegativeValue)
    pmbusExponent = LinearExponentLargestNegativeValue;

  mantissaBeforeRounding = (input / powf(2.0, pmbusExponent));
  //round result in direction of greater magnitude
  pmbusMantissa = (signed short) ( (input >= 0.0) ? mantissaBeforeRounding + 0.5 : mantissaBeforeRounding - 0.5 );

  pmbusLinearHighByte  = 0xF8 & (pmbusExponent << 3);
  pmbusLinearHighByte |= 0x07 & (char) (pmbusMantissa >> (LinearMantissaBitCount - 3));

  pmbusLinearLowByte = (char) pmbusMantissa;

  return (pmbusLinearHighByte << 8) | pmbusLinearLowByte;
}

float L16_to_float(int8_t exp, uint16_t input_val){
  int16_t mantissa = input_val;

  return mantissa * pow(2, exp);
}

uint16_t float_to_L16(int8_t exp, float input_val){
  float exponent = pow(2, exp);
  return (uint16_t)(input_val / exponent);
}


#endif /* PMBUS_UTIL_H_ */
