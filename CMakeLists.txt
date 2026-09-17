cmake_minimum_required(VERSION 3.16)
project(TetrisC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # exécutable PIE : ASLR complet

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de build" FORCE)
endif()

option(TETRIS_SFML_STATIQUE "Lier SFML en statique (utile sous Windows)" OFF)
option(TETRIS_SANITIZERS "Activer AddressSanitizer et UBSan (GCC/Clang)" OFF)
option(TETRIS_TESTS "Compiler les tests de la logique" ON)
option(TETRIS_DURCISSEMENT "Options de compilation durcies (protection de pile, vérifications de la bibliothèque standard...)" ON)

if(TETRIS_SFML_STATIQUE)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

include(CheckPIESupported)
check_pie_supported()

function(tetris_options cible)
    if(MSVC)
        target_compile_options(${cible} PRIVATE /W4 /guard:cf)
        target_link_options(${cible} PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT)
    else()
        target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic)
        if(TETRIS_DURCISSEMENT)
            target_compile_options(${cible} PRIVATE -fstack-protector-strong)
            # Vérifie les accès hors limites de std::array / std::vector (coût négligeable ici)
            target_compile_definitions(${cible} PRIVATE _GLIBCXX_ASSERTIONS)
            # _FORTIFY_SOURCE n'a d'effet qu'avec l'optimisation
            target_compile_options(${cible} PRIVATE $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2>)
            if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
                target_link_options(${cible} PRIVATE -Wl,-z,relro,-z,now -Wl,-z,noexecstack)
            endif()
        endif()
        if(TETRIS_SANITIZERS)
            target_compile_options(${cible} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${cible} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# Logique du jeu, sans SFML : testable sans fenêtre
add_library(tetris_logique STATIC
    src/Fichiers.cpp
    src/Jeu.cpp
    src/MeilleurScore.cpp
    src/Piece.cpp
    src/Reglages.cpp
    src/Repetition.cpp
    src/Sac.cpp
)
target_include_directories(tetris_logique PUBLIC src)
tetris_options(tetris_logique)

add_executable(Tetris WIN32
    src/Application.cpp
    src/Effets.cpp
    src/main.cpp
    src/Menu.cpp
    src/Rendu.cpp
    src/Ressources.cpp
    src/Texte.cpp
    src/Touches.cpp
)
target_link_libraries(Tetris PRIVATE tetris_logique sfml-graphics sfml-window sfml-system)
if(WIN32)
    target_link_libraries(Tetris PRIVATE sfml-main)
endif()
tetris_options(Tetris)

# Copie les assets à côté de l'exécutable
add_custom_command(TARGET Tetris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/asset $<TARGET_FILE_DIR:Tetris>/asset
)

if(TETRIS_TESTS)
    enable_testing()
    add_executable(tests_logique tests/test_logique.cpp)
    target_link_libraries(tests_logique PRIVATE tetris_logique)
    tetris_options(tests_logique)
    add_test(NAME logique COMMAND tests_logique)
endif()
