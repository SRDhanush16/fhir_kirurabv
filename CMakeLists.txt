cmake_minimum_required(VERSION 3.0)
project(FHIR)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories for the source files
file(GLOB_RECURSE sources Src/*.cpp Src/*.h)

# Add any additional data or resources (you can adjust this part if necessary)
set(data)  # (Add any data files here if needed)

###############################################################################
## Find OpenSSL and Link Libraries ##########################################
###############################################################################

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Include the OpenSSL headers
include_directories(${OPENSSL_INCLUDE_DIR})

# Link OpenSSL libraries to the executable
add_executable(FHIR ${sources} ${data})
target_link_libraries(FHIR PRIVATE OpenSSL::SSL OpenSSL::Crypto)

###############################################################################
## Compiler Flags ############################################################
###############################################################################

# Set compiler flags for the target
target_compile_options(FHIR PUBLIC -std=c++1y -Wall -Wfloat-conversion)

# Include source files
target_include_directories(FHIR PUBLIC Src)
