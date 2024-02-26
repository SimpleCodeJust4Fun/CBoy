
set(MAIN_SOURCES
  main.c
)

file (GLOB headers "${PROJECT_SOURCE_DIR}/include/*.h")

add_executable(gbemu ${HEADERS} ${MAIN_SOURCES})

target_link_libraries(gbemu emu)
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your output executable to .html
    target_link_libraries(gbemu "-s USE_SDL=2")
endif()
target_include_directories(gbemu PUBLIC ${PROJECT_SOURCE_DIR}/include )

message(STATUS "SDL Libraries: ${SDL2_LIBRARIES} - ${SDL2_LIBRARY}")
message(STATUS "SDL TTF Libraries: ${SDL2_TTF_LIBRARIES} - ${SDL2_TTF_LIBRARY}")


install(TARGETS gbemu
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)

if (WIN32)

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2/lib/x64/SDL2.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x64/SDL2_ttf.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x64/zlib1.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x64/libfreetype-6.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
else ()
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2/lib/x86/SDL2.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x86/SDL2_ttf.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x86/zlib1.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/lib/x86/libfreetype-6.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    VERBATIM)
endif ()

endif()

