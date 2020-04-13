find_package(LLVM REQUIRED CONFIG)

llvm_map_components_to_libnames(LLVM_LIBS core orcjit native)

set(SOURCES "Lexer.cpp"
            "Parser.cpp"
            "main.cpp")

add_executable(kaleidoscope ${SOURCES})
target_compile_features(kaleidoscope PUBLIC cxx_std_17)
target_include_directories(kaleidoscope PRIVATE .
                                                ../submodules/multiple-dispatch/include
                                                ${LLVM_INCLUDE_DIRS})
target_compile_definitions(kaleidoscope PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(kaleidoscope PRIVATE ${LLVM_LIBS})
