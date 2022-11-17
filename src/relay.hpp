#ifndef IOP_LIB_SENSORS_RELAY_HPP
#define IOP_LIB_SENSORS_RELAY_HPP

#include <iop-hal/io.hpp>
#include <iop-hal/thread.hpp>

#include <optional>
#include <functional>

namespace relay {
class Moment;
auto now() noexcept -> Moment;

struct Moment {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    Moment(uint8_t hour, uint8_t minute, uint8_t second) noexcept: hour(hour), minute(minute), second(second) {}

    static auto now() noexcept -> Moment {
        return relay::now();
    }

    auto operator==(const Moment & other) const noexcept -> bool {
        return this->hour == other.hour && this->minute == other.hour && this->second == other.second;
    }
    auto operator< (const Moment & other) const noexcept -> bool {
        return (this->hour >= 12 && other.hour < 12)
            || this->hour < other.hour
            || (this->hour == other.hour && this->minute < other.minute)
            || (this->hour == other.hour && this->minute == other.minute && this->second < other.second);
    }
    auto operator> (const Moment & other) const noexcept -> bool { return other < *this; }
    auto operator<=(const Moment & other) const noexcept -> bool { return !(*this > other); }
    auto operator>=(const Moment & other) const noexcept -> bool { return !(*this < other); }
};
}

template<>
struct std::hash<relay::Moment> {
    std::size_t operator()(const relay::Moment & moment) const noexcept {
        return static_cast<size_t>(moment.hour) ^ (static_cast<size_t>(moment.minute) << 1) ^ (static_cast<size_t>(moment.second) << 2);
    }
};

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
    std::unordered_map<Moment, iop_hal::io::Data> states;
    std::optional<std::pair<Moment, iop_hal::io::Data>> last;
    std::optional<Moment> next;
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

    auto setTime(const Moment moment, const iop_hal::io::Data data) noexcept -> void {
        const auto now = Moment::now();
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