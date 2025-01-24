cmake_minimum_required(VERSION 3.25) 
project(Mermaids CXX)               
set(CMAKE_CXX_STANDARD 17)            
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include("${CMAKE_SOURCE_DIR}/../../addons/prism/cmake/prism.cmake" REQUIRED)

# Add source/header files to project
file(GLOB_RECURSE SOURCES ../*.cpp SOURCES ../*.h SOURCES ../*.rc)
list(FILTER SOURCES EXCLUDE REGEX ".*web.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/build/.*")

add_link_options(/NODEFAULTLIB:libcmt.lib)
add_link_options(/IGNORE:4099,4286,4098)
add_link_options(/OPT:REF)
add_link_options(/OPT:ICF)

# Define exe
add_executable(Mermaids ${SOURCES})

# Link with libraries
target_link_libraries(Mermaids prism)
target_link_libraries(Mermaids SDL2)
target_link_libraries(Mermaids SDL2main)
target_link_libraries(Mermaids SDL2_image)
target_link_libraries(Mermaids SDL2_mixer)
target_link_libraries(Mermaids SDL2_ttf)
target_link_libraries(Mermaids enet)
target_link_libraries(Mermaids freetype)
target_link_libraries(Mermaids glew)
target_link_libraries(Mermaids curl)
target_link_libraries(Mermaids libjpeg)
target_link_libraries(Mermaids libpng)
target_link_libraries(Mermaids libwebp)
target_link_libraries(Mermaids zstd)
target_link_libraries(Mermaids zlib)
target_link_libraries(Mermaids crypt32)
target_link_libraries(Mermaids wldap32)
target_link_libraries(Mermaids ws2_32)
target_link_libraries(Mermaids winmm)
target_link_libraries(Mermaids OpenGL32)
target_link_libraries(Mermaids version)

# Define preprocessor definitions
target_compile_definitions(Mermaids PUBLIC UNICODE)
target_compile_definitions(Mermaids PUBLIC _UNICODE)

set_property(TARGET Mermaids PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(Mermaids PRIVATE /Gy)

set_target_properties(Mermaids PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../assets)
set_target_properties(Mermaids PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/../assets)

# Copy over DLLs
file(GLOB DLLS "${CMAKE_SOURCE_DIR}/../../addons/prism/windows/vs17/DLL/*.dll")
foreach(DLL ${DLLS})
  add_custom_command(TARGET Mermaids POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${DLL}
    $<TARGET_FILE_DIR:Mermaids>)
endforeach()

# Optimize for release build
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(Mermaids PRIVATE /O2)
endif()
