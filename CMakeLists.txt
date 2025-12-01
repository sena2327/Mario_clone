cmake_minimum_required(VERSION 3.20)
project(Mario CXX)

# C++ のバージョン
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# コンパイル時の警告オプション（お好みで）
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(mario
    main.cpp
)

# ---- SDL2 ----
# Homebrew などでインストールされていることを想定
#   brew install sdl2 sdl2_image
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

# 新しめの CMake の FindSDL2 / FindSDL2_image ならこっち
if (TARGET SDL2::SDL2 AND TARGET SDL2_image::SDL2_image)
    target_link_libraries(mario PRIVATE
        SDL2::SDL2
        SDL2_image::SDL2_image
    )
else()
    # 古い FindSDL2.cmake / 手書きモジュールの場合はこちらの変数が定義される想定
    target_include_directories(mario PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
    )
    target_link_libraries(mario PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
    )
endif()