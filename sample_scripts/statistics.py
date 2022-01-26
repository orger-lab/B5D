#!/bin/python3

import os
import h5py as h5
import pathlib as pl
import sys
import numpy as np
import time

file_order = []
time_to_load = []

inputDir = pl.Path("D:/B5D_tests/chunk_tests_1.10.8")
files = os.listdir(inputDir)
for f in files:
	print(f)
	file_order.append(f)
	start = time.time()
	loop_file = h5.File((inputDir / f),'r')
	dset = loop_file['imagedata']
	retreived_data_xy = dset[:,:,0,0,1:100]
	end = time.time()
	difference = end - start
	print(difference)
	time_to_load.append(difference)
	loop_file.close()


