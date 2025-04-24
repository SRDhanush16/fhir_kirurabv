cmake_minimum_required(VERSION 3.0)
project(FHIR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE sources      Src/*.cpp Src/*.h)

###############################################################################
## target definitions #########################################################
###############################################################################

# add the data to the target, so it becomes visible in some IDE
add_executable(FHIR ${sources} ${data})

# just for example add some compiler flags
target_compile_options(FHIR PUBLIC -std=c++1y -Wall -Wfloat-conversion)

# this lets me include files relative to the root source directory with a <> pair
target_include_directories(FHIR PUBLIC Src)


