cmake_minimum_required(VERSION 3.16)
project(image2pixel CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    # Enable Windows subsystem (no console window)
    set(CMAKE_WIN32_EXECUTABLE ON)
    message(STATUS "Configuring for Windows...")
endif()

# Try to find Qt6, fallback to Qt5 if not found
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if (NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, trying Qt5...")
    find_package(Qt5 COMPONENTS Core Widgets QUIET)
    
    if (NOT Qt5_FOUND)
        message(FATAL_ERROR "
        Could NOT find Qt6 or Qt5.
        
        Please specify the path to your Qt installation by setting the 'CMAKE_PREFIX_PATH' variable.
        Example (CMake argument): -DCMAKE_PREFIX_PATH=\"C:/Qt/6.6.0/mingw_64\"
        
        If you haven't installed Qt, please download it from https://www.qt.io/download
        or install it via a package manager (e.g., MSYS2, vcpkg).
        ")
    endif()
endif()

# Source files
set(SOURCES main.cpp)

# Define executable
add_executable(image2pixel ${SOURCES})

# Link libraries
if (Qt6_FOUND)
    target_link_libraries(image2pixel PRIVATE Qt6::Widgets)
    message(STATUS "Linking against Qt6.")
else()
    target_link_libraries(image2pixel PRIVATE Qt5::Widgets)
    message(STATUS "Linking against Qt5.")
endif()

if(WIN32)
    set(WINDEPLOYQT_EXECUTABLE "")
    
    # Find windeployqt
    if(Qt6_FOUND)
        # Get the bin directory relative to where Qt6Config.cmake was found
        get_filename_component(_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    elseif(Qt5_FOUND)
        get_filename_component(_qt_bin_dir "${Qt5_DIR}/../../../bin" ABSOLUTE)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        add_custom_command(TARGET image2pixel POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-system-d3d-compiler \"$<TARGET_FILE:image2pixel>\"
            COMMENT "Running windeployqt..."
        )
    else()
        message(WARNING "windeployqt not found. You may need to add Qt bin directory to PATH to run the executable.")
    endif()
endif()