# READEX Runtime Cluster Prediction Library

The READEX Runtime Cluster Prediction Library is written as part of the READEX project
(www.readex.eu)



## Compilation

### Prerequisites

cmake  
C++14 compiler  
MPI  
PAPI library  
READEX Runtime Library (RRL)  
   
   
   
## Building and installation
```
mkdir build && cd build  
cmake .. -DCMAKE_INSTALL_PREFIX=<directory where the dynamic library should be installed> -DCMAKE_PREFIX_PATH=<papi installation root folder>
make   
make install   
```
#### Note:

Make sure to add the ```<install_location/lib>``` to your LD_LIBRARY_PATH.  
   
   
  
## Usage

To use the cluster prediction library, do the following:  
* For C/C++:
 1. Include the header file ```cluster_predictor.h``` in the file that contains the ```SCOREP_USER_OA_PHASE_BEGIN``` macro  
 2. Define the following immediately after the ```SCOREP_USER_OA_PHASE_BEGIN``` macro:  
```SCOREP_USER_PARAMETER_INT64( "Cluster", predict_cluster() )```
 3. Add the linker flags in the Makefile


* For Fortran:  
 1. Add this at the end of the declarations:  
``SCOREP_USER_PARAMETER_DEFINE(cluster)``  
 2. Define the following immediately after the ```SCOREP_USER_OA_PHASE_BEGIN``` macro:  
``SCOREP_USER_PARAMETER_INT64( cluster, "Cluster", predict_cluster() )``  
 3. Add the linker flags in the Makefile

   
   

## If anything fails:

Check whether you have added the library location to the LD_LIBRARY_PATH.
Write a mail to the author.
   
   
    
## Authors
Madhura Kumaraswamy kumarasw(at)in.tum.de
