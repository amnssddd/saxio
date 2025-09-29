file(GLOB SRC_FILES "*.cpp")

foreach (path ${SRC_FILES})
    get_filename_component(base_name ${path} NAME_WE)
    add_executable(${base_name} ${path})
    target_link_libraries(${base_name} ${DEPEND_LIBS})
    add_test(NAME ${base_name} COMMAND ${base_name})
endforeach()