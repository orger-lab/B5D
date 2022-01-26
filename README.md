# Orger lab B5D Implemenation

A reimplementation of B3D, but extending it to work with 5D datasets.

Corresponding publication from Bálint Balázs: http://www.biorxiv.org/content/early/2017/07/21/164624

## Project specific things

### B5D_HDF5_plugin
- c/c++
  - general
    - additional include libraries
  - all options
    - additional include directories
      - C:\Program Files\HDF_Group\HDF5\1.10.8\lib
      - C:\Program Files\HDF_Group\HDF5\1.10.8\include
      - C:\Users\Aaron\source\repos\b5d_cuda\b5d_cuda\include\
- linker
  - general
    - additional library directories
  - input
    - addition dependencies
      - szip -> libsz 
      - zlib -> libzlib
      - hdf5.lib
      - hdf5_cpp.lib
      - libhdf5_cpp.lib
      - hdf5_hl.lib
      - hdf5_hl_cpp.lib