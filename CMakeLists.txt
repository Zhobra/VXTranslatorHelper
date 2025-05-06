cmake_minimum_required(VERSION 3.28)
project(VX_Translator_Helper)

enable_testing()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(fmt)

add_executable(bracket_test bracket_test.cpp)
target_link_libraries(bracket_test fmt::fmt)

if(MSVC)
  target_compile_options(bracket_test PRIVATE /W4 /WX)
else()
  target_compile_options(bracket_test PRIVATE -Wall -Wextra -Wpedantic -Werror) #${TARGET_NAME}
endif()

add_executable(VXTranslatorHelper main.cpp)
target_link_libraries(VXTranslatorHelper fmt::fmt)
#utfcpp author explicitly tells not to rely on the lib's shipped CMakeLists.txt, as it's meant to be header-only

if(MSVC)
  target_compile_options(VXTranslatorHelper PRIVATE /W4 /WX)
else()
  target_compile_options(VXTranslatorHelper PRIVATE -Wall -Wextra -Wpedantic) #-Werror #${TARGET_NAME}
endif()