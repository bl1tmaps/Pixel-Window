cmake_minimum_required(VERSION 3.16)
project(PixelWindowExample LANGUAGES C CXX)

# Assuming your source file is named main.cpp
set(SOURCES main.cpp)

if(APPLE)
    # Enable Objective-C++ support for Metal/Cocoa
    enable_language(OBJCXX)
    
    # Force CMake to treat main.c as an Objective-C++ file
    # (Skip this line and change SOURCES above if you already renamed it to main.mm)
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE OBJCXX)
endif()

# Create the executable target
add_executable(PixelWindowApp ${SOURCES})

# -----------------------------------------------------------------------------
# OS-Specific Build Rules
# -----------------------------------------------------------------------------

if(WIN32)
    # Windows: Link against User32 (Window management) and GDI32 (Drawing)
    target_link_libraries(PixelWindowApp PRIVATE user32 gdi32)

elseif(APPLE)
    # macOS: Enable Automatic Reference Counting (ARC)
    target_compile_options(PixelWindowApp PRIVATE -fobjc-arc)
    
    # Find required Apple frameworks
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    
    # Link the frameworks
    target_link_libraries(PixelWindowApp PRIVATE 
        ${COCOA_FRAMEWORK} 
        ${METAL_FRAMEWORK} 
        ${QUARTZCORE_FRAMEWORK}
    )

elseif(UNIX)
    # Linux: Find and link X11
    find_package(X11 REQUIRED)
    
    # Use the imported target if available (modern CMake), otherwise fallback to variables
    if(TARGET X11::X11)
        target_link_libraries(PixelWindowApp PRIVATE X11::X11)
    else()
        target_include_directories(PixelWindowApp PRIVATE ${X11_INCLUDE_DIR})
        target_link_libraries(PixelWindowApp PRIVATE ${X11_LIBRARIES})
    endif()
endif()
