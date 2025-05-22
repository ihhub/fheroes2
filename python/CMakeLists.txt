find_package(Python3 COMPONENTS Development REQUIRED)

add_library(pyfheroes2 MODULE pyfheroes2.cpp)

target_include_directories(pyfheroes2 PRIVATE
    ${Python3_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/src/engine
    ${PROJECT_SOURCE_DIR}/src/fheroes2
)

target_link_libraries(pyfheroes2 PRIVATE engine)

set_target_properties(pyfheroes2 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "pyfheroes2"
)

