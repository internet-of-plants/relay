#ifndef IOP_LIB_SENSORS_RELAY_ARDUINO_HPP
#define IOP_LIB_SENSORS_RELAY_ARDUINO_HPP

#include <relay.hpp>
#include <optional>

#include <time.h>
#include <Arduino.h>
#undef HIGH
#undef LOW
#undef OUTPUT

namespace relay {
auto now() noexcept -> Moment {
    const auto rawtime = time(nullptr);
    const auto *time = localtime(&rawtime);
    return Moment(static_cast<uint8_t>(time->tm_hour), static_cast<uint8_t>(time->tm_min), static_cast<uint8_t>(time->tm_sec));
}

auto TimedRelay::actIfNeeded() noexcept -> bool {
    if (!this->next) return false;
    if (Moment::now() < this->next) return false;

    std::optional<Moment> next;
    for (const auto & [moment, data]: this->states) {
        if (this->next == moment) {
            this->relay.set(data);
            this->last = std::make_pair(moment, data);
        }
        if (moment > *this->next && (!next || moment < *next)) {
            next = moment;
        }
    }
    this->next = next;

    return true;
}

auto Relay::begin() const noexcept -> void {
    iop_hal::gpio.setMode(this->pin, iop_hal::io::Mode::OUTPUT);
}

auto Relay::setHigh() const noexcept -> void {
    digitalWrite(this->pin, static_cast<uint8_t>(iop_hal::io::Data::HIGH));
}

auto Relay::setLow() const noexcept -> void {
    digitalWrite(this->pin, static_cast<uint8_t>(iop_hal::io::Data::LOW));
}

auto Relay::setFor(const iop_hal::io::Data data, const iop::time::milliseconds millis) const noexcept -> void {
    noInterrupts();

    this->set(data);
    iop_hal::thisThread.sleep(millis);
    switch (data) {
        case iop_hal::io::Data::HIGH:
            this->setLow();
            break;
        case iop_hal::io::Data::LOW:
            this->setHigh();
            break;
    }

    interrupts();
}

auto Relay::setHighFor(const iop::time::milliseconds millis) const noexcept -> void {
    noInterrupts();

    this->setHigh();
    iop_hal::thisThread.sleep(millis);
    this->setLow();

    interrupts();
}

auto Relay::setLowFor(const iop::time::milliseconds millis) const noexcept -> void {
    noInterrupts();

    this->setLow();
    iop_hal::thisThread.sleep(millis);
    this->setHigh();

    interrupts();
}
}

#endif