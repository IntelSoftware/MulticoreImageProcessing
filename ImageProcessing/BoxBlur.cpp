#include "stdafx.h"
#include <fstream>
#include "omp.h"

using namespace std;

extern "C" __declspec(dllexport) int __stdcall BoxBlur(BYTE* inBGR, BYTE* outBGR, int stride, int width, int height, KVP* arr, int nArr)
{
	// Pack the following structure on one-byte boundaries: smallest possible alignment
	// This allows to use the minimal memory space for this type: exact fit - no padding 
#pragma pack(push, 1)
	struct BGRA {
		BYTE B, G, R, A;
	};
#pragma  pack(pop)		// Back to the default packing mode

	// Reading the input parameters
	bool openMP = parameter("openMP", 1, arr, nArr) == 1 ? true : false;	// If openMP should be used for multithreading
	const double radius = parameter("radius", 1, arr, nArr);					// Radius of the convolution kernel

	// Setting up the convolution kernel
	const double size = radius * 2 + 1;
	double k = 1.0 / (size*size);
	// Allocating the memory for the convolution kernel 
	double** matrix = new double*[size];
	for (int i = 0; i < size; i++) {
		matrix[i] = new double[size];
	}
	// Each element of the matrix has the same weight
	// The sum of every item has to be one 
	// So that the picture is not darken or lighten
	for (int i = 0;i < size;i++) {
		for (int j = 0;j < size;j++) {
			matrix[i][j] = k;
		}
	}

	// If the boolean openMP is true, this directive is interpreted so that the following for loop
	// will be run on multiple cores.
#pragma omp parallel for if(openMP)			
	for (int i = 0; i < height; ++i) {
		auto offset = i * stride;
		BGRA* p = reinterpret_cast<BGRA*>(inBGR + offset);
		BGRA* q = reinterpret_cast<BGRA*>(outBGR + offset);
		for (int j = 0; j < width; ++j) {
			if (i == 0 || j == 0 || i == height - 1 || j == width - 1)
				q[j] = p[j];	// if convolution not possible (near the edges)
			else {
				BYTE R, G, B;
				double red(0), blue(0), green(0);
				// Apply the convolution kernel to every applicable pixel of the image
				for (int jj = 0, dY = -radius; jj < size; jj++, dY++) {
					for (int ii = 0, dX = -radius; ii < size; ii++, dX++) {
						int index = j + dX + dY * width;
						// Multiply each element in the local neighboorhood of the center pixel
						//  by the corresponding element in the convolution kernel
						// For the three colors
						blue += p[index].B * matrix[ii][jj];
						red += p[index].R * matrix[ii][jj];
						green += p[index].G * matrix[ii][jj];
					}
				}
				// Writing the results to the output image
				B = blue;
				R = red;
				G = green;
				q[j] = BGRA{ B,G,R,255 };
			}
		}
	}

	// Delete the allocated memory for the convolution kernel
	for (int i = 0;i < size;i++) {
		delete matrix[i];
	}
	delete[] matrix;
	return 0;
}