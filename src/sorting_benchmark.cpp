#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <deque>
#include <list>
#include <numeric>
#include <random>
#include <vector>

constexpr auto max_container_bytes = 1 << 26;
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

template <typename C>
void sort(C& container) {
  std::sort(container.begin(), container.end());
}

template <typename T>
void sort(std::list<T>& list) {
  list.sort();
}

template <int ElementBytes, template <typename> typename ContainerType>
static void BM_sort_container(benchmark::State& state) {
  using ElementType = Element<ElementBytes>;

  static constexpr auto max_container_size =
      max_container_bytes / sizeof(ElementType);
  static constexpr auto test_size = max_container_size * test_repetitions;

  auto const container_bytes = state.range(0);
  auto const container_size = container_bytes / sizeof(ElementType);
  auto const random_values = [=] {
    auto rvs = std::vector<int>(test_size);
    std::iota(rvs.begin(), rvs.end(), 0);

    auto gen = std::mt19937{};
    std::shuffle(rvs.begin(), rvs.end(), gen);
    return rvs;
  }();
  auto test_containers = [=] {
    auto const n_test_containers = test_size / container_size;
    auto tcs = std::vector<ContainerType<ElementType>>(n_test_containers);
    for (auto i = 0UL; i < test_size; ++i) {
      auto const container_index = i / container_size;
      tcs[container_index].push_back(ElementType{random_values[i]});
    }
    return tcs;
  }();

  for (auto _ : state) {
    for (auto& c : test_containers) {
      sort(c);
      benchmark::DoNotOptimize(c.front());
    }
  }
  state.SetItemsProcessed(state.iterations() * test_size);
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

#define BM_SORT_CONTAINER(ELEMENT_SIZE, CONTAINER_TYPE)               \
  BENCHMARK_TEMPLATE(BM_sort_container, ELEMENT_SIZE, CONTAINER_TYPE) \
      ->RangeMultiplier(2)                                            \
      ->Range(ELEMENT_SIZE, max_container_bytes)                      \
      ->Iterations(1);
#define BM_SORT_CONTAINER_SET(CONTAINER_TYPE) \
  BM_SORT_CONTAINER(4, CONTAINER_TYPE)        \
  BM_SORT_CONTAINER(16, CONTAINER_TYPE)       \
  BM_SORT_CONTAINER(64, CONTAINER_TYPE)       \
  BM_SORT_CONTAINER(256, CONTAINER_TYPE)      \
  BM_SORT_CONTAINER(1024, CONTAINER_TYPE)

BM_SORT_CONTAINER_SET(std::deque);
BM_SORT_CONTAINER_SET(std::vector);
BM_SORT_CONTAINER_SET(PointerVector);
BM_SORT_CONTAINER_SET(std::list);
