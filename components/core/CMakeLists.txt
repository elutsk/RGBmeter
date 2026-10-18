idf_component_register(SRCS "App.cpp" "Power.cpp"
                       INCLUDE_DIRS "include")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)