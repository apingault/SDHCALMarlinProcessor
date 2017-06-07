########################################################
# cmake file for building SDHCALMARLINProcessor
# @author Arnaud Steen
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################


# project name
PROJECT( sdhcalMarlin )

#run cmake in build directory with : 
# cmake -C ${ILCSOFT}/ILCSoft.cmake -DCaloSoftWare_DIR=${CaloSoftWare} ../

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )



### DEPENDENCIES ############################################################

FIND_PACKAGE( ILCUTIL REQUIRED COMPONENTS ILCSOFT_CMAKE_MODULES )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )


FIND_PACKAGE( Marlin 1.0 REQUIRED ) # minimum required Marlin version
FIND_PACKAGE( ROOT REQUIRED ) # minimum required Marlin version
FIND_PACKAGE( CLHEP REQUIRED )
FIND_PACKAGE( LCIO REQUIRED )

FOREACH( pkg LCIO ROOT Marlin CLHEP)
    IF( ${pkg}_FOUND )
        INCLUDE_DIRECTORIES( ${${pkg}_INCLUDE_DIRS} )
        LINK_LIBRARIES( ${${pkg}_LIBRARIES} )
        ADD_DEFINITIONS ( ${${pkg}_DEFINITIONS} )

    ENDIF()
ENDFOREACH()

FIND_PACKAGE( CaloSoftWare REQUIRED )
FOREACH( pkg CaloSoftWare )
    INCLUDE_DIRECTORIES( ${${pkg}_INCLUDE_DIRS} )
    LINK_LIBRARIES( ${${pkg}_LIBRARIES} )
    ADD_DEFINITIONS ( ${${pkg}_DEFINITIONS} )
ENDFOREACH()

### DOCUMENTATION ###########################################################

OPTION( INSTALL_DOC "Set to OFF to skip build/install Documentation" ON )
IF( INSTALL_DOC AND EXISTS "${PROJECT_SOURCE_DIR}/doc/CMakeLists.txt" )
    ADD_SUBDIRECTORY( ./doc )
ENDIF()



### LIBRARY #################################################################
# include directories
INCLUDE_DIRECTORIES( ./include )
#INSTALL_DIRECTORY( ./include DESTINATION . FILES_MATCHING PATTERN "*.h" )

FILE(GLOB_RECURSE toto "include/*.h")
add_custom_target(headers SOURCES ${toto})

# add library
AUX_SOURCE_DIRECTORY( ./src library_sources )
ADD_SHARED_LIBRARY( ${PROJECT_NAME} ${library_sources} )
INSTALL_SHARED_LIBRARY( ${PROJECT_NAME} DESTINATION lib )

# display some variables and write them to cache
DISPLAY_STD_VARIABLES()

