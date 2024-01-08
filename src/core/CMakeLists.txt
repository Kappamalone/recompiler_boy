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
)

add_library(core ${SOURCES})
target_include_directories(core PUBLIC .)
target_link_libraries(core PUBLIC fmt) # public so that we can access common.h 
