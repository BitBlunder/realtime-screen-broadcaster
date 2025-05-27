cmake_minimum_required (VERSION 3.30)
project (screen-broadcaster VERSION 0.1.0 LANGUAGES CXX)

set (BUILD_SHARED_LIBS OFF)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set (SB_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} CACHE INTERNAL "")
set (SB_VERSION_MINOR ${PROJECT_VERSION_MINOR} CACHE INTERNAL "")
set (SB_VERSION_PATCH ${PROJECT_VERSION_PATCH} CACHE INTERNAL "")

set (ENABLE_ACL     OFF CACHE BOOL "" FORCE)
set (ENABLE_TAR     OFF CACHE BOOL "" FORCE)
set (ENABLE_CAT     OFF CACHE BOOL "" FORCE)
set (ENABLE_LZMA    ON  CACHE BOOL "" FORCE)
set (ENABLE_CPIO    OFF CACHE BOOL "" FORCE)
set (ENABLE_TEST    OFF CACHE BOOL "" FORCE)
set (ENABLE_XATTR   OFF CACHE BOOL "" FORCE)
set (ENABLE_ICONV   OFF CACHE BOOL "" FORCE)
set (ENABLE_OPENSSL ON  CACHE BOOL "" FORCE)

set (LIBARCHIVE_INSTALL OFF CACHE BOOL "" FORCE)

# Use system libraries on Linux, MSYS2/pkg-config on Windows
if(WIN32 AND NOT MSYS)
    # Traditional Windows build with vcpkg or manual paths
    find_package(ZLIB REQUIRED)
    find_package(LibLZMA REQUIRED)
    
    # Set OpenSSL to optional since it's disabled
    if(ENABLE_OPENSSL)
        find_package(OpenSSL REQUIRED)
    endif()
    
    # If find_package fails, you can uncomment and modify these paths:
    # set(ZLIB_ROOT        "C:/vcpkg/installed/x64-windows")
    # set(LIBLZMA_ROOT_DIR "C:/vcpkg/installed/x64-windows")
    # set(OPENSSL_ROOT_DIR "C:/vcpkg/installed/x64-windows")
else()
    # Use pkg-config for Linux and MSYS2/MinGW environments
    find_package(PkgConfig REQUIRED)
    
    pkg_check_modules(ZLIB REQUIRED zlib)
    pkg_check_modules(LIBLZMA REQUIRED liblzma)
    if(ENABLE_OPENSSL)
        pkg_check_modules(OPENSSL REQUIRED openssl)
    endif()
    
    # For MSYS2, ensure we use the correct library paths
    if(MSYS)
        # Add MSYS2 library directories
        link_directories(${ZLIB_LIBRARY_DIRS} ${LIBLZMA_LIBRARY_DIRS})
        if(ENABLE_OPENSSL)
            link_directories(${OPENSSL_LIBRARY_DIRS})
        endif()
    endif()
endif()

option (LOG_ENABLED "Enables logging." ON)
option (LOG_ATTACH_CONSOLE "Enables console logging." ON)

add_subdirectory(external/libarchive)

add_subdirectory(common)
add_subdirectory(dropper)
add_subdirectory(recorder)