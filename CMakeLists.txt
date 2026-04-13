cmake_minimum_required(VERSION 3.16)

project(kateollama VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(KF5_DEP_VERSION "5.68.0")

find_package(ECM ${KF5_DEP_VERSION} QUIET REQUIRED NO_MODULE)

list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(FeatureSummary)

set(QT_MIN_VERSION "5.15.0")
find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets Network)
find_package(KF5 ${KF5_DEP_VERSION}
    REQUIRED COMPONENTS
        CoreAddons # Core addons on top of QtCore
        I18n # For localization
        TextEditor # The editor component
)

set(PROJECT_SOURCES
    src/plugin.cpp
    src/plugin.h
    src/settings.h
    src/settings.cpp
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)

kcoreaddons_add_plugin(kateollama INSTALL_NAMESPACE "ktexteditor")
target_compile_definitions(kateollama PRIVATE TRANSLATION_DOMAIN="kateollama")
target_link_libraries(kateollama PRIVATE KF5::I18n KF5::TextEditor Qt5::Network)
target_sources(kateollama PRIVATE ${PROJECT_SOURCES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
