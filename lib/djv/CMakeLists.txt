add_subdirectory(App)
add_subdirectory(Models)
add_subdirectory(Resource)
add_subdirectory(UI)

if(DJV_TESTS)
    add_subdirectory(ModelsTest)
endif()
