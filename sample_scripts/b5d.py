#!/bin/python3

import os
import h5py as h5
import pathlib as pl
import sys
import numpy as np
from enum import Enum

class FilterType(Enum):
	b3d = 32016
	b5d = 32666
	zlib = 2


class CompressionAttributes:
	"""A data structure defining the B5D filter attributes"""

	def __init__(self,
		CHUNKS=None,
		quantization_step=1, 
		compression_mode=2,  
		CONVERSION=2.1845, 
		BACKGROUND_LEVEL=399, 
		READ_NOISE=1.6, 
		tile_size=24):

		self.CHUNKS = CHUNKS # set chunk size to the same as input
		self.quantization_step = quantization_step
		self.compression_mode = compression_mode
		self.CONVERSION = CONVERSION  # in DN/e-, here 65535/30000
		self.BACKGROUND_LEVEL = BACKGROUND_LEVEL # set it to camera average background level
		self.READ_NOISE = READ_NOISE # in e-
		self.tile_size = tile_size


class FileHandler:
	"""Defines file handling operations"""

	def __init__(self, inDir, inFile, 
		dataset_name="imagedata", 
		outName=None, outDir=None):

		self.inDir = pl.Path(inDir)
		self.inFile = inFile
		self.outName = outName

		if (outDir is None):
			self.outDir = pl.Path(inDir)
		else:
			self.outDir = pl.Path(outDir)

		if (outName is None):
			self.outName = "test"
		else:
			self.outName = outName

		self.dataset_name = dataset_name

	# @property
	def fullInFile(self):
		return self.inDir / self.inFile

	def fullOutFile_b5d_5d(self):
		return self.outDir / (self.outName + "_b5d_5d.h5")

	def fullOutFile_b5d_3d(self):
		return self.outDir / (self.outName + "_b5d_3d.h5")

	def fullOutFile_b3d(self):
		return self.outDir / (self.outName + "_b3d.h5")

	def fullOutFile_noFilter(self):
		return self.outDir / (self.outName + "_noFilter.h5")


class B5D_Compression_Test:
	"""Save as hdf5 file using various filters"""

	def __init__(self, compression_attributes,
		fileHandler):

		self.files = fileHandler
		self.attrs = compression_attributes
		self.original_file = None
		self.workingDSet = None

		
	# open original dataset
	# TODO: handle errors!
	def setUpInputFile(self, dset=None):
		"""Open input file and save a pointer to it"""
		if (dset is None):
			dset = self.files.dataset_name

		self.original_file = h5.File(self.files.fullInFile(),'r')
		self.workingDSet = self.original_file[dset]
		# original_file.close()

	# get subset of data if desired
	def subsetDataByTrimingTimeDim(self,
		trimRange=slice(0,30)):
		if (self.workingDSet is None):
			sys.exit("Working data set is not set")
		else:
			self.workingDSet = self.workingDSet[:,:,:,:,trimRange]
		# pass

	# save dataset as hdf5 with custom filter
	def saveAsHDF5WithFilter(self, # outFile,
		filter=FilterType.b5d):

		if (filter is FilterType.b3d):
			x,y,z,c,t = self.workingDSet.shape
			localData=self.workingDSet.reshape(x,y,z*c*t)
			outFile = self.files.fullOutFile_b3d()
			# some magic numbers here, but easier than writing checks against None
			x_chunk,y_chunk,z_chunk,c_chunk,t_chunk=self.attrs.CHUNKS
			CHUNKS = (x_chunk,y_chunk,z_chunk * c_chunk * t_chunk)
		else:
			localData=self.workingDSet
			outFile = self.files.fullOutFile_b5d_5d()
			CHUNKS=self.attrs.CHUNKS

		f = h5.File(outFile,'w')

		if (filter is FilterType.zlib):
			dset = f.create_dataset(self.files.dataset_name,
				data=np.asarray(localData,dtype='uint16'),
				chunks=CHUNKS,compression=filter.value)
		else:
			dset = f.create_dataset(self.files.dataset_name,
				data=np.asarray(localData,dtype='uint16'),
				chunks=CHUNKS,
				compression=filter.value,
				compression_opts=(
					round(self.attrs.quantization_step*1000), 
					self.attrs.compression_mode, 
					round(self.attrs.CONVERSION*1000), 
					self.attrs.BACKGROUND_LEVEL, 
					round(self.attrs.READ_NOISE*1000),
					self.attrs.tile_size
					)
			)

		f.close()

	def saveAsHDF5WithoutFilter(self,compression=2):

		localData=self.workingDSet
		outFile = self.files.fullOutFile_noFilter()
		CHUNKS = None
		f = h5.File(outFile,'w')
		dset = f.create_dataset(self.files.dataset_name,
			data=np.asarray(localData,dtype='uint16'),
			chunks=CHUNKS,compression=compression)
		f.close()


	def convertB5Dto3D(self, inFile=None, dataset_name=None): #, 
		# , outFile):
		if (inFile is None):
			inFile = self.files.fullOutFile_b5d_5d()
		if (dataset_name is None):
			dataset_name = self.files.dataset_name

		if os.path.isfile(inFile):
			f = h5.File(inFile,'r')
		else:
			sys.exit("There is not an existing B5D 5D dataset")

		x,y,z,c,t = f[dataset_name].shape
		localData = f[dataset_name]
		localData = localData[:,:,:,:,:].reshape(x,y,z*c*t)
		out = h5.File(self.files.fullOutFile_b5d_3d(),'w')
		dset = out.create_dataset(dataset_name,
			data=np.asarray(localData,dtype='uint16'),
			chunks=(181,181,1)
			)
		f.close()
		out.close()

	def tearDownInputFile(self):
		self.original_file.close()

class B5D_Read_Test:
	"""Defines file handling operations"""

	def __init__(self, inFile):
		self.inputFile = pl.Path(inFile)

		self.imageFile = None
		self.dset = None

	def load_file(self):
		print("loading file")
		self.imageFile = h5.File(self.inputFile, 'r')
		self.dset = self.imageFile['imagedata']
		print(self.dset.chunks)

	def process_file(self, z_range=slice(0,10)):
		return np.mean(self.dset[:,:,0,0,z_range])

	def unload_file(self):
		print("Closing file in python")
		self.imageFile.close()