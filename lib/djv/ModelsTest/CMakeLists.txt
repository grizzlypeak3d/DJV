set(HEADERS
    AudioModelTest.h
    ModelsTestUtil.h
    TimeUnitsModelTest.h)

set(SOURCE
    AudioModelTest.cpp
    TimeUnitsModelTest.cpp)

add_library(djvModelsTest ${SOURCE} ${HEADERS})

target_include_directories(djvModelsTest
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djvModelsTest djvModels ftkTestLib)

set_target_properties(djvModelsTest PROPERTIES FOLDER tests)
