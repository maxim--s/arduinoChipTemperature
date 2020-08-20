// Reads the on-chip temperature sensor as abstract [0..1023] integer
// The reading is linear, and must be then calibrated to the real temperature
// As the undocumented "as is" empirical fact, one can treat the value
//  as Kelvin degrees from absolute zero (-273 C),
//  though uncalibrated value can give the error by ±10°C
// The reading is frequently oscillating by +-1..2
// Applying the debounce tineout logic (like for buttons,
//   i.e. wait till the reading is constant for some time)
//  does not work - the debounce timeout never occurs
// So, some averaging should be used on the return value of this function
// The logic is based on analogRead() source, and does not break analogRead()
// The function only contains delayMicroseconds(2), not delay() or analogs of any kind,
//  so it supports hard realtime
int chipTemperatureReadRaw()
{
  //
  // ATmega32U4 datasheet, the chapter of:
  //  24.6.1 Sensor Calibration
  // says:
  //  The sensor initial tolerance is large (±10°C), but its characteristic is linear.
  //
  // We are compatible with analogRead() since it will reprogram
  //  all of the registers we're touching
  //
  // We read these twice, and don't want the compiler to get rid of the first reading action
  //  (it is probably important for the chip)
  volatile uint8_t low, high;
  //
  // Connect the temperature sensor as ADC input,
  //   power the sensor up,
  //   and set the proper analog reference for it
  //
  // The chapter of:
  //  24.6 Temperature Sensor
  // says:
  //  "The internal 2.56V voltage reference must also be selected
  //   for the ADC voltage reference source in he temperature sensor measurement."
  // The chapter of:
  //  24.9.1 ADC Multiplexer Selection Register – ADMUX
  // says for REFS1:0:
  //  "1 1 Internal 2.56V Voltage Reference with external capacitor on AREF pin"
  // - and Leonardo has the capacitor,
  //   so does Iskra Neo (Russian clone of Leonardo with MUCH improved power supply chips)
  // Same chapter, the paragraph of:
  //  Bits 4:0 – MUX4:0: Analog Channel Selection Bits
  // says for MUX5..0:
  //  100111 Temperature Sensor
  ADMUX = _BV(REFS1) | _BV(REFS0) | // REFS1..0 is 0b11 -> internal 2.56V voltage for ref
                                      // ADLAR (left-adjust result) is 0
            0x07;                     // MUX4..0 is 0b00111
  // The chapter of:
  //  24.9.4 ADC Control and Status Register B – ADCSRB
  // the paragraph of:
  //  Bit 5 – MUX5: Analog Channel Additional Selection Bits
  // says:
  //  "This bit make part of MUX5:0 bits of ADRCSRB and ADMUX register,
  //  that select the combination of analog inputs connected to the ADC
  //  (including differential amplifier configuration)."
  // So, set MUX5 to 1 to get 0b100111
  ADCSRB = ADCSRB | _BV(MUX5);
  //
  // Sensor connected, now start the ADC
  //
  // The chapter of:
  //  24.9.2 ADC Control and Status Register A – ADCSRA
  // the paragraph of:
  //  Bit 6 – ADSC: ADC Start Conversion
  // says:
  //  "In Single Conversion mode, write this bit to one to start each conversion.
  //   In Free Running mode, write this bit to one to start the first conversion.
  //   The first conversion after ADSC has been written after the ADC has been enabled,
  //    or if ADSC is written at the same time as the ADC is enabled,
  //    will take 25 ADC clock cycles instead of the normal 13.
  //    This first conversion performs initialization of the ADC.
  //   ADSC will read as one as long as a conversion is in progress.
  //   When the conversion is complete, it returns to zero.
  //   Writing zero to this bit has no effect."
  // Start the measurement
  ADCSRA = ADCSRA | _BV(ADSC);
  // Poll-wait for the measurement to be done (it takes some time)
  while (bit_is_set(ADCSRA, ADSC));
  // The chapter of:
  //  24.9.3 The ADC Data Register – ADCL and ADCH
  // says:
  //  "When ADCL is read, the ADC Data Register is not updated until ADCH is read.
  //  Consequently, if the result is left adjusted and no more than 8 -
  //  bit precision(7 bit + sign bit for differential input channels) is required, it is
  //  sufficient to read ADCH. Otherwise, ADCL must be read first, then ADCH."
  // Read ADCL first
  low = ADCL;
  high = ADCH;
  // The chapter of:
  //  24.6 Temperature Sensor
  // says:
  //  "The temperature sensor and its internal driver are enabled
  //   when ADMUX value selects the temperature sensor as ADC input.
  //   The propagation delay of this driver is approximately 2uS.
  //   Therefore two successive conversions are required.
  //   The correct temperature measurement will be the second one."
  //
  // So, delay for 2us and repeat the ADC measurement
  //
  delayMicroseconds(2); // this busy-loops the CPU in a calibrated execution loop
  // Start the measurement
  ADCSRA = ADCSRA | _BV(ADSC);
  // Poll-wait for the measurement to be done
  while (bit_is_set(ADCSRA, ADSC));
  // Read and return the result
  low = ADCL;
  high = ADCH;
  return (high << 8) | low;
}
