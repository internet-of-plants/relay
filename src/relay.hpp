#ifndef IOP_LIB_SENSORS_RELAY_HPP
#define IOP_LIB_SENSORS_RELAY_HPP

#include <iop-hal/io.hpp>
#include <iop-hal/device.hpp>
#include <iop-hal/thread.hpp>

#include <optional>
#include <functional>

namespace relay {
class Relay {
    iop_hal::PinRaw pin;
public:
    Relay(const iop_hal::PinRaw pin) noexcept: pin(pin) {}

    auto begin() const noexcept -> void;

    /// Sets relay to high, blocks for `millis` and set it to low again
    ///
    /// Disables interrupts
    ///
    /// SAFETY:
    ///     Yields to background tasks while waiting (like wifi), a iop_panic may happen so be sure to add a cleanup hook to the panic machinery
    auto setHighFor(iop::time::milliseconds millis) const noexcept -> void;

    /// Sets relay to low, blocks for `millis` and set it to high again
    ///
    /// Disables interrupts
    /// Be ware, always on relays have a risk of leaving things on when they shouldn't if there is a crash
    ///
    /// SAFETY:
    ///     Yields to background tasks while waiting (like wifi), a iop_panic may happen so be sure to add a cleanup hook to the panic machinery
    auto setLowFor(iop::time::milliseconds millis) const noexcept -> void;

    auto setFor(iop_hal::io::Data data, iop::time::milliseconds millis) const noexcept -> void;

    /// Low level API, you should avoid it as they can leave things running when they shouldn't if there is a panic
    auto setHigh() const noexcept -> void;
    auto setLow() const noexcept -> void;
    auto set(const iop_hal::io::Data data) const noexcept -> void {
        switch (data) {
        case iop_hal::io::Data::HIGH:
            this->setHigh();
            break;
        case iop_hal::io::Data::LOW:
            this->setLow();
            break;
        }
    }

    auto operator=(Relay && other) noexcept -> Relay & = default;
    auto operator=(const Relay & other) noexcept = delete;

    Relay(Relay && other) noexcept = default;
    Relay(const Relay & other) noexcept = delete;

    ~Relay() noexcept = default;
};

// Don't use this for things like water pumps, that can cause damage if they are active for longe
// Use `Relay::setFor`, `Relay::setHighFor` or `Relay::setLowFor` as they have more garantees
class TimedRelay {
    Relay relay;
    std::unordered_map<iop_hal::Moment, iop_hal::io::Data> states;
    std::optional<std::pair<iop_hal::Moment, iop_hal::io::Data>> last;
    std::optional<iop_hal::Moment> next;
public:
    TimedRelay(const iop_hal::PinRaw pin) noexcept: relay(pin) {}
    TimedRelay(Relay relay) noexcept: relay(std::move(relay)) {}

    auto begin() const noexcept -> void {
        this->relay.begin();
    }

    auto clear() noexcept -> void {
        this->states.clear();
        this->next = std::nullopt;
    }

    auto setTime(const iop_hal::Moment moment, const iop_hal::io::Data data) noexcept -> void {
        const auto now = iop_hal::Moment::now();
        if (now > moment && (!this->last || this->last->first < moment)) {
            this->last = std::make_pair(moment, data);
            this->set(data);
        }

        if (now < moment && (!this->next || *this->next > moment)) {
            this->next = moment;
        }

        this->states.insert_or_assign(moment, data);
    }

    auto actIfNeeded() noexcept -> bool;

    /// Low level API, you should avoid it as they can leave things running when they shouldn't if there is a panic
    auto set(iop_hal::io::Data data) const noexcept -> void { this->relay.set(data); }
    auto setHigh() const noexcept -> void { this->relay.setHigh(); }
    auto setLow() const noexcept -> void { this->relay.setLow(); }
};
}

#endif
