#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <list>
#include <random>
#include <utility>
#include <vector>

// #define REPEAT2(X) X X
// #define REPEAT4(X) REPEAT2(REPEAT2(X))
// #define REPEAT16(X) REPEAT4(REPEAT4(X))
// #define REPEAT(X) REPEAT16(REPEAT16(X))
//
// constexpr auto n_repetitions = 256;

constexpr auto max_container_bytes = 1 << 22;
constexpr auto test_repetitions = 1;

template <int ElementBytes, template <typename> typename ContainerType>
static void BM_insert_in_sorted_order(benchmark::State& state) {
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
    return std::vector<ContainerType<ElementType>>(n_output_containers);
  }();

  for (auto _ : state) {
    for (auto const& [container_idx, element] : input_vector) {
      auto& current_container = output_containers[container_idx];
      auto it = current_container.begin();
      for (; it != current_container.end(); ++it) {
        if (element[0] < (*it)[0]) {
          break;
        }
      }
      benchmark::DoNotOptimize(current_container.insert(it, element));
    }
  }
  state.SetItemsProcessed(state.iterations() * test_size);
}

BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 4, std::vector)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 4, std::list)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 32, std::vector)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 32, std::list)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 256, std::vector)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 256, std::list)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 2048, std::vector)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
BENCHMARK_TEMPLATE(BM_insert_in_sorted_order, 2048, std::list)
    ->RangeMultiplier(2)
    ->Range(1 << 2, max_container_bytes)
    ->Iterations(1);
