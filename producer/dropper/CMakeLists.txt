cmake_minimum_required(VERSION 3.30)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS

	${CMAKE_SOURCE_DIR}/dropper/src/*.cpp
)

set(INC_DIRECTORIES
	${CMAKE_SOURCE_DIR}/dropper/include/

	${CMAKE_SOURCE_DIR}/external/libarchive/
)

add_executable(dropper WIN32 ${SRC_FILES})

target_include_directories(dropper PRIVATE
	${INC_DIRECTORIES}
)

target_link_libraries(dropper PRIVATE
	common archive_static
)

target_compile_definitions(dropper PRIVATE
	SB_VERSION_MAJOR=${SB_VERSION_MAJOR}
	SB_VERSION_MINOR=${SB_VERSION_MINOR}
	SB_VERSION_PATCH=${SB_VERSION_PATCH}
)

if (MSVC)
	target_compile_options(dropper PRIVATE /w)
else()
	target_link_options(dropper PRIVATE
		-static
		-static-libgcc
		-static-libstdc++
	)
	target_compile_options (dropper PRIVATE -w)
endif()

set_target_properties(dropper PROPERTIES
	OUTPUT_NAME launcher
	VERSION "${SB_VERSION_MAJOR}.${SB_VERSION_MINOR}.${SB_VERSION_PATCH}"
)