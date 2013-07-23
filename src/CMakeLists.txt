set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/../common_external/boost_1_53_0)
set(Boost_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/../common_external/boost_1_53_0/stage/lib)

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
               BrickManager.cpp
	        Config.cpp
               Utils.cpp
               WindowManager.cpp
               Renderer.cpp
               Raycaster.cpp
               Texture.cpp
               Texture1D.cpp
               Texture2D.cpp
               Texture3D.cpp
               Animator.cpp
               MappingKey.cpp
               TransferFunction.cpp
               TSP.cpp
               CLManager.cpp
               CLProgram.cpp
               ShaderProgram.cpp)

target_link_libraries(FlareApp
                      ${GLEW_LIBRARY}
                      ${GLFW_LIBRARY}
                      ${OPENGL_LIBRARY}
                      ${OPENCL_LIBRARIES}
                      ${Boost_LIBRARIES})
