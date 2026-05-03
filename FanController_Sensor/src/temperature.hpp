#pragma once
#include <avr/io.h>
#include <Arduino.h>

namespace adc {
enum class Adc : uint8_t {
    ADC0 = 0,
    ADC2 = 2,
    ADC3 = 3,
};

enum class RefVoltage : uint16_t {
    INTERNAL_1_1V = 1100,
    INTERNAL_2_56V = 2560,
};

enum class Prescaler : uint8_t {
    DIV128 = 0x07,
};

inline uint8_t refBits(RefVoltage refV) {
    switch (refV) {
    case RefVoltage::INTERNAL_1_1V:
        return (1 << REFS1);
    case RefVoltage::INTERNAL_2_56V:
        return (1 << REFS2) | (1 << REFS1);
    }
    return 0;
}

inline void startConversion() {
    ADCSRA |= (1 << ADSC);
    while ((ADCSRA & (1 << ADSC)) != 0) {
        // Wait for conversion to complete
    }
}

inline void configure(RefVoltage refV, uint8_t precisionBits, Prescaler prescaler) {
    if (precisionBits != 8) {
        return;
    }

    ADMUX = (1 << ADLAR) | refBits(refV);
    ADCSRA = (1 << ADEN) | static_cast<uint8_t>(prescaler);
}

inline uint8_t read8(Adc adc) {
    ADMUX = (ADMUX & 0xF0) | static_cast<uint8_t>(adc);
    startConversion();
    return ADCH;
}
} // namespace adc

namespace temperature {
constexpr uint8_t ACCURACY_BITS = 8;
static_assert(ACCURACY_BITS == 8, "Current implementation assumes 8-bit ADC reading for temperature");

constexpr adc::RefVoltage ADC_REF_MV = adc::RefVoltage::INTERNAL_1_1V;
constexpr adc::Prescaler ADC_PRESCALER = adc::Prescaler::DIV128;

constexpr uint16_t OFFSET_AT_0_CELSIUS_MV = 500;
constexpr uint8_t MV_PER_CELSIUS = 10;
constexpr uint16_t ADC_STEPS = 1u << ACCURACY_BITS;

inline uint8_t rawToCelsius(uint8_t raw) {
    const uint16_t mv = static_cast<uint16_t>((static_cast<uint32_t>(raw) * static_cast<uint16_t>(ADC_REF_MV)) / ADC_STEPS);
    if (mv <= OFFSET_AT_0_CELSIUS_MV) {
        return 0;
    }
    return static_cast<uint8_t>((mv - OFFSET_AT_0_CELSIUS_MV) / MV_PER_CELSIUS);
}

inline uint8_t readTempC(adc::Adc channel) { return rawToCelsius(adc::read8(channel)); }

inline uint8_t readTemp1C() { return readTempC(adc::Adc::ADC0); }
inline uint8_t readTemp2C() { return readTempC(adc::Adc::ADC3); }
inline uint8_t readTemp3C() { return readTempC(adc::Adc::ADC2); }
inline uint8_t readCelsius() { return readTemp1C(); }

inline void init() { adc::configure(ADC_REF_MV, ACCURACY_BITS, ADC_PRESCALER); }
} // namespace temperature