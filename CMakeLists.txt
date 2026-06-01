cmake_minimum_required(VERSION 3.28)

project(Waterfall3D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#use if you need it
#set(CMAKE_PREFIX_PATH "G:/Qt/6.8.3/msvc2022_64" CACHE PATH "Hardcoded Qt path")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)

set(PROJECT_SOURCES
    main.cpp
    SpectrogramWidget.cpp
    SpectrogramWidget.h
    DataWorker.h
    DataWorker.cpp
    HudOverlay.h
    HudOverlay.cpp
 )

if(WIN32)
    list(APPEND PROJECT_SOURCES app_icon.rc)
endif()

qt_add_executable(Waterfall3D
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Ob2>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Ot>
        $<$<CONFIG:Release>:/fp:fast>
    )
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )
endif()

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-compiler-runtime
                    --no-translations
                    --dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>" 
                    "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Executing windeployqt: Packing required DLLs for flawless execution..."
        )
    endif()
endif()
