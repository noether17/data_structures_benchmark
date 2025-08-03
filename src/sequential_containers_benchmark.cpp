#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <list>
#include <memory>
#include <random>
#include <utility>
#include <vector>

constexpr auto max_container_bytes = 1 << 22;
constexpr auto test_repetitions = 1;

template <typename ContainerType>
void reserve_if_reserving_vector(ContainerType&, std::size_t) {}

template <int ElementBytes, template <typename> typename ContainerType>
static void BM_insert_in_sorted_order(benchmark::State& state) {
  static_assert(
      (ElementBytes & (ElementBytes - 1)) == 0 and ElementBytes >= sizeof(int),
      "ElementBytes must be a power of two and at least sizeof(int).");
  using ElementType = std::array<int, ElementBytes / sizeof(int)>;

  constexpr auto max_container_size = max_container_bytes / sizeof(ElementType);
  constexpr auto test_size = max_container_size * test_repetitions;

  auto const container_bytes = state.range(0);
  auto const container_size = container_bytes / sizeof(ElementType);
  auto const random_values = [=] {
    auto rvs = std::vector<int>(test_size);
    std::iota(rvs.begin(), rvs.end(), 0);

    auto rd = std::random_device{};
    auto gen = std::mt19937{rd()};
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
      reserve_if_reserving_vector(c, container_size);
    }
    return std::vector<ContainerType<ElementType>>(n_output_containers);
  }();

  for (auto _ : state) {
    for (auto const& [container_idx, element] : input_vector) {
      auto& current_container = output_containers[container_idx];
      auto it = current_container.begin();
      for (; it != current_container.end(); ++it) {
        if (element[0] < static_cast<ElementType>(*it)[0]) {
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

// Use std::vector implementation, but tag to use reserve() when initializing.
template <typename ElementType>
struct ReservingVector : public std::vector<ElementType> {};

template <typename ElementType>
void reserve_if_reserving_vector(ReservingVector<ElementType>& rv,
                                 std::size_t s) {
  rv.reserve(s);
}

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

#define BM_SORTED_INSERT(ELEMENT_SIZE, CONTAINER_TYPE)                        \
  BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, ELEMENT_SIZE, CONTAINER_TYPE) \
      ->RangeMultiplier(2)                                                    \
      ->Range(ELEMENT_SIZE, max_container_bytes)                              \
      ->Iterations(1);
#define BM_SORTED_INSERT_SET(CONTAINER_TYPE) \
  BM_SORTED_INSERT(4, CONTAINER_TYPE)        \
  BM_SORTED_INSERT(16, CONTAINER_TYPE)       \
  BM_SORTED_INSERT(64, CONTAINER_TYPE)       \
  BM_SORTED_INSERT(256, CONTAINER_TYPE)      \
  BM_SORTED_INSERT(1024, CONTAINER_TYPE)

BM_SORTED_INSERT_SET(NullContainer);
BM_SORTED_INSERT_SET(std::vector);
BM_SORTED_INSERT_SET(ReservingVector);
BM_SORTED_INSERT_SET(std::list);
BM_SORTED_INSERT_SET(PointerVector);
