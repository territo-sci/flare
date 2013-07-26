set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules")

if (NOT WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	find_package(OpenGL REQUIRED)
	find_package(GLFW REQUIRED)
	find_package(OpenCL REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(Boost COMPONENTS timer system REQUIRED)
endif ()

#set(Boost_INCLUDE_DIR "C:/alebo68/boost")
#set(Boost_LIBRARY_DIR "C:/alebo68/boost/stage/lib")
if (WIN32)

	# SGCT special case
	ADD_DEFINITIONS(-D__WIN32__)
	SET(SGCT_DEBUG_LIBRARY "C:/Program Files (x86)/SGCT/SGCT_1.5.2_x86/lib/msvc11/sgct32_d.lib" )
	SET(SGCT_RELEASE_LIBRARY "C:/Program Files (x86)/SGCT/SGCT_1.5.2_x86/lib/msvc11/sgct32.lib" )
	SET(SGCT_INCLUDE_DIRECTORY "C:/Program Files (x86)/SGCT/SGCT_1.5.2_x86/include" )
	SET(SGCT_LINK_LIBRARY 
		optimized ${SGCT_RELEASE_LIBRARY}
		debug ${SGCT_DEBUG_LIBRARY}
	)

    #set(Boost_INCLUDE_DIR "C:/alebo68/boost")
	set(Boost_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../external/boost_1_54_0")
    #set(Boost_LIBRARIES
    #    "C:/alebo68/boost/stage/lib/libboost_timer-vc110-mt-1_53.lib"
    #    "C:/alebo68/boost/stage/lib/libboost_system-vc110-mt-1_53.lib"
    #    "C:/alebo68/boost/stage/lib/libboost_chrono-vc110-mt-1_53.lib"
    #    )
    
    #set(GLEW_INCLUDE_PATH "C:/alebo68/glew-1.9.0/include")
    #set(GLEW_LIBRARY "C:/alebo68/glew-1.9.0/lib/glew32.lib")
	set(GLEW_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/../external/glew-1.10.0/include")
	set(GLEW_LIBRARY "${CMAKE_SOURCE_DIR}/../external/glew-1.10.0/lib/Release/Win32/glew32s.lib")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DGLEW_STATIC)
	
	set(OpenCL_INCPATH "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.0/include")
    set(OpenCL_LIBPATH "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v5.0/lib/Win32")

	#set(GLFW_ROOT_DIR "C:/alebo68/sgct/src/deps/glfw27")
	set(GLFW_ROOT_DIR "${CMAKE_SOURCE_DIR}/../external/glfw-2.7.9")
	message("${GLFW_ROOT_DIR}")
    
    find_package(OpenGL REQUIRED)
    find_package(GLFW REQUIRED)
    find_package(OpenCL REQUIRED)
endif ()


set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../external/glm")

include_directories(${CMAKE_SOURCE_DIR}/inc
                    ${OPENGL_INCLUDE_DIR}
                    ${OPENCL_INCLUDE_DIRS}
                    ${GLFW_INCLUDE_DIRS}
                    ${GLEW_INCLUDE_PATH}
                    ${GLM_INCLUDE_DIR}
                    ${Boost_INCLUDE_DIR}
					${SGCT_INCLUDE_DIRECTORY}
					)

link_directories(${Boost_LIBRARY_DIR})

add_executable(FlareApp
               FlareApp.cpp
               BrickManager.cpp
			   Config.cpp
               Utils.cpp
               #WindowManager.cpp
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
               ShaderProgram.cpp
			   SGCTWinManager.cpp)

target_link_libraries(FlareApp
                      ${GLEW_LIBRARY}
                      ${GLFW_LIBRARY}
                      ${OPENGL_LIBRARY}
                      ${OPENCL_LIBRARIES}
                      ${Boost_LIBRARIES}
					  ${SGCT_LINK_LIBRARY})
