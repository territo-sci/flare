set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")
set(OPENSPACE_DIR /home/vsand/OpenSpace)

set(Boost_INCLUDE_DIR ${OPENSPACE_DIR}/common_external/boost_1_53_0)
set(Boost_LIBRARY_DIR ${OPENSPACE_DIR}/common_external/boost_1_53_0/stage/lib)

find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Boost COMPONENTS timer system REQUIRED)

set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/glm-0.9.4.3")

include_directories(${CMAKE_SOURCE_DIR}/inc
                    ${OPENGL_INCLUDE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${GLFW_INCLUDE_DIRS}
                    ${GLEW_INCLUDE_PATH}
                    ${GLM_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIR})

add_executable(FlareApp
               FlareApp.cpp
               Utils.cpp
               WindowManager.cpp
               Renderer.cpp
               Raycaster.cpp
               Texture.cpp
               Texture1D.cpp
               Texture2D.cpp
               Texture3D.cpp
               PixelBuffer.cpp
               CLHandler.cpp
               Animator.cpp
               MappingKey.cpp
               TransferFunction.cpp
               VDFReader.cpp
               ShaderProgram.cpp)

target_link_libraries(FlareApp
                      ${GLEW_LIBRARY}
                      ${GLFW_LIBRARY}
                      ${OPENGL_LIBRARY}
                      ${OPENCL_LIBRARIES}
                      ${Boost_LIBRARIES}
              	      pthread)

set(CMAKE_BUILD_TYPE Debug)
