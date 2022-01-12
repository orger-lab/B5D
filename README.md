# Orger lab B5D Implemenation

A reimplementation of B3D, but extending it to work with 5D datasets.

Corresponding publication from Bálint Balázs: http://www.biorxiv.org/content/early/2017/07/21/164624

## Open Questions

  - What is tile size?
  - In HDF5_plugin.cpp, can I just combine z and t/c dimensions when calling pShared = new GPUResources() at line 269?
  - Similarly, can the same be done at line 337?