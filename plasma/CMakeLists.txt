cmake_minimum_required(VERSION 3.22)
project(owly-plasma VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick DBus)

# Keep the native adapter inside the package: no global QML import path,
# privileged install or restart of the user's shell is necessary.
set(OWLY_PACKAGE "${CMAKE_CURRENT_BINARY_DIR}/package")
file(COPY package/ DESTINATION "${OWLY_PACKAGE}")
add_library(owlybackend MODULE src/backend.cpp src/backend.h src/plugin.cpp)
target_link_libraries(owlybackend PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::DBus)
target_compile_options(owlybackend PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(owlybackend PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${OWLY_PACKAGE}/contents/ui/backend")

add_custom_target(plasmoid ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/package" "${OWLY_PACKAGE}"
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${CMAKE_CURRENT_BINARY_DIR}/io.github.eirom16.owlykde.plasmoid" --format=zip metadata.json contents
    WORKING_DIRECTORY "${OWLY_PACKAGE}"
    DEPENDS owlybackend)
install(DIRECTORY "${OWLY_PACKAGE}/" DESTINATION share/plasma/plasmoids/io.github.eirom16.owlykde)
install(FILES package/contents/icons/owly-airdrop.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME io.github.eirom16.owlykde.svg)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    find_package(KF6I18n REQUIRED)
    add_executable(backend-test tests/backend_test.cpp src/backend.cpp src/backend.h)
    target_link_libraries(backend-test PRIVATE Qt6::Core Qt6::Gui Qt6::DBus Qt6::Test)
    target_compile_definitions(backend-test PRIVATE FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake-owly.py")
    add_test(NAME backend COMMAND backend-test)
    set_tests_properties(backend PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(owly-preview tests/preview.cpp src/backend.cpp src/backend.h)
    target_link_libraries(owly-preview PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::DBus KF6::I18nQml)
    target_compile_definitions(owly-preview PRIVATE
        FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake-owly.py"
        VIEW_PATH="${OWLY_PACKAGE}/contents/ui/FullRepresentation.qml")
    option(OWLY_TEST_PLASMA "Load the complete plasmoid through libplasma (requires the Plasma development SDK and ECM)" OFF)
    if(OWLY_TEST_PLASMA)
        find_package(Plasma REQUIRED)
        find_package(PlasmaQuick REQUIRED)
        add_executable(plasmoid-test tests/plasmoid_test.cpp)
        target_link_libraries(plasmoid-test PRIVATE Qt6::Gui Qt6::Quick Plasma::Plasma Plasma::PlasmaQuick)
        target_compile_definitions(plasmoid-test PRIVATE PACKAGE_PATH="${OWLY_PACKAGE}"
            FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fake-owly.py")
        add_dependencies(plasmoid-test plasmoid)
        add_test(NAME plasmoid-load COMMAND plasmoid-test)
        set_tests_properties(plasmoid-load PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    endif()
endif()
