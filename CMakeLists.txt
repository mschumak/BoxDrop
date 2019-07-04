PROJECT( CellularityMeasurement )
CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
# Load the Sedeen dependencies
FIND_PACKAGE( SEDEENSDK REQUIRED 
                HINTS ..\\..\\..)
                
INCLUDE_DIRECTORIES( "${SEDEEN_INCLUDE_DIR}" )

LINK_DIRECTORIES( "${SEDEEN_LIBRARY_DIR}" )

##
# Build the code into a module library
ADD_LIBRARY( CellularityMeasurement MODULE CellularityMeasurement.cpp CellularityMeasurement.h)

# Link the library against the Sedeen libraries
# NOTE: The QT libraries must be linked first.
TARGET_LINK_LIBRARIES( CellularityMeasurement ${SEDEEN_LIBRARIES} )

##
# Install the plugin in the sedeen plugins directory
IF( ${SEDEEN_FOUND} )
INSTALL( TARGETS CellularityMeasurement
	 LIBRARY DESTINATION "${SEDEEN_BINARY_DIR}/plugins" )
ENDIF()