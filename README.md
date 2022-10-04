# Orger lab B5D Implemenation

A reimplementation of B3D, but extending it to work with 5D datasets.

Corresponding publication from Bálint Balázs: http://www.biorxiv.org/content/early/2017/07/21/164624

## Setup

To use:

 - Clone the Weightloss branch to your local computer.
 - Download and install [this version](https://support.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.8/bin/windows/hdf5-1.10.8-Std-win10_64-vs16.zip) of the HDF5 Library. 
 - Run vcredist.exe from the vcredist folder in the B5D repo. 
 - Set the Windows Environment Variable HDF5_PLUGIN_PATH to point to where you want the .dll files in the x64/Release folder to live. Leaving them in place is fine.
 - The filter code is 306. You will need to supply this to python/matlab/etc only when writing B5D compressed files
 - See the original repo [here](https://git.embl.de/balazs/B3D) for more detailed instructions.
 

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
