cmake_minimum_required(VERSION 3.22)

project(obs-plugin-easy-config VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EASY_CONFIG_BUILD_PLUGIN "Build the OBS plugin target" ON)
option(EASY_CONFIG_BUILD_TESTS "Build unit tests" ON)
set(EASY_CONFIG_INSTALL_LAYOUT "user" CACHE STRING "Plugin install layout: user or program")
set_property(CACHE EASY_CONFIG_INSTALL_LAYOUT PROPERTY STRINGS user program)

add_library(easy_config_core STATIC
  src/path-template.cpp
  src/plugin-config.cpp
)

set_target_properties(easy_config_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

target_include_directories(easy_config_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(easy_config_core PUBLIC cxx_std_17)

if(EASY_CONFIG_BUILD_PLUGIN)
  find_package(Qt6 COMPONENTS Core Widgets QUIET)
  set(OBS_APP_FRAMEWORK_DIR "/Applications/OBS.app/Contents/Frameworks" CACHE PATH "OBS.app Frameworks directory on macOS")
  set(OBS_INSTALL_RPATH_FRAMEWORK_DIR "/Applications/OBS.app/Contents/Frameworks" CACHE PATH "OBS.app Frameworks directory encoded in installed macOS plugin bundles")
  find_path(OBS_FRONTEND_INCLUDE_DIR obs-frontend-api.h
    HINTS
      ${OBS_SOURCE_DIR}/UI/obs-frontend-api
      ${OBS_SOURCE_DIR}/frontend/api
      ${OBS_SOURCE_DIR}/frontend
      ${OBS_SOURCE_DIR}/libobs
      /usr/include
      /usr/local/include
      $ENV{OBS_SOURCE_DIR}/UI/obs-frontend-api
      $ENV{OBS_SOURCE_DIR}/frontend/api
      $ENV{OBS_SOURCE_DIR}/frontend
      $ENV{OBS_SOURCE_DIR}/libobs
    PATH_SUFFIXES obs UI/obs-frontend-api frontend/api frontend libobs
  )
  find_path(OBS_MODULE_INCLUDE_DIR obs-module.h
    HINTS
      ${OBS_SOURCE_DIR}/libobs
      /usr/include
      /usr/local/include
      $ENV{OBS_SOURCE_DIR}/libobs
    PATH_SUFFIXES obs libobs
  )
  find_library(OBS_LIBRARY obs libobs
    HINTS
      ${OBS_LIBRARY_DIR}
      $ENV{OBS_LIBRARY_DIR}
    PATH_SUFFIXES lib lib64 x86_64-linux-gnu
  )
  find_library(OBS_FRONTEND_LIBRARY obs-frontend-api libobs-frontend-api libobs-frontend-api.1
    HINTS
      ${OBS_LIBRARY_DIR}
      $ENV{OBS_LIBRARY_DIR}
      ${OBS_APP_FRAMEWORK_DIR}
    PATH_SUFFIXES lib lib64 x86_64-linux-gnu
  )
  if(NOT OBS_FRONTEND_LIBRARY AND EXISTS "${OBS_APP_FRAMEWORK_DIR}/obs-frontend-api.dylib")
    set(OBS_FRONTEND_LIBRARY "${OBS_APP_FRAMEWORK_DIR}/obs-frontend-api.dylib")
  endif()
  if(NOT OBS_FRONTEND_LIBRARY AND EXISTS "${OBS_APP_FRAMEWORK_DIR}/libobs-frontend-api.1.dylib")
    set(OBS_FRONTEND_LIBRARY "${OBS_APP_FRAMEWORK_DIR}/libobs-frontend-api.1.dylib")
  endif()
  if(NOT OBS_LIBRARY AND EXISTS "${OBS_APP_FRAMEWORK_DIR}/libobs.framework/libobs")
    set(OBS_LIBRARY "${OBS_APP_FRAMEWORK_DIR}/libobs.framework/libobs")
  endif()
  find_path(SIMDE_INCLUDE_DIR simde/x86/sse2.h
    HINTS
      /opt/homebrew/include
      /usr/local/include
  )

  if(Qt6_FOUND AND OBS_FRONTEND_INCLUDE_DIR AND OBS_MODULE_INCLUDE_DIR AND (APPLE OR (OBS_LIBRARY AND OBS_FRONTEND_LIBRARY)))
    set(CMAKE_AUTOMOC ON)

    add_library(obs-plugin-easy-config MODULE
      src/plugin-main.cpp
      src/easy-config-dock.cpp
      src/obs-controller.cpp
    )

    target_include_directories(obs-plugin-easy-config
      PRIVATE
        ${OBS_FRONTEND_INCLUDE_DIR}
        ${OBS_MODULE_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    )

    if(SIMDE_INCLUDE_DIR)
      target_include_directories(obs-plugin-easy-config PRIVATE ${SIMDE_INCLUDE_DIR})
    endif()

    if(APPLE)
      get_target_property(QT_CORE_LOCATION Qt6::Core LOCATION)
      get_target_property(QT_WIDGETS_LOCATION Qt6::Widgets LOCATION)
      get_filename_component(QT_FRAMEWORK_DIR "${QT_CORE_LOCATION}" DIRECTORY)
      get_filename_component(QT_FRAMEWORK_DIR "${QT_FRAMEWORK_DIR}" DIRECTORY)
      get_filename_component(QT_FRAMEWORK_DIR "${QT_FRAMEWORK_DIR}" DIRECTORY)
      get_filename_component(QT_FRAMEWORK_DIR "${QT_FRAMEWORK_DIR}" DIRECTORY)

      target_include_directories(obs-plugin-easy-config
        PRIVATE
          "${QT_FRAMEWORK_DIR}"
          "${QT_FRAMEWORK_DIR}/QtCore.framework/Headers"
          "${QT_FRAMEWORK_DIR}/QtGui.framework/Headers"
          "${QT_FRAMEWORK_DIR}/QtWidgets.framework/Headers"
      )

      target_link_libraries(obs-plugin-easy-config
        PRIVATE
          easy_config_core
          "${QT_CORE_LOCATION}"
          "${QT_WIDGETS_LOCATION}"
      )

      target_link_options(obs-plugin-easy-config PRIVATE
        "-Wl,-undefined,dynamic_lookup"
      )

      if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
        target_compile_options(obs-plugin-easy-config PRIVATE
          "-Xarch_arm64" "-include"
          "-Xarch_arm64" "arm_acle.h"
        )
      elseif(NOT CMAKE_OSX_ARCHITECTURES AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        target_compile_options(obs-plugin-easy-config PRIVATE
          "-include" "arm_acle.h"
        )
      endif()
    else()
      target_link_libraries(obs-plugin-easy-config
        PRIVATE
          easy_config_core
          Qt6::Core
          Qt6::Widgets
          ${OBS_LIBRARY}
          ${OBS_FRONTEND_LIBRARY}
      )
    endif()

    target_compile_definitions(obs-plugin-easy-config
      PRIVATE
        EASY_CONFIG_VERSION="${PROJECT_VERSION}"
        QT_NO_VERSION_TAGGING
    )

    set_target_properties(obs-plugin-easy-config PROPERTIES
      PREFIX ""
      OUTPUT_NAME "obs-plugin-easy-config"
    )

    if(APPLE)
      set_target_properties(obs-plugin-easy-config PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "plugin"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos-plugin-info.plist.in"
        BUILD_RPATH "${OBS_APP_FRAMEWORK_DIR}"
        INSTALL_RPATH "@loader_path/../../../../Frameworks;${OBS_INSTALL_RPATH_FRAMEWORK_DIR}"
      )
    endif()

    add_custom_command(TARGET obs-plugin-easy-config POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/data/locale"
        "$<IF:$<BOOL:${APPLE}>,$<TARGET_BUNDLE_CONTENT_DIR:obs-plugin-easy-config>/Resources/locale,$<TARGET_FILE_DIR:obs-plugin-easy-config>/data/locale>"
      VERBATIM
    )

    if(APPLE)
      add_custom_command(TARGET obs-plugin-easy-config POST_BUILD
        COMMAND install_name_tool
          -change "/opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore"
                  "@rpath/QtCore.framework/Versions/A/QtCore"
          -change "/opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui"
                  "@rpath/QtGui.framework/Versions/A/QtGui"
          -change "/opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets"
                  "@rpath/QtWidgets.framework/Versions/A/QtWidgets"
          "$<TARGET_FILE:obs-plugin-easy-config>"
        COMMAND /bin/sh -c
          "otool -l '$<TARGET_FILE:obs-plugin-easy-config>' | grep -q '/opt/homebrew/opt/qt/lib' && install_name_tool -delete_rpath '/opt/homebrew/opt/qt/lib' '$<TARGET_FILE:obs-plugin-easy-config>' || true"
        COMMAND /bin/sh -c
          "otool -l '$<TARGET_FILE:obs-plugin-easy-config>' | grep -q '${QT_FRAMEWORK_DIR}' && install_name_tool -delete_rpath '${QT_FRAMEWORK_DIR}' '$<TARGET_FILE:obs-plugin-easy-config>' || true"
        VERBATIM
      )

      set(EASY_CONFIG_OBS_USER_PLUGIN_DIR
        "$ENV{HOME}/Library/Application Support/obs-studio/plugins"
        CACHE PATH "OBS user plugin directory for local macOS testing"
      )

      add_custom_target(install-user-plugin
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "${EASY_CONFIG_OBS_USER_PLUGIN_DIR}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf
          "${EASY_CONFIG_OBS_USER_PLUGIN_DIR}/obs-plugin-easy-config.plugin"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "$<TARGET_BUNDLE_DIR:obs-plugin-easy-config>"
          "${EASY_CONFIG_OBS_USER_PLUGIN_DIR}/obs-plugin-easy-config.plugin"
        DEPENDS obs-plugin-easy-config
        COMMENT "Installing obs-plugin-easy-config.plugin to the OBS user plugin directory"
        VERBATIM
      )
    endif()

    if(EASY_CONFIG_INSTALL_LAYOUT STREQUAL "program" AND WIN32)
      set(EASY_CONFIG_PLUGIN_DESTINATION "obs-plugins/64bit")
      set(EASY_CONFIG_LOCALE_DESTINATION "data/obs-plugins/obs-plugin-easy-config/locale")
    else()
      set(EASY_CONFIG_PLUGIN_DESTINATION "obs-plugin-easy-config/bin/64bit")
      set(EASY_CONFIG_LOCALE_DESTINATION "obs-plugin-easy-config/data/locale")
    endif()

    if(APPLE)
      install(TARGETS obs-plugin-easy-config
        BUNDLE DESTINATION "."
      )
    else()
      install(TARGETS obs-plugin-easy-config
        RUNTIME DESTINATION "${EASY_CONFIG_PLUGIN_DESTINATION}"
        LIBRARY DESTINATION "${EASY_CONFIG_PLUGIN_DESTINATION}"
      )

      install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/"
        DESTINATION "${EASY_CONFIG_LOCALE_DESTINATION}"
        FILES_MATCHING PATTERN "*.ini"
      )
    endif()
  else()
    message(WARNING "Skipping OBS plugin target. Install Qt6 and provide OBS_SOURCE_DIR/OBS_LIBRARY_DIR with OBS headers and libraries.")
  endif()
endif()

if(EASY_CONFIG_BUILD_TESTS)
  enable_testing()

  add_executable(easy_config_tests
    tests/path-template-test.cpp
  )

  target_include_directories(easy_config_tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/third_party/doctest
  )

  target_link_libraries(easy_config_tests PRIVATE easy_config_core)

  add_test(NAME easy_config_tests COMMAND easy_config_tests)
endif()
