#!/bin/python3

import numpy as np
import h5py as h5
import sys, os
import time

print("making files")

matfile = r"D:\B5D_tests\input\20211018_hUC-6fEF05_fish-2-1_bar-20p.mat"
b5d_outfile = r"D:\B5D_tests\lab_meeting\20211018_hUC-6fEF05_fish-2-1_bar-20p_b5d_q3_m1.h5"
b5d_3d_outfile = r"D:\B5D_tests\lab_meeting\20211018_hUC-6fEF05_fish-2-1_bar-20p_b5d-3d_q3_m1.h5"
b3d_outfile = r"D:\B5D_tests\lab_meeting\20211018_hUC-6fEF05_fish-2-1_bar-20p_b3d_q3_m1.h5"
h5_out = r"D:\B5D_tests\lab_meeting\20211018_hUC-6fEF05_fish-2-1_bar-20p_zlib.h5"

compression_opts=(
		round(3*1000),
		1,
		round(2.1845*1000),
		399,
		round(1.6*1000))

m = h5.File(matfile,'r')
m_dset = m['imagedata']

xx,yy,zz,cc,tt = m_dset.chunks

# b5d = h5.File(b5d_outfile,'w')

# start=time.time()
# b5d_dset = b5d.create_dataset('imagedata',
# 	data=np.asarray(m_dset[:,:,:,:,1:101],dtype='uint16'),
# 	chunks=(181*2,181,1,1,1), #(xx,yy,zz,cc,tt),
# 	compression=32666,
# 	compression_opts=compression_opts
# 	)
# end = time.time()
# b5d.flush()
# b5d.close()

# print("time taken to compress b5d:\n")
# print((end - start))

# zlib = h5.File(h5_out,'w')
# start = time.time()
# zlib_dset = zlib.create_dataset('imagedata',
# 	data=np.asarray(m_dset[:,:,:,:,1:101],dtype='uint16'),
# 	chunks=(181*2,181,1,1,1), #(xx,yy,zz,cc,tt),
# 	compression=2)
# end = time.time()
# zlib.flush()
# zlib.close()

# print("time taken to compress zlib:\n")
# print((end - start))

# b3d = h5.File(b3d_outfile,'w')
# b3d_data = m_dset[:,:,:,:,1:101]
# x,y,z,c,t = b3d_data.shape
# b3d_data = b3d_data.reshape(x,y,z*c*t)
# start = time.time()
# b3d_dset = b3d.create_dataset('imagedata',
# 	data=np.asarray(b3d_data,dtype='uint16'),
# 	chunks=(181*2,181,1), #(xx,yy,zz*cc*tt),
# 	compression=32016,
# 	compression_opts=compression_opts
# 	)
# end = time.time()
# b3d.flush()
# b3d.close()

# b5d_3d = h5.File(b5d_3d_outfile,'w')
# b5d_3d_data = m_dset[:,:,:,:,1:101]
# x,y,z,c,t = b5d_3d_data.shape
# b5d_3d_data = b5d_3d_data.reshape(x,y,z*c*t)
# start = time.time()
# b5d_3d_dset = b5d_3d.create_dataset('imagedata',
# 	data=np.asarray(b5d_3d_data,dtype='uint16'),
# 	chunks=(181*2,181,1), #(xx,yy,zz*cc*tt),
# 	compression=32666,
# 	compression_opts=compression_opts
# 	)
# end = time.time()
# b5d_3d.flush()
# b5d_3d.close()
# sys.exit("done writing for now")

# print("time taken to compress b3d:\n")
# print((end - start))

# print("ending")


############
# Analysis #
############

# for _ in range(4):
# 	start = time.time()
# 	count = 0
# 	with h5.File(b5d_outfile,'r') as f:
# 		dset = f['imagedata']
# 		print(dset.chunks)
# 		for i in range(dset.shape[4]):
# 			count = dset[:,:,:,:,i]
# 		print(count/i)
# 		f.flush()
# 	end = time.time()
# 	print("b5d read whole volumes time:")
# 	print(end - start)


start = time.time()
count = 0
with h5.File(b3d_outfile,'r') as f:
	dset = f['imagedata']
	print(dset.chunks)
	for i in range(dset.shape[2]):
		count = dset[:,:,i]
	print(count/i)
	f.flush()
end = time.time()
print("b3d read whole volumes time:")
print(end - start)

start = time.time()
count = 0
with h5.File(b5d_3d_outfile,'r') as f:
	dset = f['imagedata']
	print(dset.chunks)
	for i in range(dset.shape[2]):
		count = dset[:,:,i]
	print(count/i)
	f.flush()
end = time.time()
print("b5d-3d read whole volumes time:")
print(end - start)

# for _ in range(4):
# 	start = time.time()
# 	count = 0
# 	with h5.File(h5_out,'r') as f:
# 		dset = f['imagedata']
# 		print(dset.chunks)
# 		for i in range(dset.shape[4]):
# 			count = dset[:,:,:,0,i]
# 		print(count/i)
# 		f.flush()
# 	end = time.time()
# 	print("libz read whole volumes time:")
# 	print(end - start)

