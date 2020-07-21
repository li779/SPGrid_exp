#pragma once

#include "Parameters.h"
#include "Config.h"

// Compute the maximum absolute value among the array elements
float Norm(const float (&x)[XDIM][YDIM][ZDIM]);

// Compute the "dot product" between the two arrays
float InnerProduct(const float (&x)[XDIM][YDIM][ZDIM], const float (&y)[XDIM][YDIM][ZDIM]);


float SPGridNorm(DataArrayType& x_array, MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);

float SPGridInnerProduct(DataArrayType& x_array, DataArrayType& y_array, 
    MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);