// HDF5_plugin.cpp : Defines the exported functions for the DLL application.
//


#include "HDF5_plugin.h"
//#define H5Z_CUDACOMPRESS_DEBUG
#include <iostream>

//using namespace cudaCompress;

#define DEVICE 0

#ifdef __cplusplus
extern "C" {
#endif

	const H5Z_class2_t H5Z_CUDACOMPRESS[1] = { {
		H5Z_CLASS_T_VERS,       /* H5Z_class_t version */
		(H5Z_filter_t)H5Z_FILTER_B5D,         /* Filter id number             */
		1,              /* encoder_present flag (set to true) */
		1,              /* decoder_present flag (set to true) */
		"HDF5 B5D filter",	/* Filter name for debugging    */
		(H5Z_can_apply_func_t)H5Z_cudaCompress_can_apply,                       /* The "can apply" callback     */
		(H5Z_set_local_func_t)H5Z_cudaCompress_set_local,                       /* The "set local" callback     */
		(H5Z_func_t)H5Z_cudaCompress_filter,         /* The actual filter function   */
	} };

	H5PL_type_t   H5PLget_plugin_type(void) { return H5PL_TYPE_FILTER; }
	const void *H5PLget_plugin_info(void) { return H5Z_CUDACOMPRESS; }


	/* Try to register the filter, passing on the HDF5 return value */
	int register_cudaCompress(void){
#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "registering filter\n");
#endif

		int retval;

		retval = H5Zregister(H5Z_CUDACOMPRESS);
		if (retval < 0){
			//PUSH_ERR("register_cudaCompress", H5E_CANTREGISTER, "Can't register cudaCompress filter");
		}
		return retval;
	}


	/*	Check requirements to run filter
	1.	check for compatible GPU
	2.	check if data type is compatible
	3.	see if data + buffers fit in GPU memory
	*/
	htri_t H5Z_cudaCompress_can_apply(hid_t dcpl, hid_t type, hid_t space) {
		/* check GPU */
#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "filter check started\n");
#endif
		cudaError status;
		int deviceCount;
		status = cudaGetDeviceCount(&deviceCount);
		if (status != cudaSuccess)
			return 0;
		if (deviceCount == 0) {
#ifdef H5Z_CUDACOMPRESS_DEBUG
			fprintf(stderr, "No devices supporting CUDA.\n");
#endif
			return 0;
		}
		cudaDeviceProp deviceProp;
		status = cudaGetDeviceProperties(&deviceProp, DEVICE); // always uses default CUDA device, should be an option to choose device
		if (status != cudaSuccess)
			return -1;
		if (deviceProp.major < 3) {
#ifdef H5Z_CUDACOMPRESS_DEBUG
			fprintf(stderr, "GPU device must support at least compute 3.0 (Kepler).\n");
#endif
			return 0;
		}
		printf("> Using CUDA device [%d]: %s\n", DEVICE, deviceProp.name);
		cudaSetDevice(DEVICE);

		/* check datatype */
		size_t sz = H5Tget_size(type);
		if (sz > 4)			// max 32 bit numbers are supported: 8/16 bit (u)int, float
			return 0;

		H5T_order_t ord = H5Tget_order(type);
		switch (H5Tget_class(type)) {
		case H5T_INTEGER:
			if (ord == H5T_ORDER_BE)
				return 0;
			if (sz > 2)		// max 16 bit intergers are supported
				return 0;
			break;
		case H5T_FLOAT:
			if (sz > 4)		// max 32 bit (single precision) floats are supported
				return 0;
			break;
		default:
			return 0;
		}

		hsize_t chunkdims[32];
		//int r = H5Sget_simple_extent_ndims(123);
		//int ndims = H5Pget_chunk(234, 32, chunkdims);
		//int r = H5Sget_simple_extent_ndims(space);
		int ndims = H5Pget_chunk(dcpl, 32, chunkdims);
		//int ndims = H5Sget_simple_extent_ndims(space);
		//int r = H5Sget_simple_extent_dims(space, chunkdims, nullptr);


		// Aaron Edit: Check that we have between 2 and 5 dimensions
		if (ndims < 2 || ndims > 5)
			return 0;

		
		/* check size */
		//GPUResources::Config config = CompressVolumeResources::getRequiredResources(width, height, depth, 1, 16);


		/* can apply: 1
		   can't apply: 0
		   error: -1
		   */
#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "filter check completed\n");
#endif
		return 1;
	}


	/*  Filter setup.  Records the following inside the DCPL:

	1.  If version information is not present, set slot 0 to the filter version.
	2.	slot 1, 2 and 3 should contain dwtLevels, quantStep and bgLevel. If not supplied,
	defaults to 1, 0 and 0 respectively (lossless compression).

	3.	Read chunk sizes and write to slots 3, 4 and 5 (height, width, depth)
	4.	Read data type and write to slot 6 
	5.  Allocate GPU resources, store pointer in slots 7 and 8

	*/
	herr_t H5Z_cudaCompress_set_local(hid_t dcpl, hid_t type, hid_t space){
		cudaSetDevice(DEVICE);

		int ndims;
		int i;
		herr_t r;

		unsigned int elemSize;
		unsigned int bufsize;
		hsize_t chunkdims[32];  // Aaron edit: hsize_t is just a typedef for int ???

		unsigned int flags;
		size_t nelements = 16;
		unsigned values[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		r = H5Pget_filter_by_id2(dcpl, H5Z_FILTER_B5D, &flags, &nelements, values, 0, NULL, NULL);
		if (r < 0) return -1;

		if (nelements < N_CD_VALUES+6) nelements = N_CD_VALUES+6;  /* First 9 slots reserved.  If any higher
										   slots are used, preserve the contents. */

		
		ndims = H5Pget_chunk(dcpl, 32, chunkdims);
				
		/* It seems the H5Z_FLAG_REVERSE flag doesn't work here, so we have to be
		careful not to clobber any existing version info */
		if (values[0] == 0) values[0] = 0;		// quantStep
		if (values[1] != 2) values[1] = 1;		// mode // Aaron edit: set default to mode 1
		if (values[2] == 0) values[2] = 1000;	// conversion (num/photoelectrons)*1000
		if (values[3] == 0) values[3] = 0;		// background level
		if (values[4] == 0) values[4] = 0;		// read noise (electrons)*1000
		if (values[5] == 0) values[5] = 24;		// tile size
		if (values[6] == 0) values[6] = H5Z_FILTER_B5D_VERSION;
		// change N_CD_VALUES when adding more options

		// only ndims 2 and 3 are allowed, no size constraint in z direction
		// TODO: fix this for 5D
		for (i = ndims - 2; i < ndims; i++){
			if (values[1] > 100)
				chunkdims[i] = getAlignedSize(chunkdims[i], 1 << (values[1]-100));
		}

		r = H5Pset_chunk(dcpl, ndims, chunkdims);
		if (r < 0) return -1;

		if (ndims == 2) {
			values[N_CD_VALUES + 0] = chunkdims[1]; // sizeX
			values[N_CD_VALUES + 1] = chunkdims[0]; // sizeY
			values[N_CD_VALUES + 2] = 1; 			// sizeZ
			values[N_CD_VALUES + 3] = 1; 			// sizeC
			values[N_CD_VALUES + 4] = 1; 			// sizeT

		}
		else if (ndims == 3) {
			values[N_CD_VALUES + 0] = chunkdims[2]; // sizeX
			values[N_CD_VALUES + 1] = chunkdims[1]; // sizeY
			values[N_CD_VALUES + 2] = chunkdims[0]; // sizeZ
			values[N_CD_VALUES + 3] = 1; 			// sizeC
			values[N_CD_VALUES + 4] = 1; 			// sizeT

		}
		else if (ndims == 4) {
			values[N_CD_VALUES + 0] = chunkdims[3]; // sizeX
			values[N_CD_VALUES + 1] = chunkdims[2]; // sizeY
			values[N_CD_VALUES + 2] = chunkdims[1]; // sizeZ
			values[N_CD_VALUES + 3] = chunkdims[0]; // sizeC
			values[N_CD_VALUES + 4] = 1; 			// sizeT
		}
		else if (ndims == 5) {
			values[N_CD_VALUES + 0] = chunkdims[4]; // sizeX
			values[N_CD_VALUES + 1] = chunkdims[3]; // sizeY
			values[N_CD_VALUES + 2] = chunkdims[2]; // sizeZ
			values[N_CD_VALUES + 3] = chunkdims[1]; // sizeC
			values[N_CD_VALUES + 4] = chunkdims[0]; // sizeT
		}



		elemSize = H5Tget_size(type);
		H5T_order_t ord = H5Tget_order(type);
		H5T_sign_t sgn = H5Tget_sign(type);
		H5T_class_t clss = H5Tget_class(type);
		//H5T_class_t clss = H5Tget_class((hid_t)354);

#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stderr, "elemSize %d\n ", elemSize);
		fprintf(stderr, "H5T_order_t %d\n ", ord);
		fprintf(stderr, "H5T_sign_t %d d\n ", sgn);
		fprintf(stderr, "H5T_class_ts %d\n ", clss);
#endif



		if (elemSize == 0) return -1;

		if (clss == H5T_INTEGER) {
			if (sgn == H5T_SGN_NONE) {
				if (elemSize == 1)
					values[N_CD_VALUES + 5] = UINT8_TYPE;
				if (elemSize == 2)
					values[N_CD_VALUES + 5] = UINT16_TYPE;
			}
			else {
				if (elemSize == 1)
					values[N_CD_VALUES + 5] = INT8_TYPE;
				if (elemSize == 2)
					values[N_CD_VALUES + 5] = INT16_TYPE;
			}
		}
		else if (clss == H5T_FLOAT) {
			if (elemSize == 2)
				values[N_CD_VALUES + 5] = FLOAT32_TYPE;
		}


#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stderr, "set local clss %d type %d values %d\n ", clss, type, values[N_CD_VALUES + 5]);
#endif

		int elemCount = 1;
		for (i = 0; i < ndims; i++){
			elemCount *= chunkdims[i];
		}
		bufsize = elemSize * elemCount;
		//values[6] = sizeof(GPUResources*);
		Resources* pShared;
		char buffer[50];
		char* buffer2;

		// check if environment variable B5D_INSTANCE is set
		buffer2 = getenv("B5D_INSTANCE");
		// if set free it up
		if (buffer2 != nullptr) {
			sscanf(buffer2, "%p", &pShared);
			//fprintf(stderr, "B5D instance already exists, getting ready to destroy it\n");
			pShared->destroy();
			delete pShared;
		}
#ifdef H5Z_CUDACOMPRESS_DEBUG
		else {
			fprintf(stderr, "buffer2 is already null\n");
		}
#endif // H5Z_CUDACOMPRESS_DEBUG

		uint newZ = values[N_CD_VALUES + 2] * 
					values[N_CD_VALUES + 3] * 
					values[N_CD_VALUES + 4];

		// initiate GPUResources

		//fprintf(stderr, "Creating new GPUResource...\n");
		pShared = new GPUResources(newZ, // z
			values[N_CD_VALUES + 1], // y
			values[N_CD_VALUES + 0], // x
			DEVICE);

		// set B5D_INSTANCE environment variable to GPUResources pointer
		sprintf(buffer, "B5D_INSTANCE=%p", pShared);
		putenv(buffer);


		#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stderr, "cudaCompress: Computed buffer size %d\n", bufsize);
		#endif
		r = H5Pmodify_filter(dcpl, H5Z_FILTER_B5D, flags, nelements, values);
		if (r < 0) return -1;

		// Aaron caveman debugging:
		// fprintf(stderr, "H5Z_cudaCompress_set_local tileSize: %d\n", (values[5]));
#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stderr, "Compression mode: %d\n", (values[1]));
		fprintf(stdout, "-----------------------\n");
#endif
		return 1;
	}




	/* Filter function itself */
	size_t H5Z_cudaCompress_filter(unsigned int flags, size_t cd_nelmts, const unsigned int cd_values[], size_t nbytes, size_t *buf_size, void **buf)
	{
		cudaSetDevice(DEVICE);
		int outDataLength;

#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "nbytes: %d\n", nbytes);
		fprintf(stdout, "buf_size: %d %d\n", buf_size, buf_size[0]);
		
	/*	fprintf(stdout, "buf: %d %d\n", buf, buf[0, 0]);
		fprintf(stdout, "buf: %d %d\n", &buf, buf[0]);*/

		uint16_t* q = (uint16_t*)buf[0];
		fprintf(stdout, "Q: %u %u %u %u %u %u\n", q[0], q[1], q[2], q[3], q[4], q[5]);
		//std::cout << "q[] " << q[0];

#endif // H5Z_CUDACOMPRESS_DEBUG


		/* Aaron edit: From the H5Z_cudaCompress_set_local function
		if (values[0] == 0) values[0] = 0;		// quantStep
		if (values[1] == 0) values[1] = 1;		// mode
		if (values[2] == 0) values[2] = 1000;	// conversion (num/photoelectrons)*1000
		if (values[3] == 0) values[3] = 0;		// background level
		if (values[4] == 0) values[4] = 0;		// read noise (electrons)*1000
		if (values[5] == 0) values[5] = 24;		// tile size
		if (values[6] == 0) values[6] = H5Z_FILTER_B5D_VERSION;

		N_CD_VALUES + 0 // x
		N_CD_VALUES + 1 // y 
		N_CD_VALUES + 2 // z
		N_CD_VALUES + 3 // c
		N_CD_VALUES + 4 // t
		N_CD_VALUES + 5 // type: int/uint/float/...
		*/

		cudaError status;

		float quantStep = cd_values[0] / 1000.0f; 
		uint dwtLevels = cd_values[1];
		float conversion = cd_values[2] / 1000.0f;
		float bgLevel = cd_values[3];
		float readNoise = cd_values[4] / 1000.0f;
		uint tileSize = cd_values[5];
		uint version = cd_values[6];

		uint sizeX = cd_values[N_CD_VALUES + 0];
		uint sizeY = cd_values[N_CD_VALUES + 1];
		uint sizeZ = cd_values[N_CD_VALUES + 2];
		uint sizeC = cd_values[N_CD_VALUES + 3];
		uint sizeT = cd_values[N_CD_VALUES + 4];

		uint type = cd_values[N_CD_VALUES + 5];

		uint newSizeZ = sizeZ * sizeC * sizeT;

		// Initialize cudaCompress, allocate GPU resources and upload data.
		
		int elemCount = sizeX * sizeY * newSizeZ; 
		long long elemCountCheck = sizeX * sizeY * newSizeZ;
		outDataLength = elemCount * sizeof(short); //(short); // Aaron edit: should this be sizeof(type) ???

#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "size of type: %d\n", sizeof(type));
		fprintf(stdout, "size of short: %d\n", sizeof(short));
		fprintf(stdout, "outDataLength: %d\n", outDataLength);
		fprintf(stdout, "element count: %d\n", elemCount);
		std::cout << "element count check: " << elemCountCheck << "\n";

#endif


		Resources* shared;
		char buffer[50];
		char* buffer2;
		// read environment variable
		buffer2 = getenv("B5D_INSTANCE");
		// if set use it as GPUResources
		if (buffer2 != nullptr) {
			sscanf(buffer2, "%p", &shared);
			shared->destroy();
			delete shared;

		}

		shared = new GPUResources(sizeX, sizeY, newSizeZ, DEVICE);

		sprintf(buffer, "B5D_INSTANCE=%p", shared);
		putenv(buffer);
		 

		short* dpImage = shared->getBuffer<short>(elemCount * 2);
		float* dpBuffer = shared->getBuffer<float>(elemCount);
		float* dpScratch = shared->getBuffer<float>(elemCount);
		cudaCompress::Symbol16* dpSymbols = shared->getBuffer<Symbol16>(elemCount);

		
		// initialize Symbols array to 0
		cudaMemset(dpSymbols, 0, elemCount * sizeof(cudaCompress::Symbol16));

		std::vector<uint> bitStream;

		if (flags & H5Z_FLAG_REVERSE) {

			/** Decompress data.
			**
			**/

			/* copy compressed data from buffer to bitStream vector */
			//bitStream = std::vector<uint>(nbytes / sizeof(uint));
			//memcpy(bitStream.data(), *buf, nbytes);
			bitStream = std::vector<uint>((uint*)*buf, (uint*)*buf + nbytes / sizeof(uint));

			// TODO: fix lossy CPU version
			if (quantStep > 0) {
				// start lossy decompression
				decompressImage(shared->m_pCuCompInstance, bitStream, dpImage, dpBuffer, dpScratch, dpSymbols, 
					sizeX, sizeY, newSizeZ, dwtLevels, quantStep, bgLevel, tileSize, conversion, readNoise);
			}
			else {
				// start lossless decompression
				decompressImageLL(shared->m_pCuCompInstance, bitStream, dpImage, (short*)dpBuffer, (short*)dpScratch, dpSymbols, 
					sizeX, sizeY, newSizeZ, dwtLevels, tileSize);

			}
			/* decompressed image is now in dpImage
			check if buffer is enough to store decompressed image
			if necessary reallocate buffer, and copy decompressed data */
			if (*buf_size < outDataLength) {
				// fprintf(stdout, "buf_size < outDataLength\n");
				//#ifdef H5Z_CUDACOMPRESS_DEBUG
				H5free_memory(*buf);
				*buf = H5allocate_memory(outDataLength, false); // https://portal.hdfgroup.org/display/HDF5/H5_ALLOCATE_MEMORY
				//#else
//				free(*buf);
//				*buf = malloc(outDataLength);
				//#endif
				*buf_size = outDataLength;
			}
			
			// TODO: CPU version for 8 bit
			if (type == INT8_TYPE || type == UINT8_TYPE) {
				cudaCompress::util::u16tou8((uint16_t*)dpImage, (uint8_t*)dpBuffer, elemCount);
				cudaMemcpy(*buf, dpBuffer, elemCount * sizeof(uint8_t), cudaMemcpyDeviceToHost);
			}
			else {
				cudaMemcpy(*buf, dpImage, elemCount * sizeof(uint16_t), cudaMemcpyDeviceToHost);
			}
			shared->releaseBuffers(4);
		}
		else {
#ifdef H5Z_CUDACOMPRESS_DEBUG

			fprintf(stdout, "Enter write\n");
#endif
			/** Compress data.
			**
			**/

			/* copy data from buffer to GPU */
			// TODO: CPU version for 8 bit
			if (type == INT8_TYPE || type == UINT8_TYPE) {
				cudaMemcpy(dpBuffer, *buf, elemCount * sizeof(uint8_t), cudaMemcpyHostToDevice);
				cudaCompress::util::u8tou16((uint8_t*)dpBuffer, (uint16_t*)dpImage, elemCount);
			}
			else {
				cudaMemcpy(dpImage, *buf, elemCount * sizeof(uint16_t), cudaMemcpyHostToDevice);
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "Not 8bit data\n");
#endif
			}

			// TODO: fix lossy CPU version
			if (quantStep > 0) {
				// start lossy compression
				compressImage(shared->m_pCuCompInstance, bitStream, dpImage, dpBuffer, dpScratch, 
					dpSymbols, sizeX, sizeY, newSizeZ, dwtLevels, 
					quantStep, bgLevel, tileSize, conversion, readNoise);
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "Lossy\n");
#endif
			}
			else {
				// start lossless compression
				compressImageLL(shared->m_pCuCompInstance, bitStream, dpImage, (short*)dpBuffer, (short*)dpScratch, 
					dpSymbols, sizeX, sizeY, newSizeZ, 
					dwtLevels, tileSize);
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "Lossless\n");
#endif
			}


#ifdef H5Z_CUDACOMPRESS_DEBUG
			fprintf(stdout, "size after compression\n");
			fprintf(stdout, "buf_size: %d %d\n", buf_size, buf_size[0]);
			fprintf(stdout, "outDataLength: %d\n", outDataLength);

#endif



			/* compressed data is now in bitStream - both GPU and CPU version
			copy it to output buffer */
			outDataLength = bitStream.size() * sizeof(uint);
			if (*buf_size < outDataLength) {
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "freeing memory\n");

#endif
				// Balint had left this as free(*buf) with H5free_memory(*buf) being commented out.
				// Swapping the comment to free(*buf) seems to clear up a bug if the compression fails
				// and the size of the byte stream is bigger than expected
				H5free_memory(*buf);
				//free(*buf);
			//#ifdef H5Z_CUDACOMPRESS_DEBUG
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "pre-buffer allocation\n");
#endif
				*buf = H5allocate_memory(outDataLength, false);
#ifdef H5Z_CUDACOMPRESS_DEBUG
				fprintf(stdout, "buffer allocated\n");
#endif
			//#else
//				*buf = malloc(outDataLength);
			//#endif
				*buf_size = outDataLength;
			}
			memcpy(*buf, bitStream.data(), outDataLength);
#ifdef H5Z_CUDACOMPRESS_DEBUG
			fprintf(stdout, "finished memcopy\n");
#endif
			shared->releaseBuffers(4);
		}


#ifdef H5Z_CUDACOMPRESS_DEBUG
		fprintf(stdout, "buf_size: %d %d\n", buf_size, buf_size[0]);
		fprintf(stdout, "outDataLength: %d\n", outDataLength);
		fprintf(stdout, "------------\n");
#endif


		return outDataLength;

	}



	int initDirectCudaCompress(const size_t* size, GPUResources** res) {
		int elemCount = size[0] * size[1] * size[2];
		//GPUResources::Config config = CompressHeightfieldResources::getRequired3DResources(size[2], size[1], size[0], dwtLevels);
		*res = new GPUResources(size[2], size[1], size[0], DEVICE);
		//(*res)->create(config);
		return 0;
	}

	int closeDirectCudaCompress(GPUResources* res) {
		// Aaron edit!!!
		fprintf(stderr, "Destroy called from within closeDirectCudaCompress");
		res->destroy();
		delete res;
		return 0;
	}

	int directCudaCompress(hid_t dset_id, hsize_t* offset, size_t* size, void* data, 
						   uint dwtLevels, float quantStep, float bgLevel, int tileSize, float conversion, float readNoise, int onDevice, GPUResources* res) {

		herr_t r;
		int outDataLength;
		cudaError status;
		//GPUResources* res = *pRes;

		uint sizeX = size[2];
		uint sizeY = size[1];
		uint sizeZ = size[0];

		uint elemCount = sizeX * sizeY * sizeZ;
		outDataLength = elemCount * sizeof(short);

		short* dpImage = res->getBuffer<short>(elemCount);;
		float* dpBuffer = res->getBuffer<float>(elemCount);
		float* dpScratch = res->getBuffer<float>(elemCount);
		cudaCompress::Symbol16* dpSymbols = res->getBuffer<Symbol16>(elemCount);

		std::vector<uint> bitStream;

		if (!onDevice)
			cudaMemcpy(dpImage, data, outDataLength, cudaMemcpyHostToDevice);
		else
			dpImage = (short*)data;

		if (quantStep > 0) {
			// start lossy compression
			compressImage(res->m_pCuCompInstance, bitStream, dpImage, dpBuffer, dpScratch, dpSymbols, sizeX, sizeY, sizeZ, dwtLevels, quantStep, bgLevel, tileSize, conversion, readNoise);
		}
		else {
			// start lossless compression
			compressImageLL(res->m_pCuCompInstance, bitStream, dpImage, (short*)dpBuffer, (short*)dpScratch, dpSymbols, sizeX, sizeY, sizeZ, dwtLevels, tileSize);
		}
		/* compressed data is now in bitStream
		copy it to output buffer */
		outDataLength = bitStream.size() * sizeof(uint);
		
		//memcpy(*buf, bitStream.data(), outDataLength);
		
		r = H5DOwrite_chunk(dset_id, 0, 0, offset, outDataLength, bitStream.data());

		res->releaseBuffers(4);


		return 0;
	}

#ifdef __cplusplus
}
#endif
