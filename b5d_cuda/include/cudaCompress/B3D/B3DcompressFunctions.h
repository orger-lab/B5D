#ifndef __B3D__COMPR_FUNC_H__
#define __B3D__COMPR_FUNC_H__


#include <cuda_runtime.h>

#include "../Encode.h" // cudaCompress/
#include "../util/DWT.h"
#include "../util/Quantize.h"
#include "../util/Predictors.h"
#include "../cudaUtil.h"


namespace cudaCompress {

	//namespace util {
		CUCOMP_DLL void compressImageLL( //CUCOMP_DLL 
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			int16_t* dpBuffer,
			int16_t* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, int tileSize);               // quantization step
		
		CUCOMP_DLL void decompressImageLL( //CUCOMP_DLL 
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			int16_t* dpBuffer,
			int16_t* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, int tileSize);

		CUCOMP_DLL void compressImage( //CUCOMP_DLL 
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			float* dpBuffer,
			float* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion = 1., float readNoise = 0.);               // quantization step
		
		CUCOMP_DLL void decompressImage( //CUCOMP_DLL 
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			float* dpBuffer,
			float* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion = 1., float readNoise = 0.);



}

#endif
