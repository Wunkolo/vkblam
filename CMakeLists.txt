cmake_minimum_required( VERSION 3.21.0 )
project( vkblam CXX )

### Standard
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

set( CMAKE_COLOR_MAKEFILE ON )

# Generate 'compile_commands.json' for clang_complete
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if( MSVC )
elseif( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	# Force colored diagnostic messages in Ninja's output
	if( CMAKE_GENERATOR STREQUAL "Ninja" )
		add_compile_options( -fdiagnostics-color=always )
	endif()
endif()

find_package( Vulkan REQUIRED )

find_package( glm REQUIRED )

# mio
add_subdirectory( external/mio )

#cmrc
include( external/cmrc/CMakeRC.cmake )

### shaders build target
file(
	GLOB_RECURSE GLSL_SOURCE_FILES
	"shaders/*.frag"
	"shaders/*.vert"
	"shaders/*.comp"
)

# Get glslangValidator
if( WIN32 )
	if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64" )
		set( GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator" )
	else()
		set( GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator" )
	endif()
else()
	set( GLSL_VALIDATOR "glslangValidator" )
endif()

# Get spirv-opt
if( WIN32 )
	if( ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64" )
		set( SPIRV_OPT "$ENV{VULKAN_SDK}/Bin/spirv-opt" )
	else()
		set( SPIRV_OPT "$ENV{VULKAN_SDK}/Bin32/spirv-opt" )
	endif()
else()
	set( SPIRV_OPT "spirv-opt" )
endif()

foreach( GLSL ${GLSL_SOURCE_FILES} )
	get_filename_component( FILE_NAME ${GLSL} NAME )
	set( SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv" )
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
		COMMAND ${GLSL_VALIDATOR} -t --target-env vulkan1.1 -g -V ${GLSL} -o ${SPIRV}
		COMMAND ${SPIRV_OPT} -O ${SPIRV} -o ${SPIRV}
		DEPENDS ${GLSL}
	)
	list( APPEND SPIRV_BINARY_FILES ${SPIRV} )
endforeach()

add_custom_target(
	shaders 
	DEPENDS ${SPIRV_BINARY_FILES}
)

### common
add_library(
	common
	source/Common/Format.cpp
)
target_include_directories(
	common
	PRIVATE
	include
)

### blam
add_library(
	blam
	source/Blam/Blam.cpp
	source/Blam/Util.cpp
)
target_include_directories(
	blam
	PRIVATE
	include
)
target_link_libraries(
	blam
	PRIVATE
	common
)

### dump-bsp
add_executable(
	dump-bsp
	source/dump-bsp.cpp
)
target_include_directories(
	dump-bsp
	PRIVATE
	include
)
target_link_libraries(
	dump-bsp
	PRIVATE
	blam
	mio::mio
)


### vkblam

cmrc_add_resource_library(
	vkblam-resources
	ALIAS Resource::vkblam
	NAMESPACE vkblam
	WHENCE ${PROJECT_BINARY_DIR}
	"${PROJECT_BINARY_DIR}/shaders/Debug.vert.spv"
	"${PROJECT_BINARY_DIR}/shaders/Debug.frag.spv"
)
add_dependencies(vkblam-resources shaders)

add_executable(
	vkblam
	source/main.cpp
	source/stb_image_write.cpp
	source/Vulkan/Debug.cpp
	source/Vulkan/Memory.cpp
	source/Vulkan/VulkanAPI.cpp
)
target_include_directories(
	vkblam
	PRIVATE
	include
)
target_link_libraries(
	vkblam
	PRIVATE
	vkblam-resources
	blam
	Vulkan::Vulkan
	mio::mio
	glm
	${CMAKE_DL_LIBS}
)