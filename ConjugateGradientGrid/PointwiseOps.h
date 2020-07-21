#pragma once

#include "Parameters.h"
#include "Config.h"

// Copy array x into y
void Copy(const float (&x)[XDIM][YDIM][ZDIM], float (&y)[XDIM][YDIM][ZDIM]);

// Scale array x by given number, add y, and write result into z
void Saxpy(const float (&x)[XDIM][YDIM][ZDIM], const float (&y)[XDIM][YDIM][ZDIM],
    float (&z)[XDIM][YDIM][ZDIM], const float scale);

void SPGridCopy(DataArrayType& x_array, DataArrayType& y_array, MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);

// Scale array x by given number, add y, and write result into z
void SPGridSaxpy(DataArrayType& x_array, DataArrayType& y_array, DataArrayType& z_array, 
    MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks, const float scale);