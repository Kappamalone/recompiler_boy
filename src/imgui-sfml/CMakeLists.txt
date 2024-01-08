SET(SOURCES
    main.cpp
    gui.h
)

# Windows specific config
IF (WIN32)
    # Link sfml statically
    set(SFML_STATIC_LIBRARIES TRUE)
ENDIF()

# Find SFML/-IMGUI shared libraries
# SET(ImGui-SFML_DIR .)
# find_package(SFML COMPONENTS system window graphics audio CONFIG REQUIRED)
# find_package(ImGui-SFML REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SFML_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE core sfml-system sfml-graphics sfml-window ImGui-SFML::ImGui-SFML)
