#include <cudaCompress/B3D/B3DcompressFunctions.h>
//#include <stdio.h>      /* printf */
#include <time.h>       /* time_t, struct tm, difftime, time, mktime */
#include <windows.h>

//#define H5Z_CUDACOMPRESS_DEBUG

namespace cudaCompress {

	//namespace util {

		void compressImageLL(
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			int16_t* dpBuffer,
			int16_t* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, int tileSize)               // quantization step
		{
			sizeY = sizeY * sizeZ;
			// Do multi-level DWT in the same buffers. Need to specify pitch now!

			cudaMemcpy(dpBuffer, dpImage, sizeX*sizeY * sizeof(int16_t), cudaMemcpyDeviceToDevice);
			switch (dwtLevel) {
			case 1:
			case 2:
				cudaCompress::util::predictor7_tiles(dpImage, dpBuffer, sizeX * sizeof(int16_t), sizeX, sizeY, tileSize);
				break;
			default:
				break;
			}
			cudaCheckMsg("predictor failed");


			// dpBuffer now contains the multi-level DWT decomposition.

			// Quantize the coefficients and convert them to unsigned values (symbols).
			// For better compression, quantStep should be adapted to the transform level!
			cudaCompress::util::symbolize(dpSymbols, dpBuffer, sizeX, sizeY, 1);
			//cudaMemcpy(dpSymbols, dpBuffer, sizeX * sizeY * sizeof(int16_t), cudaMemcpyDeviceToDevice);

			// Run-length + Huffman encode the quantized coefficients.
			cudaCompress::BitStream bitStream(&i_bitStream);
			cudaCompress::encodeRLHuff(pInstance, bitStream, &dpSymbols, 1, sizeX * sizeY);
			//return bitStream;
		}

		// dpSymbols has to be initialized to 0
		void decompressImageLL(
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			int16_t* dpBuffer,
			int16_t* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, int tileSize)
		{
			sizeY = sizeY * sizeZ;
			BitStreamReadOnly bitStream(i_bitStream.data(), uint(i_bitStream.size() * sizeof(uint) * 8));
			cudaCompress::decodeRLHuff(pInstance, bitStream, &dpSymbols, 1, sizeX * sizeY);

			cudaCompress::util::unsymbolize(dpBuffer, dpSymbols, sizeX, sizeY, 1);
			cudaMemcpy(dpImage, dpBuffer, sizeX*sizeY * sizeof(int16_t), cudaMemcpyDeviceToDevice);
			switch (dwtLevel) {
			case 1:
			case 2:
				cudaCompress::util::unPredictor7_tiles(dpBuffer, dpImage, sizeX * sizeof(int16_t), sizeX, sizeY, tileSize);
				break;
			default:
				break;
			}
			cudaCheckMsg("unpredictor failed");
		}

		void compressImage(
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			float* dpBuffer,
			float* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion, float readNoise)               // quantization step
		{
			sizeY = sizeY * sizeZ;
			sizeZ = 1;
			uint16_t* pdpSymbols[1] = { dpSymbols };


			cudaCheckMsg("cuda start");

#ifdef H5Z_CUDACOMPRESS_DEBUG
			fprintf(stdout, "COMPRESSION START\n");
			fprintf(stdout, "sizeX %d \n", sizeX);
			fprintf(stdout, "sizeY %d \n", sizeY);
			fprintf(stdout, "sizeZ %d \n", sizeZ);
			fprintf(stdout, "dwtLevel %lu \n", dwtLevel);
			fprintf(stdout, "quantStep %f \n", quantStep);
			fprintf(stdout, "bgLevel %f \n", bgLevel);
			fprintf(stdout, "tileSize %d \n", tileSize);
			fprintf(stdout, "conversion %f \n", conversion);
			fprintf(stdout, "readNoise %f \n", readNoise);/*
			fprintf(stdout, "dpImage: %d %d %d %d %d %d\n", dpImage[0], dpImage[1], dpImage[2], dpImage[3], dpImage[4], dpImage[5]);
			fprintf(stdout, "i_bitStream: %u %u %u %u %u %u\n", i_bitStream[0], i_bitStream[1], i_bitStream[2], i_bitStream[3], i_bitStream[4], i_bitStream[5]);*/

#endif
			// c++ timers
			//time_t cpp_start, cpp_end;
			//SYSTEMTIME st_start, st_end;
			//double seconds;
			//GetSystemTime(&st_start);
			switch (dwtLevel) {
			case 1: // first version, square root /w readnoise + prediction7 + quantization within noise level

				//time(&cpp_start);

				cudaCompress::util::u2f((uint16_t*)dpImage, dpBuffer, sizeX * sizeY);
				// variance stabilization
				cudaCompress::util::vst(dpBuffer, dpBuffer, sizeX * sizeY, bgLevel, conversion, readNoise);
				// scale with quantization step
				cudaCompress::util::multiply(dpBuffer, dpBuffer, 1 / quantStep, sizeX * sizeY);
				// run prediction + quantization
				cudaCompress::util::predictor7_tiles_wnll(dpBuffer, dpScratch, dpImage, sizeX, sizeX, sizeY, tileSize);
				cudaCompress::util::symbolize(dpSymbols, dpImage, sizeX, sizeY, sizeZ);
				//time(&cpp_end);
				break;
			case 2: // swapped: square root /w readnoise + quantization + prediction7
				cudaCompress::util::u2f((uint16_t*)dpImage, dpBuffer, sizeX * sizeY);
				// variance stabilization
				cudaCompress::util::vst(dpBuffer, dpBuffer, sizeX * sizeY, bgLevel, conversion, readNoise);
				// scale with quantization step
				cudaCompress::util::multiply(dpBuffer, dpBuffer, 1 / quantStep, sizeX * sizeY);
				// run  quantization first then prediction
				cudaCompress::util::f2u(dpBuffer, (uint16_t*)dpScratch, sizeX * sizeY);
				//cudaMemcpy(dpImage, dpScratch, sizeX*sizeY * sizeof(int16_t), cudaMemcpyDeviceToDevice);
				cudaCompress::util::predictor7_tiles((int16_t*)dpScratch, dpImage, sizeX * sizeof(int16_t), sizeX, sizeY, tileSize);
				cudaCompress::util::symbolize(dpSymbols, dpImage, sizeX, sizeY, sizeZ);
				break;
			default:
				break;
			}
			cudaCheckMsg("predictor failed");
			cudaCompress::BitStream bitStream(&i_bitStream);
			cudaCompress::encodeRLHuff(pInstance, bitStream, pdpSymbols, 1, sizeX * sizeY);
			//GetSystemTime(&st_end);
			//fprintf(stdout, "everything took %d ms\n", st_end.wMilliseconds - st_start.wMilliseconds);
		}

		void decompressImage(
			Instance* pInstance,
			std::vector<uint>& i_bitStream, // bitStream for compressed data
			int16_t* dpImage,  // input image in GPU memory
			float* dpBuffer,
			float* dpScratch,
			uint16_t* dpSymbols,
			int sizeX, int sizeY, int sizeZ,         // image size
			size_t dwtLevel, float quantStep, float bgLevel, int tileSize, float conversion, float readNoise)
		{
			sizeY = sizeY * sizeZ;
			sizeZ = 1;
			BitStreamReadOnly bitStream(i_bitStream.data(), uint(i_bitStream.size() * sizeof(uint) * 8));
			//uint16_t* dpSymbolsN = dpSymbols + sizeX * sizeY;
			uint16_t* pdpSymbols[1] = { dpSymbols };
			switch (dwtLevel) {
			case 1:
				cudaCompress::decodeRLHuff(pInstance, bitStream, pdpSymbols, 1, sizeX * sizeY);
				cudaCompress::util::unsymbolize(dpImage, dpSymbols, sizeX, sizeY, sizeZ);
				//cudaCompress::util::unQuantize(dpImage, dpBuffer, sizeX, sizeX, sizeY);
				cudaCompress::util::unPredictor7_tiles_wnll(dpImage, dpBuffer, sizeX * sizeof(int16_t), sizeX, sizeY, tileSize);
				// undo scaling from quantization
				cudaCompress::util::multiply(dpBuffer, dpBuffer, quantStep, sizeX * sizeY);
				// inverse variance stabilization
				cudaCompress::util::invVst(dpBuffer, dpBuffer, sizeX * sizeY, bgLevel, conversion, readNoise);
				//back to int16_t from float
				cudaCompress::util::f2u(dpBuffer, (uint16_t*)dpImage, sizeX*sizeY);
				break;

			case 2:
				cudaCompress::decodeRLHuff(pInstance, bitStream, pdpSymbols, 1, sizeX * sizeY);
				cudaCompress::util::unsymbolize(dpImage, dpSymbols, sizeX, sizeY, sizeZ);

				cudaCompress::util::unPredictor7_tiles(dpImage, (int16_t*)dpScratch, sizeX * sizeof(int16_t), sizeX, sizeY, tileSize);
				cudaCompress::util::u2f((uint16_t *)dpScratch, dpBuffer, sizeX*sizeY);

				cudaCompress::util::multiply(dpBuffer, dpBuffer, quantStep, sizeX * sizeY);
				// inverse variance stabilization
				cudaCompress::util::invVst(dpBuffer, dpBuffer, sizeX * sizeY, bgLevel, conversion, readNoise);
				//back to int16_t from float
				cudaCompress::util::f2u(dpBuffer, (uint16_t*)dpImage, sizeX*sizeY);
				break;
			default:
				break;
			}
			cudaCheckMsg("unpredictor failed");

		}



}