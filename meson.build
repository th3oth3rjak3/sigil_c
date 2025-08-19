project('sigil', 'c', version: '0.0.1')

includes = include_directories(
  [
    'src/compiler',
    'src/debug',
    'src/error_handling',
    'src/memory',
    'src/runtime',
    'src/scanner',
    'src/types',
  ],
)

lib_sources = [
  'src/compiler/compiler.c',
  'src/debug/debug.c',
  'src/error_handling/error_handler.c',
  'src/memory/memory.c',
  'src/runtime/bytecode.c',
  'src/runtime/vm.c',
  'src/scanner/scanner.c',
  'src/types/hash_map.c',
  'src/types/object.c',
  'src/types/value.c',
]

executable('sigil', ['src/main.c'] + lib_sources, include_directories: includes)

unity_dep = dependency('unity', fallback: ['unity', 'unity_dep'])
testing_deps = [unity_dep]

# Define your test files
test_files = [
  'src/compiler/compiler_tests.c',
  'src/error_handling/error_handler_tests.c',
  'src/memory/memory_tests.c',
  'src/runtime/bytecode_tests.c',
  'src/runtime/vm_tests.c',
  'src/scanner/scanner_tests.c',
  'src/types/hash_map_tests.c',
  'src/types/object_tests.c',
  'src/types/value_tests.c',
]

# Create and register tests
foreach test_file : test_files
  # Extract test name from path (remove extension and path components)
  test_name = test_file.split('/')[-1].split('.')[0]

  # Create executable
  test_exe = executable(
    test_name,
    [lib_sources, test_file],
    c_args: ['-fsanitize=address'],
    link_args: ['-fsanitize=address'],
    include_directories: includes,
    dependencies: testing_deps,
  )

  # Register test
  test(test_name, test_exe)
endforeach