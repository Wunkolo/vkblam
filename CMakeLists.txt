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

add_subdirectory( external/mio )

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
add_executable(
	vkblam
	source/main.cpp
	source/stb_image_write.cpp
	source/Vulkan/VulkanAPI.cpp
	source/Vulkan/Memory.cpp
)
target_include_directories(
	vkblam
	PRIVATE
	include
)
target_link_libraries(
	vkblam
	PRIVATE
	blam
	Vulkan::Vulkan
	mio::mio
	glm
	${CMAKE_DL_LIBS}
)