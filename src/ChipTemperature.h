/////////////////////////////////////////////////////////////////////
//
// Author: Maxim S. Shatskih (maxim__s@hotmail.com)
// Platform: Arduino
// Description: reading of the on-chip temperature sensor of the MCU itself
// Date started: 20-Aug-2020
// Version: 1.0
//
/////////////////////////////////////////////////////////////////////

// ChipTemperature - main public header file
#ifndef _CHIP_TEMPERATURE_H
#define _CHIP_TEMPERATURE_H

// Check the hardware compatibility, and avoid linking-in unnecessary code

#ifdef __AVR_ATmega32U4__
uint16_t chipTemperatureReadRaw_m32U4();
#else
#error ChipTemperature is only supported for now on ATmega32U4 (Arduino Leonardo etc)
#endif

// Constants

// Kelvin temperature of 0°C
// Ignoring the fraction part - the sensor is not so accurate anyway
#define CHIPTEMP_CELSIUS_ZERO           273
// Fahrenheit temperature of 0°C
// Exact definition
#define CHIPTEMP_FAHRENHEIT_0_C         32
// Fahrenheit/Celsius scaling factor
// 1°F = (CHIPTEMP_FAHRENHEIT_NUM/CHIPTEMP_FAHRENHEIT_DENOM) * 1°C
// Exact definition
#define CHIPTEMP_FAHRENHEIT_NUM         5
#define CHIPTEMP_FAHRENHEIT_DENOM       9

// Kelvin temperature is always in uint16_t to match the bit width of the hardware readings

// Classes for calibration points

// Calibration point of the temperature sensor (Kelvin)
// tempK should be measured by external master thermometer during calibration
// hwReading is the hardware reading (ChipTemperature::getRaw()) for the temperature of tempK
struct ChipTempCalPointK {
  const uint16_t _tempK;
  const uint16_t _hwReading;
  ChipTempCalPointK(uint16_t tempK, uint16_t hwReading) :
    _tempK(tempK),
    _hwReading(hwReading) {}
  ChipTempCalPointK(const ChipTempCalPointK& right) :
    _tempK(right._tempK),
    _hwReading(right._hwReading) {}
};

// Same as ChipTempCalPointK but in Celsius
// Contains the same fields of _tempK and _hwReading
struct ChipTempCalPointC : public ChipTempCalPointK {
  ChipTempCalPointC(int tempC, uint16_t hwReading) :
    ChipTempCalPointK(
        // Celsius to Kelvin
        (uint16_t)(tempC + CHIPTEMP_CELSIUS_ZERO),
        hwReading) {}
  ChipTempCalPointC(const ChipTempCalPointC& right) :
    ChipTempCalPointK(right._tempK, right._hwReading) {}
};

// Same as ChipTempCalPointK but in Fahrenheit
// Contains the same fields of _tempK and _hwReading
struct ChipTempCalPointF : public ChipTempCalPointK {
  ChipTempCalPointF(int tempF, uint16_t hwReading) :
    ChipTempCalPointK(
            // Fahrenheit to Kelvin
            (uint16_t)((((tempF - CHIPTEMP_FAHRENHEIT_0_C)
                                * CHIPTEMP_FAHRENHEIT_NUM)
                              / CHIPTEMP_FAHRENHEIT_DENOM)
                            + CHIPTEMP_CELSIUS_ZERO),
            hwReading) {}
  ChipTempCalPointF(const ChipTempCalPointF& right) :
    ChipTempCalPointK(right._tempK, right._hwReading) {}
};

// Number of samples used for averaging
// getXxx() are valid only after this number of loops
// Before that, getXxx() will return something close to zero,
//  and much lesser than the real value
#define CHIPTEMP_SAMPLES_FOR_AVG        5

// The main API class
// Provides averaging and temperature units conversion for the hardware readings
// No methods call delay() or analogs of any kind,
//   in the worst case, only delayMicroseconds(2) is used
// So, the class supports hard realtime
class ChipTemperature {
public: // API
  // Initializes as uncalibrated (getK() == getRaw())
  ChipTemperature();
  // Initializes as calibrated with the provided calibration points
  // The values in the calibration points can go in any order
  // Surely ChipTempCalPointC/F are also OK for this call
  ChipTemperature(const ChipTempCalPointK& calPoint1,
                    const ChipTempCalPointK& calPoint2);
  // Must be called on each loop() iteration
  void loop();
  // Returns the averaged temperature value as hardware reading
  // The return value is:
  //   - guaranteed to be linear with the real-world temperature
  //   - as the undocumented "as is" empirical fact, one can treat the value as °K
  //   - but it can have error of ±10°K
  uint16_t getRaw() const;
  // Returns the averaged temperature value as Kelvins
  inline uint16_t getK() const __attribute__((always_inline));
private: // initialization data
  // 2 calibration points
  // Their values can be in any order
  const ChipTempCalPointK _calPoint1;
  const ChipTempCalPointK _calPoint2;
private: // state fields
  // Last hardware readings measured
  // Zeroes initially, but never mind - the first several loops will fill the array properly
  uint16_t _samples[CHIPTEMP_SAMPLES_FOR_AVG];
private: // internals
  // Initializes all state fields to their initial values
  void _resetObject();
  // Reads the temperature from the hardware without any averaging or conversions
  inline uint16_t _readHw() const __attribute__((always_inline));
};

// Temperature unit conversion functions
// Their names are self-speaking

inline int kelvinToCelsius(uint16_t kelvin) __attribute__((always_inline));
inline uint16_t celsiusToKelvin(int celsius) __attribute__((always_inline));
inline int celsiusToFahrenheit(int celsius) __attribute__((always_inline));

// Bodies of public inlines

int kelvinToCelsius(uint16_t kelvin) { return (int)kelvin - CHIPTEMP_CELSIUS_ZERO; }

uint16_t celsiusToKelvin(int celsius) { return (uint16_t)(celsius + CHIPTEMP_CELSIUS_ZERO); }

int celsiusToFahrenheit(int celsius) {
    return (celsius * CHIPTEMP_FAHRENHEIT_NUM) / CHIPTEMP_FAHRENHEIT_DENOM
            + CHIPTEMP_FAHRENHEIT_0_C;
}

uint16_t ChipTemperature::getK() const {
 !!! check for signs - getRaw() is the maximum, reverse order
 // Recalculation against the calibration points
 // The sign is always correct, even if the points go in reverse order
 return (uint16_t)(((((long)getRaw() - (long)_calPoint1._hwReading)
                         * ((long)_calPoint2._tempK - (long)_calPoint1._tempK))
                       / ((long)_calPoint2._hwReading - (long)_calPoint1._hwReading))
                     + (long)_calPoint1._tempK);
}

uint16_t ChipTemperature::_readHw() const {
  // Avoid linking-in unnecessary code
#ifdef __AVR_ATmega32U4__
  return chipTemperatureReadRaw_m32U4();
#endif
}

#endif // _CHIP_TEMPERATURE_H
