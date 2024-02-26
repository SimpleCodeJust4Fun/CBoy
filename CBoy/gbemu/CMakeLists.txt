
set(MAIN_SOURCES
  main.c
)

file (GLOB headers "${PROJECT_SOURCE_DIR}/include/*.h")

add_executable(gbemu ${HEADERS} ${MAIN_SOURCES})

target_link_libraries(gbemu emu)
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your output executable to .html
    target_link_libraries(gbemu "-s USE_SDL=2" "-s USE_PTHREADS=1" "-s PTHREAD_POOL_SIZE=4")
endif()
target_include_directories(gbemu PUBLIC ${PROJECT_SOURCE_DIR}/include )

message(STATUS "SDL Libraries: ${SDL2_LIBRARIES} - ${SDL2_LIBRARY}")

install(TARGETS gbemu
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)
