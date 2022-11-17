#ifndef IOP_LIB_SENSORS_RELAY_NOOP_HPP
#define IOP_LIB_SENSORS_RELAY_NOOP_HPP

#include <relay.hpp>

namespace relay {
auto now() noexcept -> Moment { return Moment(0, 0, 0); }
auto TimedRelay::actIfNeeded() noexcept -> bool { return false; }
auto Relay::begin() const noexcept -> void {}
auto Relay::setHigh() const noexcept -> void {}
auto Relay::setLow() const noexcept -> void {}
auto Relay::setHighFor(const iop::time::milliseconds millis) const noexcept -> void { (void) millis; }
auto Relay::setLowFor(const iop::time::milliseconds millis) const noexcept -> void { (void) millis; }
}

#endif