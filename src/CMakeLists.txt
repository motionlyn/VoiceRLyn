cmake_minimum_required(VERSION 3.4.1)

add_library( voicerlyn
             SHARED
             effect.cpp )

target_include_directories( voicerlyn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( voicerlyn log )
