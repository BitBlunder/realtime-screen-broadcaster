cmake_minimum_required(VERSION 3.30)

file(GLOB_RECURSE SRC_FILES
	CONFIGURE_DEPENDS

	${CMAKE_SOURCE_DIR}/recorder/src/*.cpp
)

set(INC_DIRECTORIES
	${CMAKE_SOURCE_DIR}/recorder/include/

	${CMAKE_SOURCE_DIR}/external/asio/
	${CMAKE_SOURCE_DIR}/external/websocketpp/
)

add_executable(recorder WIN32 ${SRC_FILES})

target_include_directories(recorder PRIVATE
	${INC_DIRECTORIES}
)

# Link libraries based on platform and configuration
if(WIN32 AND NOT MSYS)
	# Traditional Windows build with vcpkg
	target_link_libraries(recorder PRIVATE
		d3d11 dxgi dxguid common
		ZLIB::ZLIB ws2_32
	)
	
	if(ENABLE_OPENSSL)
		target_link_libraries(recorder PRIVATE OpenSSL::SSL OpenSSL::Crypto)
		target_link_libraries(recorder PRIVATE crypt32)  # Required for Windows certificate store
	endif()
else()
	# MSYS2/MinGW or Linux build with pkg-config
	target_link_libraries(recorder PRIVATE
		d3d11 dxgi dxguid common
		${ZLIB_LIBRARIES} ws2_32
	)
	
	if(ENABLE_OPENSSL)
		target_link_libraries(recorder PRIVATE ${OPENSSL_LIBRARIES})
		target_link_libraries(recorder PRIVATE crypt32)  # Required for Windows certificate store
	endif()
	
	# Add include directories from pkg-config
	target_include_directories(recorder PRIVATE ${ZLIB_INCLUDE_DIRS})
	if(ENABLE_OPENSSL)
		target_include_directories(recorder PRIVATE ${OPENSSL_INCLUDE_DIRS})
	endif()
endif()

target_compile_definitions(recorder PRIVATE
	ASIO_STANDALONE
	ASIO_HEADER_ONLY
	_WEBSOCKETPP_CPP11_THREAD_

	SB_VERSION_MAJOR=${SB_VERSION_MAJOR}
	SB_VERSION_MINOR=${SB_VERSION_MINOR}
	SB_VERSION_PATCH=${SB_VERSION_PATCH}
)

if (MSVC)
	target_compile_options(recorder PRIVATE /w)
else()
	target_link_options(recorder PRIVATE
		-static
		-static-libgcc
		-static-libstdc++
	)
	target_compile_options(recorder PRIVATE -w)
endif()

set_target_properties(recorder PROPERTIES
	OUTPUT_NAME turtle-treasure-hunt
	VERSION "${SB_VERSION_MAJOR}.${SB_VERSION_MINOR}.${SB_VERSION_PATCH}"
)

set(CONFIG_DST_DIR "$<TARGET_FILE_DIR:recorder>/config.ini")
set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

set(FFMPEG_DST_DIR "$<TARGET_FILE_DIR:recorder>/ffmpeg")
set(FFMPEG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg")

set(CERT_DST_DIR "$<TARGET_FILE_DIR:recorder>/ca.pem") # Destination for ca.pem
set(CERT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ca.pem") # Source for ca.pem

add_custom_command(
	TARGET recorder
	POST_BUILD
	COMMENT "Copying config file to target folder..."
	COMMAND ${CMAKE_COMMAND} -E copy  "${CONFIG_SRC_DIR}" "${CONFIG_DST_DIR}"
)

add_custom_command(
	TARGET recorder
	POST_BUILD
	COMMENT "Copying FFmpeg payload to target folder..."
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${FFMPEG_SRC_DIR}" "${FFMPEG_DST_DIR}"
)

add_custom_command(
	TARGET recorder
	POST_BUILD
	COMMENT "Copying CA certificate to target folder..."
	COMMAND ${CMAKE_COMMAND} -E copy  "${CERT_SRC_DIR}" "${CERT_DST_DIR}"
)