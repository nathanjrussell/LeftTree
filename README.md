# LeftTree
Binary Tree structure that grows toward the left and recursively returns upward

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run tests

```sh
ctest --test-dir build --output-on-failure
```

## Use from another CMake project (FetchContent by tag)

In your parent project's `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
  LeftTree
  GIT_REPOSITORY https://github.com/<org-or-user>/LeftTree.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(LeftTree)

target_link_libraries(your_target PRIVATE LeftTree::LeftTree)
```

## Library layout

- `include/LeftTree` public headers
- `src` implementation
- `testing` unit tests (GoogleTest)
- `examples` small sample program(s)
