

file (GLOB sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/lib/*.c")

file (GLOB headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")

add_library(emu STATIC ${sources} ${headers})

target_include_directories(emu PUBLIC ${PROJECT_SOURCE_DIR}/include )


target_include_directories(emu PUBLIC ${SDL2_INCLUDE_DIR})
target_link_libraries(emu PUBLIC ${SDL2_LIBRARY})  

include_directories("/usr/local/include")
include_directories("/opt/homebrew/include")
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(emu ${SDL2_LIBRARIES})
