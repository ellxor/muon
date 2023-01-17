project(
  'muon',
  'c', 'cpp',

  default_options : [
    'c_std=c2x',
    'cpp_std=c++20',
    'cpp_eh=none',
    'cpp_rtti=false',

    'warning_level=3',
    'werror=true',

    'buildtype=release',
    'b_lto=true',
    'b_pie=true',
  ]
)

SDL = dependency('SDL2', version: '>= 2.0.17')
SDI = dependency('SDL2_image')

SRC   = declare_dependency(include_directories: ['src/'])
IMGUI = declare_dependency(include_directories: ['src/graphics/imgui'])

add_project_arguments('-DUSE_PEXT', language : ['c', 'cpp'])
add_project_arguments('-march=native', language : ['c', 'cpp'])

test('movegen-test',
  executable('movegen-test', [
    'src/common/bitbase.c',
    'src/common/movegen.c',
    'src/common/unittest.c',
    'src/parser/fen.c',
  ],
  dependencies: SRC)
)

executable(
  'muon', [
    'src/main.c',
    'src/common/bitbase.c',
    'src/common/gametree.c',
    'src/common/movegen.c',
    'src/graphics/renderer.cc',
    'src/graphics/textures.c',
    'src/graphics/window.c',

    'src/graphics/imgui/imgui.cpp',
    'src/graphics/imgui/imgui_draw.cpp',
    'src/graphics/imgui/imgui_tables.cpp',
    'src/graphics/imgui/imgui_widgets.cpp',

    'src/graphics/imgui/backends/imgui_impl_sdl.cpp',
    'src/graphics/imgui/backends/imgui_impl_sdlrenderer.cpp',
  ],

  dependencies: [SDL, SDI, SRC, IMGUI],
)

