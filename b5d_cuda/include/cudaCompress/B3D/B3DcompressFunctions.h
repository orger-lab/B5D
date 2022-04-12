#ifndef __B3D__COMPR_FUNC_H__
#define __B3D__COMPR_FUNC_H__


#include <cuda_runtime.h>

#include "../Encode.h" // cudaCompress/
#include "../util/DWT.h"
#include "../util/Quantize.h"
#include "../util/Predictors.h"
#include "../cudaUtil.h"

/*
#include "../CPU/EncodeCPU.h"
#include "../CPU/QuantizeCPU.h"
#include "../CPU/PredictorsCPU.h"
*/

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


		//CUCOMP_DLL void compressImageLLCPU( //CUCOMP_DLL 
		//	//Instance* pInstance,
		//	std::vector<uint>& i_bitStream, // bitStream for compressed data
		//	int16_t* pImage,  // input image in GPU memory
		//	int16_t* pBuffer,
		//	int16_t* pScratch,
		//	uint16_t* pSymbols,
		//	int sizeX, int sizeY, int sizeZ,         // image size
		//	size_t dwtLevel, int tileSize);               // quantization step

		//CUCOMP_DLL void decompressImageLLCPU( //CUCOMP_DLL 
		//	//Instance* pInstance,
		//	std::vector<uint>& i_bitStream, // bitStream for compressed data
		//	int16_t* pImage,  // input image in GPU memory
		//	int16_t* pBuffer,
		//	int16_t* pScratch,
		//	uint16_t* pSymbols,
		//	int sizeX, int sizeY, int sizeZ,         // image size
		//	size_t dwtLevel, int tileSize);

		//CUCOMP_DLL void compressImageCPU( //CUCOMP_DLL 
		//	//Instance* pInstance,
		//	std::vector<uint>& i_bitStream, // bitStream for compressed data
		//	int16_t* pImage,  // input image in GPU memory
		//	float* pBuffer,
		//	float* pScratch,
		//	uint16_t* pSymbols,
		//	int sizeX, int sizeY, int sizeZ,         // image size
		//	size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion = 1., float readNoise = 0.);               // quantization step

		//CUCOMP_DLL void decompressImageCPU( //CUCOMP_DLL 
		//	//Instance* pInstance,
		//	std::vector<uint>& i_bitStream, // bitStream for compressed data
		//	int16_t* pImage,  // input image in GPU memory
		//	float* pBuffer,
		//	float* pScratch,
		//	uint16_t* pSymbols,
		//	int sizeX, int sizeY, int sizeZ,         // image size
		//	size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion = 1., float readNoise = 0.);
	//}
}

#endif
