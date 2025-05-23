cmake_minimum_required(VERSION 3.30)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS

	${CMAKE_SOURCE_DIR}/common/src/*.cpp
)

set(INC_DIRECTORIES
	${CMAKE_SOURCE_DIR}/common/include/
)

add_library(common STATIC ${SRC_FILES})

target_include_directories(common PUBLIC
	${INC_DIRECTORIES}
)

target_compile_definitions(common PUBLIC
	LOG_ENABLED=$<BOOL:${LOG_ENABLED}>
	LOG_ATTACH_CONSOLE=$<BOOL:${LOG_ATTACH_CONSOLE}>
)

if (MSVC)
	target_compile_options(common PRIVATE /w)
else()
	target_link_options(common PRIVATE
		-static
		-static-libgcc
		-static-libstdc++
	)
	target_compile_options(common PRIVATE -w)
endif()

set_target_properties(common PROPERTIES
	OUTPUT_NAME common
)
