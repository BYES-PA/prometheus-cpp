#include "prometheus/gauge.h"

#include <ctime>

namespace prometheus {

Gauge::Gauge(const double value) : value_{value} {}

void Gauge::Increment() { Increment(1.0); }

void Gauge::Increment(const double value) { Change(value); }

void Gauge::Decrement() { Decrement(1.0); }

void Gauge::Decrement(const double value) { Change(-1.0 * value); }

void Gauge::Set(const double value) { value_.store(value); }

void Gauge::Change(const double value) {
  // C++ 20 will add std::atomic::fetch_add support for floating point types
  auto current = value_.load();
  while (!value_.compare_exchange_weak(current, current + value)) {
    // intentionally empty block
  }
}

void Gauge::SetToCurrentTime() {
  const auto time = std::time(nullptr);
  Set(static_cast<double>(time));
}

double Gauge::Value() const { return value_; }

ClientMetric Gauge::Collect() {
  ClientMetric metric;
  metric.gauge.value = Value();
  if (_resetOnCollect)
    Set(0);
  return metric;
}

Gauge& Gauge::ResetOnCollect(bool value) {
  _resetOnCollect = value;
  return *this;
}

void Gauge::Update(double value, UpdateCallback updateCallback) {
  double current = 0;
  while (!value_.compare_exchange_weak(current, updateCallback(current, value)));
}

}  // namespace prometheus
