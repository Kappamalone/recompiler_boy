set(SOURCES 
    core_wrapper.h
    core.h
    core.cpp
    common.h
    interpreter.h
    interpreter.cpp
    ppu.cpp
    config.h
    mbc.cpp
    mbc.h
    cached_interpreter.h
    cached_interpreter.cpp
    common_recompiler.h
)

add_library(core ${SOURCES})
target_include_directories(core PUBLIC .)
target_link_libraries(core PUBLIC fmt xbyak) # public so that we can access common.h 
