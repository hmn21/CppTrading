find_package(quill CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

add_executable(HelloWorld helloworld.cpp)
target_link_libraries(HelloWorld PRIVATE quill::quill)

add_executable(CacheBench cachebench.cpp)
target_link_libraries(CacheBench PRIVATE benchmark::benchmark)

add_executable(ContainerBench containerbench.cpp)
target_link_libraries(ContainerBench PRIVATE absl::absl_check absl::random_random absl::flat_hash_map absl::node_hash_map benchmark::benchmark)