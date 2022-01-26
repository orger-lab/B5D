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
			CHUNKS=(181,181,1)
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

class chunkingLoop:
	"""A class to loop through chunking parameters, because I'm tired of 
	writing the same thing a million times"""

	def __init__(self, inDir, matfile, file_suffix, outDir):
		self.attrs = CompressionAttributes(
			CHUNKS=(x,y,z,c,t),
			quantization_step=ql,
			compression_mode=mode
			)
		self.files = FileHandler(inDir=inDir,
			inFile=matfile,
			dataset_name="imagedata", 
			outName=matfile[0:-4] + file_suffix, 
			outDir=(outDir / "chunk_tests")
			)



if __name__=="__main__":


	# rootDir = pl.Path("F:/GCaMP_Comparisons/6fEF05/")
	# outDir = pl.Path("D:/B5D_tests")
	# x = os.listdir(rootDir)
	# animalDirs = [z for z in x if z.endswith("20p")]
	# animalDir = animalDirs[0]

	# z=1
	# for x in (1,2): # x = 4 didn't work
	# # for z in (1,10,20): # z > 1 didn't work
	# 	for t in (1,5,10):
	# 		file_suffix = "_xy-" + str(181*x) + "_z-" + str(z) + "_t-" + str(t)
	# 		attrs = CompressionAttributes(CHUNKS=((x * 181),(x * 181),z,1,t))
	# 		files = FileHandler(inDir=(rootDir / animalDir),
	# 			inFile=matfile,
	# 			dataset_name="imagedata", 
	# 			outName=rootFileName + file_suffix, 
	# 			outDir=(outDir / "chunk_tests"))
	# 		if os.path.isfile(files.fullOutFile_b5d_5d()):
	# 			print("skipping")
	# 		else:
	# 			comp = B5D_Compression_Test(attrs,files)
	# 			comp.setUpInputFile()
	# 			comp.subsetDataByTrimingTimeDim()
	# 			comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
	# 			comp.tearDownInputFile()




	#############
	# oop tests #
	#############

	inputDir = pl.Path(r"D:\B5D_tests\input")
	print(inputDir)

	inputFiles = os.listdir(inputDir)

	print("Start object oriented file handling")
	file_suffix = "_oop_test_truncated"
	outDir = pl.Path(r"D:\B5D_tests")

	for f in inputFiles:
		matfile=inputDir / f
		rootFileName = f[0:-4]
		print(matfile)
		attrs = CompressionAttributes(
			CHUNKS=(362,181,1,1,10),
			quantization_step=1,
			compression_mode=1
			)
		files = FileHandler(inDir=inputDir ,
			inFile=matfile,
			dataset_name="imagedata", 
			outName=rootFileName + file_suffix, 
			outDir=(outDir / "oop_tests"))
		comp = B5D_Compression_Test(attrs,files)
		comp.setUpInputFile()
		comp.subsetDataByTrimingTimeDim()
		comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
		comp.tearDownInputFile()
		sys.exit("done!")

	print("oop tests have finished\n\n")

	for f in inputFiles:
		matfile=inputDir / f
		rootFileName = f[0:-4]
		print(matfile)
		print(rootFileName)
		print("START")
		outfile = h5.File((outDir / "oop_tests" / (rootFileName + "_no_oop.h5")),'w')
		infile = h5.File(matfile,'r')
		localdata = infile['imagedata']#[:,:,:,:,slice(0,30)]
		dset = outfile.create_dataset('imagedata',
			data=np.asarray(localdata,dtype='uint16'),
			chunks=(362,181,1,1,10),
			compression=32666,
			compression_opts=(round(1000),1,round(2.1845*1000),399,round(1.6*1000))
			)
		print("END")
		outfile.close()
		infile.close()

	sys.exit("first test finished")

	#### END ####
	# oop tests #
	#############


	rootDir = pl.Path("F:/GCaMP_Comparisons/6fEF05/")
	outDir = pl.Path("D:/B5D_tests")
	x = os.listdir(rootDir)
	animalDirs = [z for z in x if z.endswith("20p")]

	# for animalDir in animalDirs:
	# 	matfiles = os.listdir(rootDir / animalDir)
	# 	matfile = [x for x in matfiles if x.endswith("20p.mat")]
	# 	inputFile = rootDir / animalDir / matfile[0]
	# 	rootFileName = matfile[0][0:-4]
	# 	outFile = outDir / rootFileName
	# 	print(inputFile)

	# 	files = FileHandler(inDir=(rootDir / animalDir),inFile=matfile[0],
	# 		dataset_name="imagedata", outName=rootFileName, 
	# 		outDir=(outDir / "multi-file_truncated_to_30_volumes"))

	# 	# set up compression attributes
	# 	attrs = CompressionAttributes(CHUNKS=(181,181,1,1,1))

	# 	# Initialize B5D_Compression_Test
	# 	comp = B5D_Compression_Test(attrs,files)
	# 	comp.setUpInputFile()
	# 	comp.subsetDataByTrimingTimeDim()
	# 	comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
	# 	comp.saveAsHDF5WithFilter(filter=FilterType.b3d)
	# 	comp.convertB5Dto3D()
	# 	comp.tearDownInputFile()

	#################
	# test chunking #
	#################
	rootDir = pl.Path("F:/GCaMP_Comparisons/6fEF05/")
	animalDir = animalDirs[0]
	matfile = "20200909-hUC-6fEF05-bars-fish-1-20p.mat"
	rootFileName = "20200909-hUC-6fEF05-bars-fish-1-20p"
	z = 1
	x = 181
	y = 181
	c = 1
	t = 1


	#####################
	# Compression Modes #
	#####################

	for mode in range(1,3):
		for ql in range(1,3):
			# mode = 2

			file_suffix = "_mode-" + str(mode) + "_quant_level-" + str(ql)
			attrs = CompressionAttributes(
				CHUNKS=(x,y,z,c,t),
				quantization_step=ql,
				compression_mode=mode
				)
			files = FileHandler(inDir=(rootDir / animalDir),
				inFile=matfile,
				dataset_name="imagedata", 
				outName=rootFileName + file_suffix, 
				outDir=(outDir / "mode_quant-level_tests"))
			comp = B5D_Compression_Test(attrs,files)
			comp.setUpInputFile()
			comp.subsetDataByTrimingTimeDim()
			comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
			comp.tearDownInputFile()
			# sys.exit("exit early")

	sys.exit("finished mode and quant level tests")




	count = 1

	for i in range(10):
		file_suffix = "_CREATE" + str(count)
		attrs = CompressionAttributes(CHUNKS=None)
		files = FileHandler(inDir=(rootDir / animalDir),
			inFile=matfile,
			dataset_name="imagedata", 
			outName=rootFileName + file_suffix, 
			outDir=(outDir / "chunk_tests"))
		comp = B5D_Compression_Test(attrs,files)
		comp.setUpInputFile()
		comp.subsetDataByTrimingTimeDim()
		comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
		# comp.saveAsHDF5WithoutFilter()
		comp.tearDownInputFile()
		count = count + 1

	sys.exit("finished without crashing")


	file_suffix = "_xy-" + str(x) + "_z-" + str(z) + "_t-" + str(t) + "_test"
	attrs = CompressionAttributes(CHUNKS=(x,y,z,1,t))
	files = FileHandler(inDir=(rootDir / animalDir),
		inFile=matfile,
		dataset_name="imagedata", 
		outName=rootFileName + file_suffix, 
		outDir=(outDir / "chunk_tests"))
	comp = B5D_Compression_Test(attrs,files)
	comp.setUpInputFile()
	comp.subsetDataByTrimingTimeDim()
	comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
	comp.tearDownInputFile()

	f = h5.File(files.fullOutFile_b5d_5d(),'r')
	testpoint = f['imagedata'][0,0,0,0,0]
	print(testpoint)
	f.close()
	sys.exit("We survived!")




	# files = FileHandler(inDir="~/My/Random/Path",
		# inFile="myFile.mat")
	# print(files.outDir)
	# print(files.outName)
	# print(files.fullInFile())
	# print(files.dataset_name)
	# print(files.fullOutFile_b5d_3d())

	# atts = CompressionAttributes(CHUNKS=(2,2,1,2,2))
	# print(atts.CHUNKS)

