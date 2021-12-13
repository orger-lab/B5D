import h5py
import numpy as np
import os

original_file      = h5py.File("G:/B3D/jensdata_time.h5")
compression_mode = 2
CHUNKS = None # set chunk size to the same as input
CONVERSION = 2.1845  # in DN/e-, here 65535/30000
BACKGROUND_LEVEL = 399 # set it to camera average background level
READ_NOISE = 1.6 # in e-

# shape=(700,1024,45,1,2001)

T = 2001;
C = 1;
Z = 45;
X = 1024;
Y = 700;


for quantization_step in range(1,4):
    compressed_file_disk    = "G:/B3D/compressed_time_q{}.h5".format(quantization_step)
    compressed_file_5d_disk = "G:/B3D/compressed_time_5d_q{}.h5".format(quantization_step)


    if os.path.isfile(compressed_file_disk):
        print('Reading compressed h5 file compressed_time_q{}.h5 sorted by datasets'.format(quantization_step))
        compressed_file    = h5py.File("G:/B3D/compressed_time_q{}.h5".format(quantization_step),'r')
    else:
        compressed_file    = h5py.File("G:/B3D/compressed_time_q{}.h5".format(quantization_step))
        print('Creating compressed h5 file compressed_time_q{}.h5 sorted by datasets'.format(quantization_step))
        for original_dataset in original_file.keys():
            
            n = int(original_dataset[1:])
            
            dataset_name = f'{n:04}'
            
            print('\tCompressing oringinal dataset dataset {}'.format(dataset_name))
            
            dset = compressed_file.create_dataset(dataset_name,
                                    data=np.asarray(original_file[original_dataset],dtype='uint16'),
                                    chunks=CHUNKS,
                                    compression=32016,
                                    compression_opts=(round(quantization_step*1000), compression_mode, round(CONVERSION*1000), BACKGROUND_LEVEL, round(READ_NOISE*1000)))
            

        # original_file.close()

        


    if os.path.isfile(compressed_file_5d_disk):
        print("skipping")
    else:
        print('Creating compressed 5D h5 file')
        compressed_file_5d = h5py.File("G:/B3D/compressed_time_5d_q{}.h5".format(quantization_step))
        compressed_data_5d = compressed_file_5d.create_dataset(name='/imagedata', shape=(Y,X,Z,C,T), dtype='uint16', maxshape=(Y,X,Z,C,T))

        for compressed_dataset in compressed_file.keys():
            
            n = int(compressed_dataset)
            
            print('\tUncompressing dataset {}'.format(f'{n:04}'))
                
            compressed_data_5d[:,:,:,0,(n-1)] = np.asarray(compressed_file[compressed_dataset],dtype='uint16')
        
        compressed_file_5d.close()
            
    compressed_file.close()
    

original_file.close()
print('DONE')
