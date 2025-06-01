# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and language
project(MenuUITest C)

##############################################################################

# MenuUITestSub.exe
add_executable(MenuUITestSub WIN32 MenuUITestSub.c)

# MenuUITest.exe
add_executable(MenuUITest MenuUITest.c)

##############################################################################
