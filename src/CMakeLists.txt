add_executable(sequential_containers_benchmark sequential_containers_benchmark.cpp)
target_link_libraries(sequential_containers_benchmark
  benchmark::benchmark
  benchmark::benchmark_main
)

add_executable(sorting_benchmark sorting_benchmark.cpp)
target_link_libraries(sorting_benchmark
  benchmark::benchmark
  benchmark::benchmark_main
)

add_custom_target(plot_benchmark
  COMMAND sequential_containers_benchmark --benchmark_out=data.json
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/plot_benchmark.py
)
add_dependencies(plot_benchmark sequential_containers_benchmark)
