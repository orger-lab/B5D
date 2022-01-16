#!/bin/python3

# import os
import h5py as h5
import pathlib as pl
import sys
import numpy as np
from enum import Enum

class FilterType(Enum):
	b3d = 32016
	b5d = 32666


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
			self.outName = "test.h5"
		else:
			self.outName = outName

		self.dataset_name = dataset_name

	def fullInFile(self):
		return self.inDir / self.inFile

	def fullOutFile(self):
		return self.outDir / self.outName


class B5D_Compression_Test:
	"""Save as hdf5 file using various filters"""

	def __init__(self, compression_attributes,
		fileHandler):

		self.inFile = fileHandler.fullInFile()
		self.outFile = fileHandler.fullOutFile()
		self.dataset_name = fileHandler.dataset_name
		self.attrs = compression_attributes
		self.workingDSet = None

		
	# open original dataset
	def openInputFile(self, dset="imageData"):
		"""Open input file and save a pointer to it"""
		original_file = h5.File(self.inFile,'r')
		self.workingDSet = original_file[dset]
		original_file.close()

	# get subset of data if desired
	def subsetDataByTrimingTimeDim(self,
		trimRange=slice(0,30)):
		if (self.workingDSet is None):
			sys.exit("Working data set is not set")
		else:
			self.workingDSet = self.workingDSet[:,:,:,:,trimRange]
		# pass

	# save dataset as hdf5 with custom filter
	def saveAsHDF5WithFilter(self, outFile,
		filter=FilterType.b5d):

		if (filter is FilterType.b3d):
			x,y,z,c,t = self.workingDSet.shape
			localData=self.workingDSet.reshape(x,y,z*c*t)
		else:
			localData=self.workingDSet

		f = h5.File(outFile,'w')
		dset = f.create_dataset(self.dataset_name,
			data=np.asarray(localData,dtype='uint16'),
			chunks=self.attrs.CHUNKS,
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

	def convertB5Dto3D(self, inFile, 
		dataset_name, outFile):
		f = h5.File(inFile,'r')
		x,y,z,c,t = f[dataset_name].shape
		localData = f[dataset_name].reshape(x,y,z*c*t)
		out = h5.File(outFile,'w')
		dset = out.create_dataset(self.dataset_name,
			data=np.asarray(localData,dtype='uint16'),
			chunks=self.attrs.CHUNKS
			)
		f.close()
		out.close()

	def closeConnectionToOriginalFile(self):
		pass



if __name__=="__main__":
	files = FileHandler(inDir="~/My/Random/Path",
		inFile="myFile.mat")
	print(files.outDir)
	print(files.outName)
	print(files.fullInFile())
	print(files.dataset_name)

	atts = CompressionAttributes(CHUNKS=(2,2,1,2,2))
	print(atts.CHUNKS)

