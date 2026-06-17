cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Renderer")

find_package(PNG REQUIRED)
find_package(glm REQUIRED)
find_package(SDL3 REQUIRED)

file(GLOB SRC_FILES "src/*.cpp")
file(GLOB RENDER_FILES "src/Render/*.cpp")
file(GLOB PIPELINE_FILES "src/Render/Pipeline/*.cpp")
file(GLOB RESOURCES_FILES "src/Resources/*.cpp")
file(GLOB SHADER_FILES "src/Shaders/*.cpp")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${RENDER_FILES} ${PIPELINE_FILES} ${RESOURCES_FILES} ${SHADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC src/)

target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
endif()

if(MSVC)
    # For Microsoft Visual C++
    target_compile_options(${PROJECT_NAME} PRIVATE /arch:AVX2)
    target_compile_definitions(${PROJECT_NAME} PUBLIC vectorcall=__vectorcall)
else()
    # For GCC, Clang, and Apple Clang
    target_compile_options(${PROJECT_NAME} PRIVATE -mavx2)
    target_compile_definitions(${PROJECT_NAME} PUBLIC vectorcall=) # GCC should use vector registers automatically
endif()

target_compile_options(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Release>:/O2>")