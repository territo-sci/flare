set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GLEW REQUIRED)

set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/glm-0.9.4.3")

include_directories(${CMAKE_SOURCE_DIR}/inc
                    ${OPENGL_INCLUDE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${GLFW_INCLUDE_DIRS}
                    ${GLEW_INCLUDE_PATH}
                    ${GLM_INCLUDE_DIR})

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
                      ${OPENCL_LIBRARIES})

