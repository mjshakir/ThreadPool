project('threadpool', 'cpp', version: '1.0', default_options: ['cpp_std=c++20', 'warning_level=3'])

# Import the fs module
fs = import('fs')

# Determine if ThreadPool is built as a standalone project or included by other projects
is_standalone = meson.source_root() == meson.project_source_root()



# Compiler-specific flags
if meson.get_compiler('cpp').get_id() == 'msvc'
  add_project_arguments('/W4', language: 'cpp')
else
  add_project_arguments(
    ['-Wpedantic', '-pedantic-errors', '-Werror=format-security', '-fasynchronous-unwind-tables'],
    language: 'cpp'
  )
endif

# Define the source and include directories
threadpool_includes = include_directories('include')
threadpool_sources = files(
  'src/ThreadPool.cpp',
  'src/PriorityQueue.cpp',
  'src/ThreadTask.cpp'
)

# Create the ThreadPool library
threadpool_lib = library(
  'threadpool',
  threadpool_sources,
  include_directories: threadpool_includes,
  install: true
)

# Dependencies
gtest_dep = dependency('gtest', required: false)
if not gtest_dep.found()
  gtest_proj = subproject('googletest', default_options: ['build_gmock=false'])
  gtest_dep = gtest_proj.get_variable('gtest_dep')
endif

benchmark_dep = dependency('benchmark', required: false)
if not benchmark_dep.found()
  benchmark_proj = subproject('googlebenchmark')
  benchmark_dep = benchmark_proj.get_variable('benchmark_dep')
endif

# Tests
if get_option('build_threadpool_tests')
  test_sources = files(
    'test/PriorityQueueTest.cpp',
    'test/PriorityQueueDequeTest.cpp',
    'test/PriorityQueueStructTest.cpp',
    'test/PriorityQueueDegueStructTest.cpp',
    'test/ThreadTaskTest.cpp',
    'test/ThreadPoolTest.cpp',
    'test/ThreadPoolDisableTest.cpp',
    'test/ThreadPoolDequeTest.cpp',
    'test/ThreadPoolDisableDequeTest.cpp'
  )

  foreach test_src : test_sources
    test_name = fs.stem(test_src)  # Extract the filename without the extension
    exe = executable(
      test_name,
      test_src,
      include_directories: threadpool_includes,
      link_with: [threadpool_lib],
      dependencies: [gtest_dep],
      install: true  # Install the tests
    )
    test(test_name, exe)
  endforeach
endif

# Benchmarks
if get_option('build_threadpool_benchmark')
  benchmark_sources = files(
    'benchmark/PriorityQueueBenchmark.cpp',
    'benchmark/ThreadTaskBenchmark.cpp'
  )

  foreach bench_src : benchmark_sources
    bench_name = fs.stem(bench_src)  # Extract the filename without the extension
    exe = executable(
      bench_name,
      bench_src,
      include_directories: threadpool_includes,
      link_with: [threadpool_lib],
      dependencies: [benchmark_dep],
      install: true  # Install the benchmarks
    )
    test(bench_name, exe)
  endforeach
endif

# Examples
if get_option('build_threadpool_example')
  example_exe = executable(
    'threadpool_example',
    'example/main.cpp',
    include_directories: threadpool_includes,
    link_with: [threadpool_lib],
    install: true  # Ensure installation
  )
endif

# Documentation
doxygen = find_program('doxygen', required: false)
if doxygen.found()
  custom_target(
    'docs',
    command: [doxygen, 'docs/Doxyfile.in'],
    output: 'docs',
    install: false
  )
endif

# Installation
install_headers(
  files(
    'include/ThreadPool.hpp',
    'include/PriorityQueue.hpp',
    'include/ThreadTask.hpp'
  ),
  subdir: 'threadpool'
)