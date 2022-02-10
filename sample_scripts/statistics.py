#!/bin/python3

import os
import h5py as h5
import pathlib as pl
import sys
import numpy as np
import time

file_order = []
time_to_load = []

class FileHandler:
	"""Defines file handling operations"""

	def __init__(self, inFile):
		self.inputFile = pl.Path(inFile)

		self.imageFile = None
		self.dset = None

	def load_file(self):
		print("loading file")
		self.imageFile = h5.File(self.inputFile, 'r')
		self.dset = self.imageFile['imagedata']

	def process_file(self, z_range=slice(0,10)):
		return np.mean(self.dset[:,:,0,0,z_range])

	def unload_file(self):
		print("Closing file in python")
		self.imageFile.close()



if __name__ == '__main__':
	inputDir = pl.Path("D:/B5D_tests/chunk_tests_1.10.8_release")
	files = os.listdir(inputDir)
	for f in files:
		my_file = FileHandler((inputDir / f))
		my_file.load_file()
		data = my_file.process_file()
		print(data)
		my_file = None

	sys.exit("end of test")

	inputDir = pl.Path("D:/B5D_tests/chunk_tests_1.10.8_release")
	files = os.listdir(inputDir)
	for f in files:
		print(f)
		file_order.append(f)
		start = time.time()
		loop_file = h5.File((inputDir / f),'r')
		dset = loop_file['imagedata']
		retreived_data_xy = dset[:,:,0,0,0:10]
		end = time.time()
		difference = end - start
		print(difference)
		time_to_load.append(difference)
		loop_file.close()


