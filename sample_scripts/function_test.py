#!/bin/python3

from b5d import *

print("hello world")
rootDir = pl.Path(r'D:\B5D_tests')
inputDir = "input"
outputDir = "chunk_read_testing"
inFile = "20211018_hUC-6fEF05_fish-2-1_bar-20p.mat"


try:
	os.makedirs(rootDir / outputDir)
except OSError as error: 
	print("out dir already exists!")

x=181 * 2
y=181 * 2
z=1 
t=1

# for x in (181,181*2,181*3):
print(z)
suffix = "_x-" + str(x) + "y-" + str(y) + "_z-" + str(z) + "_t-" + str(t) + "_test"

attrs = CompressionAttributes(
	CHUNKS=(x,y,z,1,t),
	quantization_step=4,
	compression_mode=1
	)

files = FileHandler(
	inDir=(rootDir / inputDir),
	inFile=inFile,
	dataset_name="imagedata", 
	outName=inFile[0:-4] + suffix, 
	outDir=(rootDir / outputDir)
	)

comp = B5D_Compression_Test(attrs,files)
comp.setUpInputFile()
comp.subsetDataByTrimingTimeDim()
comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
comp.tearDownInputFile()

