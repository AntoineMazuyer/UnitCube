# Defining the project
cmake_minimum_required(VERSION 3.1)

project(UnitCube)
include(cmake/UserConfig.cmake)

# Configuration modes - Debug and Release by default 
set(CMAKE_CONFIGURATION_TYPES 
      Debug Release CACHE CMAKE_CONFIGURATION_TYPES "Configuration types")

# Path to RINGMesh install directory path
set(RINGMesh_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/../RINGMesh/build/Release" CACHE PATH "Path to RINGMesh install directory (mandatory dependency)")

# Turn on the ability to create folders to organize projects and files
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#------------------------------------------------------------------------------------------------
# Additional cmake modules
include(ExternalProject)

#------------------------------------------------------------------------------------------------
# Generate configuration directories for single-configuration generators (Make)
# and run cmake configuration command in each one of them

if(UNIX)    
    # If the CMAKE_BUILD_TYPE is no defined
    # i.e. at first run of that file
    if(CMAKE_BUILD_TYPE STREQUAL "")   
    
      # For all configuration types  
      foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
          # The binary directory for this configuration
          set(project_binary_dir_config  ${PROJECT_BINARY_DIR}/${config})
    
          # Create the directory
          file(MAKE_DIRECTORY ${project_binary_dir_config})
          
          # Launch cmake for this configuration
          # by specifying the CMAKE_BUILD_TYPE and the PROJECT_BINARY_DIR, 
          # and pass on all the options
          execute_process(
             COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}
                -Wno-dev
                -DCMAKE_BUILD_TYPE:STRING=${config}
                -DPROJECT_BINARY_DIR:STRING=${project_binary_dir_config}
                -DRINGMesh_INSTALL_PREFIX:STRING=${RINGMesh_INSTALL_PREFIX}
                WORKING_DIRECTORY ${project_binary_dir_config})                
      endforeach()
      
      # Get out when all configurations have been run
      # We do not want to execute the rest of the file in that case
      return()
    endif()
else(UNIX)
    set(GLOBAL_BINARY_DIR  ${PROJECT_BINARY_DIR}/..)
endif(UNIX)

message(STATUS "UnitCube binary directory is: ${PROJECT_BINARY_DIR}") 
message(STATUS "UnitCube source directory is: ${PROJECT_SOURCE_DIR}") 

#------------------------------------------------------------------------------------------------
# Platform dependent settings

if(WIN32)	
	# Select the CMAKE_CONFIGURATION_TYPES from Visual Studio
    set(COMPILATION_OPTION --config ${CMAKE_CFG_INTDIR})
elseif(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    
    # Determine gcc version and activate additional warnings available in latest versions
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
    
	if(G++_VERSION VERSION_LESS 4.8 )
		message(FATAL_ERROR "UnitCube require G++ version >= 4.8")
	endif()
	
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_compile_options(-Wall -Wextra -Wno-long-long -Wconversion)
	add_compile_options(-Wsign-conversion)
	add_compile_options(-Wdouble-promotion)

    # Enable parallel computation
    set(COMPILATION_OPTION -- -j3)    
else(WIN32)
    # Nothing else really supported
    message(FATAL_ERROR "Your platform is not supported. Please modify the cmakelist file.")    
endif(WIN32)


#------------------------------------------------------------------------------------------------
# RINGMESH 

# RINGMesh is not defined by cmake as an external project (as geogram in RINGMesh) since
# several codes can depend on the same RINGMesh repository [BC].
message(STATUS "Using RINGMesh install directory = ${RINGMesh_INSTALL_PREFIX}/${CMAKE_BUILD_TYPE}")
find_package(RINGMesh REQUIRED PATHS ${RINGMesh_INSTALL_PREFIX})

# We want to be able to include this file
include_directories(${PROJECT_BINARY_DIR})

#------------------------------------------------------------------------------------------------
# Collect the library files

set(unitcube_src "${PROJECT_SOURCE_DIR}/unit_cube.cpp")

source_group("Source Files" FILES ${unitcube_src})

#------------------------------------------------------------------------------------------------
# Build configuration

# Organize outputs in a Linux way
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Libraries to which UnitCube should link
set(RINGMesh_link_libraries RINGMesh::basic 
RINGMesh::geomodel_core 
RINGMesh::geomodel_builder 
RINGMesh::geomodel_tools 
RINGMesh::io 
RINGMesh::geogram_extension)

add_executable(unit_cube ${unitcube_src})
target_link_libraries(unit_cube ${RINGMesh_link_libraries})
