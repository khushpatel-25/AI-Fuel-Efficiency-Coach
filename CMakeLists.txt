cmake_minimum_required(VERSION 3.16)

project(FuelEfficiencyCoach VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    Concurrent)
qt_standard_project_setup(REQUIRES 6.8)

# -------------------------------
# llama.cpp (embedded) dependency
# -------------------------------
# Keep the build small: only build the core "llama" library.
# These options are provided by llama.cpp's top-level CMakeLists. :contentReference[oaicite:1]{index=1}
set(LLAMA_BUILD_COMMON  ON  CACHE BOOL "" FORCE)  # <-- TURN ON
set(LLAMA_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER  OFF CACHE BOOL "" FORCE)

# If you want to ship a single .exe more easily, you can also try static libs:
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/llama.cpp)

# -------------------------------
# Your Qt application
# -------------------------------
qt_add_executable(FuelEfficiencyCoach
    main.cpp
    CsvTripParser.cpp
    TripController.cpp)

qt_add_qml_module(FuelEfficiencyCoach
    URI FuelEfficiencyCoach
    QML_FILES
        Main.qml
        JourneyPage.qml
        PerTripFeedback.qml

        TripHistory.qml
        FuelSavingGoals.qml
    RESOURCES
        JourneyPage.qml
        assets/journey_bg.jpg
        assets/distance.jpg
        assets/fuel.jpg
        assets/speed.jpg
        assets/car.png
        assets/fuelsign.png
        assets/yellowsign.png
        assets/bluesign.png
        assets/redsign.png
        QML_FILES
        QML_FILES AIDownloading.qml
        SOURCES CsvTripParser.h TripController.h
        SOURCES AIController.h AIController.cpp
        RESOURCES
        RESOURCES assets/downloading.png
)

set_target_properties(FuelEfficiencyCoach PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appFuelEfficiencyCoach
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(FuelEfficiencyCoach
    PRIVATE
        Qt6::Quick
        llama
        common
        Qt6::Concurrent
)

# Optional: makes include "llama.h" work even if include dirs aren't propagated as you expect
target_include_directories(FuelEfficiencyCoach PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/llama.cpp/common
)

include(GNUInstallDirs)
install(TARGETS FuelEfficiencyCoach
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/models/granite-4.0-micro-Q4_K_M.gguf
        DESTINATION ${CMAKE_INSTALL_BINDIR}/models)

if(APPLE)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/models/granite-4.0-micro-Q4_K_M.gguf
          DESTINATION FuelEfficiencyCoach.app/Contents/Resources/models)
endif()
