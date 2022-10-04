import numpy as np
import h5py as h5
import sys, os, nrrd
import time
import pathlib as pl

## Files
inputNrrd = pl.Path(r"D:\B5D_tests\lab_meeting\20211018_hUC-6fEF05_fish-2-1_bar_Anat_avg.nrrd")
inputBaseName = inputNrrd.stem
inputNrrdRead = nrrd.read(inputNrrd)
inputMatrix = inputNrrdRead[0]

for i in (0,1,3,5):
	with h5.File((inputNrrd.parent / (inputNrrd.stem + "_q-" + str(i) + ".h5")),'w') as outfile:
		compression_opts=(
			round(i*1000),
			1,
			round(2.1845*1000),
			399,
			round(1.6*1000))
		dset = outfile.create_dataset('imagedata',
			data=np.asarray(inputMatrix,dtype='uint16'),
			chunks=(181*2,181,1),
			compression=32666,
			compression_opts=compression_opts
		)

