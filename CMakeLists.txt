cmake_minimum_required(VERSION 3.10)
project(Cluster_Prediction)

find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(CMAKE_CXX_COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_COMPILE_FLAGS} -fopenmp -Wall")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS NO)

#Find the PAPI library
FIND_LIBRARY(PAPI_LIB NAMES papi)
FIND_PATH(PAPI_INCLUDE_DIRS NAMES papi.h)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PAPI DEFAULT_MSG PAPI_LIB PAPI_INCLUDE_DIRS)


#READEX RRL is required for to build the cluster prediction library
find_package(RRL CONFIG REQUIRED)
if(RRL_FOUND)
    message(STATUS "READEX RRL library found")
    include_directories(${RRL_INCLUDE_DIRS})
    message (STATUS "TMM_INCLUDE_DIRS = ${RRL_INCLUDE_DIRS}")
    message (STATUS "RRL_LIBRARIES = ${RRL_LIBRARIES}")
else()
    message(WARNING "READEX RRL library not found")
endif()

set(SOURCES  src/predict_cluster.cpp)
set(INCLUDES  include/cluster_predictor.h)
include_directories("./include")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

add_library (predict_cluster SHARED ${SOURCES})
set_target_properties(predict_cluster PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(predict_cluster PUBLIC cxx_std_14)
target_compile_options(predict_cluster PUBLIC -g)

target_link_libraries(predict_cluster PUBLIC pthread ${MPI_LIBRARIES} ${PAPI_LIB}  ${RRL_LIBRARIES})
target_include_directories(predict_cluster PUBLIC ${PAPI_INCLUDE_DIRS} ${RRL_INCLUDE_DIRS})
install(TARGETS predict_cluster DESTINATION lib)
install(FILES ${INCLUDES} DESTINATION include)

