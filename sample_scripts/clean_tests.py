#!/bin/python3

import os
import h5py as h5
import pathlib as pl
import sys
import numpy as np
from enum import Enum

import b5d_compression_evaluation_tests

if __name__=="__main__":
	print("success!")

	inputDir = pl.Path(r"D:\B5D_tests\input")
	print(inputDir)

	inputFiles = os.listdir(inputDir)

	print("Start object oriented file handling")
	file_suffix = "_oop_test"
	outDir = pl.Path(r"D:\B5D_tests")

	for f in inputFiles:
		matfile=inputDir / f
		rootFileName = f[0:-4]
		print(matfile)
		attrs = CompressionAttributes(
			CHUNKS=(x,y,z,c,t),
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

		sys.exit("first test finished")

