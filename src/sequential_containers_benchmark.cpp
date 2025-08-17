#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <list>
#include <memory>
#include <random>
#include <utility>
#include <vector>

constexpr auto max_container_bytes = 1 << 20;
constexpr auto test_repetitions = 1;

template <std::size_t ElementBytes>
struct Element : public std::array<int, ElementBytes / sizeof(int)> {
  static_assert(
      (ElementBytes & (ElementBytes - 1)) == 0 and ElementBytes >= sizeof(int),
      "ElementBytes must be a power of two and at least sizeof(int).");
  friend auto operator<(Element const& lhs, Element const& rhs) {
    return lhs[0] < rhs[0];
  }
};

template <typename ContainerType>
struct NullIniter {
  void operator()(ContainerType&, std::size_t) {}
};

template <typename ContainerType>
struct Reserver {
  void operator()(ContainerType& c, std::size_t n) { c.reserve(n); }
};

template <int ElementBytes, template <typename> typename ContainerType,
          template <typename> typename Initializer>
static void BM_insert_in_sorted_order(benchmark::State& state) {
  using ElementType = Element<ElementBytes>;

  constexpr auto max_container_size = max_container_bytes / sizeof(ElementType);
  constexpr auto test_size = max_container_size * test_repetitions;

  auto const container_bytes = state.range(0);
  auto const container_size = container_bytes / sizeof(ElementType);
  auto const random_values = [=] {
    auto rvs = std::vector<int>(test_size);
    std::iota(rvs.begin(), rvs.end(), 0);

    auto gen = std::mt19937{};
    std::shuffle(rvs.begin(), rvs.end(), gen);
    return rvs;
  }();
  auto const input_vector = [=] {
    auto iv = std::vector<std::pair<int, ElementType>>(test_size);
    for (auto i = 0UL; i < test_size; ++i) {
      iv[i].first = i / container_size;
      iv[i].second[0] = random_values[i];
    }
    return iv;
  }();
  auto output_containers = [=] {
    auto const n_output_containers = test_size / container_size;
    auto ocs = std::vector<ContainerType<ElementType>>(n_output_containers);
    for (auto& c : ocs) {
      Initializer<ContainerType<ElementType>>{}(c, container_size);
    }
    return ocs;
  }();

  for (auto _ : state) {
    for (auto const& [container_idx, element] : input_vector) {
      auto& current_container = output_containers[container_idx];
      auto it = current_container.begin();
      for (; it != current_container.end(); ++it) {
        if (element < *it) {
          break;
        }
      }
      benchmark::DoNotOptimize(current_container.insert(it, element));
    }
  }
  state.SetItemsProcessed(state.iterations() * test_size);
}

template <typename T>
struct NullContainer {
  auto begin() { return static_cast<T*>(nullptr); }
  auto end() { return static_cast<T*>(nullptr); }
  auto insert(T*, T t) {
    benchmark::DoNotOptimize(t);
    return static_cast<T*>(nullptr);
  }
};

template <typename T>
class HeapElement {
 public:
  HeapElement() : e_{std::make_unique<T>()} {}
  HeapElement(T const& t) : e_{std::make_unique<T>(t)} {}
  HeapElement(HeapElement const& other) : e_{std::make_unique<T>(*other.e_)} {}
  HeapElement(HeapElement&& other) : e_{std::move(other.e_)} {}
  HeapElement& operator=(HeapElement const& other) {
    *e_ = *other.e_;
    return *this;
  }
  HeapElement& operator=(HeapElement&& other) {
    e_ = std::move(other.e_);
    return *this;
  }
  ~HeapElement() = default;
  operator T() const { return *e_; }
  operator T&() { return *e_; }

 private:
  std::unique_ptr<T> e_;
};

template <typename T>
struct PointerVector : public std::vector<HeapElement<T>> {};

#define BM_SORTED_INSERT(ELEMENT_SIZE, CONTAINER_TYPE, INIT)                  \
  BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, ELEMENT_SIZE, CONTAINER_TYPE, \
                     INIT)                                                    \
      ->RangeMultiplier(2)                                                    \
      ->Range(ELEMENT_SIZE, max_container_bytes)                              \
      ->Iterations(1);
#define BM_SORTED_INSERT_SET(CONTAINER_TYPE, INIT) \
  BM_SORTED_INSERT(4, CONTAINER_TYPE, INIT)        \
  BM_SORTED_INSERT(16, CONTAINER_TYPE, INIT)       \
  BM_SORTED_INSERT(64, CONTAINER_TYPE, INIT)       \
  BM_SORTED_INSERT(256, CONTAINER_TYPE, INIT)      \
  BM_SORTED_INSERT(1024, CONTAINER_TYPE, INIT)

BM_SORTED_INSERT_SET(NullContainer, NullIniter);
BM_SORTED_INSERT_SET(std::vector, NullIniter);
BM_SORTED_INSERT_SET(std::vector, Reserver);
BM_SORTED_INSERT_SET(PointerVector, NullIniter);
BM_SORTED_INSERT_SET(PointerVector, Reserver);
BM_SORTED_INSERT_SET(std::list, NullIniter);
