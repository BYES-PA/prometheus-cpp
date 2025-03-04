#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "prometheus/collectable.h"
#include "prometheus/detail/core_export.h"
#include "prometheus/family.h"
#include "prometheus/labels.h"
#include "prometheus/metric_family.h"

namespace prometheus {

class Counter;
class Gauge;
class Histogram;
class Info;
class Summary;

namespace detail {

template <typename T>
class Builder;  // IWYU pragma: keep

}
/// \brief Manages the collection of a number of metrics.
///
/// The Registry is responsible to expose data to a class/method/function
/// "bridge", which returns the metrics in a format Prometheus supports.
///
/// The key class is the Collectable. This has a method - called Collect() -
/// that returns zero or more metrics and their samples. The metrics are
/// represented by the class Family<>, which implements the Collectable
/// interface. A new metric is registered with BuildCounter(), BuildGauge(),
/// BuildHistogram(), BuildInfo() or BuildSummary().
///
/// The class is thread-safe. No concurrent call to any API of this type causes
/// a data race.
class PROMETHEUS_CPP_CORE_EXPORT Registry : public Collectable {
 public:
  /// \brief How to deal with repeatedly added family names for a type.
  ///
  /// Adding a family with the same name but different types is always an error
  /// and will lead to an exception.
  enum class InsertBehavior {
    /// \brief If a family with the same name and labels already exists return
    /// the existing one. If no family with that name exists create it.
    /// Otherwise throw.
    Merge,
    /// \brief Throws if a family with the same name already exists.
    Throw,
  };

  /// \brief name Create a new registry.
  ///
  /// \param insert_behavior How to handle families with the same name.
  explicit Registry(InsertBehavior insert_behavior = InsertBehavior::Merge);

  /// \brief Deleted copy constructor.
  Registry(const Registry&) = delete;

  /// \brief Deleted copy assignment.
  Registry& operator=(const Registry&) = delete;

  /// \brief Deleted move constructor.
  Registry(Registry&&) = delete;

  /// \brief Deleted move assignment.
  Registry& operator=(Registry&&) = delete;

  /// \brief name Destroys a registry.
  ~Registry() override;

  /// \brief Returns a list of metrics and their samples.
  ///
  /// Every time the Registry is scraped it calls each of the metrics Collect
  /// function.
  ///
  /// \return Zero or more metrics and their samples.
  std::vector<MetricFamily> Collect() const override;

  /// \brief Removes a metrics family from the registry.
  ///
  /// Please note that this operation invalidates the previously
  /// returned reference to the Family and all of their added
  /// metric objects.
  ///
  /// \tparam T One of the metric types Counter, Gauge, Histogram or Summary.
  /// \param family The family to remove
  ///
  /// \return True if the family was found and removed.
  template <typename T>
  bool Remove(const Family<T>& family);

  using OnCollectCallback = void(*)(Registry const& registry, void* pApp);
  Registry& RegisterOnCollect(OnCollectCallback subscriber, void* pApp);

 private:
  template <typename T>
  friend class detail::Builder;

  template <typename T>
  std::vector<std::unique_ptr<Family<T>>>& GetFamilies();

  template <typename T>
  bool NameExistsInOtherType(const std::string& name) const;

  template <typename T>
  Family<T>& Add(const std::string& name, const std::string& help,
                 const Labels& labels);

  const InsertBehavior insert_behavior_;
  std::vector<std::unique_ptr<Family<Counter>>> counters_;
  std::vector<std::unique_ptr<Family<Gauge>>> gauges_;
  std::vector<std::unique_ptr<Family<Histogram>>> histograms_;
  std::vector<std::unique_ptr<Family<Info>>> infos_;
  std::vector<std::unique_ptr<Family<Summary>>> summaries_;
  mutable std::mutex mutex_;

  struct OnCollectSubscription {
    OnCollectCallback Callback;
    void* pApp;
  };

  std::vector<OnCollectSubscription> onCollectSubscriptions_;
};

}  // namespace prometheus
