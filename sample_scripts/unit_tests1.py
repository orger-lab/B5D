#!/bin/python3

from b5d import *

print("hello world")
rootDir = pl.Path(r'D:\B5D_tests')
inputDir = "input"
outputDir = "build_tests"
inFile = "20211018_hUC-6fEF05_fish-2-1_bar-20p.mat"


try:
	os.makedirs(rootDir / outputDir)
except OSError as error: 
	try:
	    os.rmdir(rootDir / outputDir)
	    print("Directory '% s' has been removed successfully" % outputDir)
	    os.makedirs(rootDir / outputDir)
	except OSError as error:
	    print(error)
	    print("Directory '% s' can not be removed" % outputDir)

	

x=181 * 2
y=181 * 2
z=1 
t=1

modes = (1,2,3,101)
quants = (0,3)
# Write files



## lossless (quant = 0)

## lossy (quant = 3)

### Mode 1
for quant in quants:
	for mode in modes:
		suffix = "_quant-" + str(quant) + "_mode-" + str(mode) + "_test"

		attrs = CompressionAttributes(
			CHUNKS=(x,y,z,1,t),
			quantization_step=quant,
			compression_mode=mode
			)

		files = FileHandler(
			inDir=(rootDir / inputDir),
			inFile=inFile,
			dataset_name="imagedata", 
			outName=inFile[0:-4] + suffix, 
			outDir=(rootDir / outputDir)
			)

		# write file
		comp = B5D_Compression_Test(attrs,files)
		comp.setUpInputFile()
		comp.subsetDataByTrimingTimeDim()
		comp.saveAsHDF5WithFilter(filter=FilterType.b5d)
		comp.tearDownInputFile()

		# read file
		file_to_read = B5D_Read_Test((files.outDir / files.fullOutFile_b5d_5d()))
		file_to_read.load_file()
		data = file_to_read.process_file()
		print(data)
		file_to_read.unload_file()
		sys.exit("Exiting")

### Mode 2

### Mode 3

### Mode 101
