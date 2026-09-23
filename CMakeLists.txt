cmake_minimum_required(VERSION 3.28...3.31) # 3.28 : minimum exigé par SFML 3.1
project(TetrisC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) # aucun module C++20 : pas d'analyse des sources pour en chercher
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # exécutable PIE : ASLR complet
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compile_commands.json pour clangd et les éditeurs

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Type de build" FORCE)
endif()

option(TETRIS_SFML_STATIQUE "SFML déjà installée : utiliser ses bibliothèques statiques (la SFML téléchargée l'est toujours)" OFF)
option(TETRIS_SANITIZERS "Activer AddressSanitizer et UBSan (GCC/Clang)" OFF)
option(TETRIS_TESTS "Compiler les tests de la logique" ON)
option(TETRIS_NATIF "Optimiser pour le processeur de cette machine (-march=native) : binaire non portable" OFF)
option(TETRIS_DURCISSEMENT "Options de compilation durcies (protection de pile, vérifications de la bibliothèque standard...)" ON)

# SFML 3 : celle du système si elle est assez récente, sinon téléchargée (archive vérifiée) et compilée avec le jeu
set(TETRIS_SFML_COMPOSANTS Graphics Window System)
if(WIN32)
    list(APPEND TETRIS_SFML_COMPOSANTS Main) # WinMain de l'exécutable sans console
endif()
if(TETRIS_SFML_STATIQUE)
    set(SFML_STATIC_LIBRARIES ON)
endif()

include(FetchContent)
FetchContent_Declare(SFML
    URL https://github.com/SFML/SFML/archive/refs/tags/3.1.0.tar.gz
    URL_HASH SHA256=91209a112c2bd0bc6f4ce0d5f3e413cfb48b57c0de59f5507dc81f71b1ad7a5c
    DOWNLOAD_NO_PROGRESS ON
    EXCLUDE_FROM_ALL
    SYSTEM
    FIND_PACKAGE_ARGS 3.1 QUIET COMPONENTS ${TETRIS_SFML_COMPOSANTS}
)
block()
    # Seulement les modules utiles, en bibliothèques statiques : aucune bibliothèque SFML à livrer avec le jeu
    set(BUILD_SHARED_LIBS OFF)
    set(SFML_BUILD_AUDIO OFF)
    set(SFML_BUILD_NETWORK OFF)
    FetchContent_MakeAvailable(SFML)
    if(SFML_FOUND)
        message(STATUS "SFML ${SFML_VERSION} trouvée sur le système")
    else()
        message(STATUS "SFML 3.1.0 téléchargée et compilée avec le jeu")
    endif()
endblock()

include(CheckPIESupported)
check_pie_supported()
include(CheckIPOSupported)
check_ipo_supported(RESULT TETRIS_LTO OUTPUT TETRIS_LTO_ERREUR LANGUAGES CXX)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-march=native TETRIS_MARCH_NATIVE)
check_cxx_compiler_flag(-fstack-clash-protection TETRIS_STACK_CLASH)
check_cxx_compiler_flag(-fcf-protection TETRIS_CF_PROTECTION)

# _FORTIFY_SOURCE=3 (vérifications plus complètes) avec GCC 12+ / Clang 16+
set(TETRIS_FORTIFY 2)
if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12) OR
   (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16))
    set(TETRIS_FORTIFY 3)
endif()

function(tetris_options cible)
    # Optimisation à l'édition de liens : le compilateur voit tout le programme d'un coup
    if(TETRIS_LTO)
        set_property(TARGET ${cible} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET ${cible} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    endif()
    if(TETRIS_NATIF AND TETRIS_MARCH_NATIVE)
        target_compile_options(${cible} PRIVATE -march=native)
    endif()
    if(MSVC)
        # /utf-8 : les textes accentués du code sont en UTF-8
        target_compile_options(${cible} PRIVATE /W4 /utf-8 /guard:cf)
        target_link_options(${cible} PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT)
    else()
        target_compile_options(${cible} PRIVATE -Wall -Wextra -Wpedantic)
        if(TETRIS_DURCISSEMENT)
            target_compile_options(${cible} PRIVATE -fstack-protector-strong)
            if(TETRIS_STACK_CLASH)
                target_compile_options(${cible} PRIVATE -fstack-clash-protection)
            endif()
            if(TETRIS_CF_PROTECTION)
                target_compile_options(${cible} PRIVATE -fcf-protection)
            endif()
            # Vérifie les accès hors limites de std::array / std::vector (coût négligeable ici)
            target_compile_definitions(${cible} PRIVATE _GLIBCXX_ASSERTIONS)
            # _FORTIFY_SOURCE n'a d'effet qu'avec l'optimisation
            target_compile_options(${cible} PRIVATE $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=${TETRIS_FORTIFY}>)
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
    src/Classements.cpp
    src/Fichiers.cpp
    src/Jeu.cpp
    src/MeilleurScore.cpp
    src/Mode.cpp
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
    src/Formes.cpp
    src/main.cpp
    src/Manette.cpp
    src/Menu.cpp
    src/Palette.cpp
    src/Rendu.cpp
    src/Ressources.cpp
    src/Texte.cpp
    src/Touches.cpp
)
target_link_libraries(Tetris PRIVATE tetris_logique SFML::Graphics SFML::Window SFML::System)
if(WIN32)
    target_link_libraries(Tetris PRIVATE SFML::Main)
endif()
tetris_options(Tetris)

# Copie les assets à côté de l'exécutable
add_custom_command(TARGET Tetris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/asset $<TARGET_FILE_DIR:Tetris>/asset
)

if(TETRIS_TESTS)
    enable_testing()
    add_executable(tests_logique tests/test_logique.cpp)
    target_link_libraries(tests_logique PRIVATE tetris_logique)
    tetris_options(tests_logique)
    add_test(NAME logique COMMAND tests_logique)
endif()
