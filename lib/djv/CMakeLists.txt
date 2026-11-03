add_subdirectory(App)
add_subdirectory(Models)
add_subdirectory(Resource)
add_subdirectory(UI)
# The C interface over the models, for DJV Studio's iPad application,
# wherever tlRender has its own: built and tested on the desktop too.
if(TARGET tlRender::tlC)
    add_subdirectory(C)
endif()

if(DJV_PYTHON)
    add_subdirectory(ModelsPy)
    add_subdirectory(UIPy)
    add_subdirectory(AppPy)
    add_subdirectory(djvPy)
endif()

if(DJV_TESTS)
    add_subdirectory(ModelsTest)
endif()
