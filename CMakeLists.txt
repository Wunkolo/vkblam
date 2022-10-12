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
	add_compile_options(
		/MP # Parallel builds 
		/permissive- # Stricter C++ conformance

		# Warnings
		/W3

		# Consider these warnings as errors
		/we4018 # 'expression': signed/unsigned mismatch
		/we4062 # Enumerator 'identifier' in a switch of enum 'enumeration' is not handled
		/we4101 # 'identifier': unreferenced local variable
		/we4265 # 'class': class has virtual functions, but destructor is not virtual
		/we4305 # 'context': truncation from 'type1' to 'type2'
		/we4388 # 'expression': signed/unsigned mismatch
		/we4389 # 'operator': signed/unsigned mismatch

		/we4456 # Declaration of 'identifier' hides previous local declaration
		/we4457 # Declaration of 'identifier' hides function parameter
		/we4458 # Declaration of 'identifier' hides class member
		/we4459 # Declaration of 'identifier' hides global declaration

		/we4505 # 'function': unreferenced local function has been removed
		/we4547 # 'operator': operator before comma has no effect; expected operator with side-effect
		/we4549 # 'operator1': operator before comma has no effect; did you intend 'operator2'?
		/we4555 # Expression has no effect; expected expression with side-effect
		/we4715 # 'function': not all control paths return a value
		/we4834 # Discarding return value of function with 'nodiscard' attribute
		/we5038 # data member 'member1' will be initialized after data member 'member2'
		/we5245 # 'function': unreferenced function with internal linkage has been removed

	)
elseif( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
	# Force colored diagnostic messages in Ninja's output
	if( CMAKE_GENERATOR STREQUAL "Ninja" )
		add_compile_options( -fdiagnostics-color=always )
	endif()

	add_compile_options(
		-Wall
		-Warray-bounds
		-Wextra
		-Wimplicit-fallthrough
		-Wmissing-declarations
		-Wmissing-declarations
		-Wmissing-field-initializers
		-Wno-attributes
		-Wno-invalid-offsetof
		-Wno-unused-parameter
		-Wreorder
		-Wshadow
		-Wsign-compare
		-Wswitch
		-Wuninitialized
		-Wunused-function
		-Wunused-result
		-Wunused-variable
	)
endif()


find_package( Vulkan 1.3.206 REQUIRED )

find_package( glm 0.9.9.9 QUIET )

if( glm_FOUND )
else()
	add_subdirectory( external/glm EXCLUDE_FROM_ALL )
endif()

# mio
add_subdirectory( external/mio EXCLUDE_FROM_ALL )

#cmrc
include( external/cmrc/CMakeRC.cmake )

### shaders build target
file(
	GLOB_RECURSE GLSL_SOURCE_FILES
	"shaders/*.frag"
	"shaders/*.vert"
	"shaders/*.comp"
)

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
		COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -t --target-env vulkan1.1 -g -V ${GLSL} -o ${SPIRV}
		#COMMAND ${SPIRV_OPT} -O ${SPIRV} -o ${SPIRV}
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
	source/Blam/TagVisitor.cpp
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

### decrypt-shader
add_executable(
	decrypt-shader
	source/decrypt-shader.cpp
)
target_include_directories(
	decrypt-shader
	PRIVATE
	include
)
target_link_libraries(
	decrypt-shader
	PRIVATE
	mio::mio
)

### vkblam

cmrc_add_resource_library(
	vkblam-resources
	ALIAS Resource::vkblam
	NAMESPACE vkblam
	WHENCE ${PROJECT_BINARY_DIR}
	${SPIRV_BINARY_FILES}
)
add_dependencies(vkblam-resources shaders)

add_executable(
	vkblam
	source/main.cpp
	source/stb_image_write.cpp
	source/VkBlam/Renderer.cpp
	source/VkBlam/Scene.cpp
	source/VkBlam/SceneView.cpp
	source/VkBlam/Shader.cpp
	source/VkBlam/Shaders/ShaderEnvironment.cpp
	source/VkBlam/VkBlam.cpp
	source/VkBlam/World.cpp
	source/Vulkan/Debug.cpp
	source/Vulkan/DescriptorHeap.cpp
	source/Vulkan/DescriptorUpdateBatch.cpp
	source/Vulkan/Memory.cpp
	source/Vulkan/Pipeline.cpp
	source/Vulkan/SamplerCache.cpp
	source/Vulkan/StreamBuffer.cpp
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