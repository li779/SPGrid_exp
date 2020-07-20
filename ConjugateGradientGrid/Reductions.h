#pragma once

#include "Parameters.h"
#include "Config.h"

// Compute the maximum absolute value among the array elements
float Norm(const float (&x)[XDIM][YDIM][ZDIM]);

// Compute the "dot product" between the two arrays
float InnerProduct(const float (&x)[XDIM][YDIM][ZDIM], const float (&y)[XDIM][YDIM][ZDIM]);


float SPGridNorm(const DataArrayType& x_array, const MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);

float SPGridInnerProduct(const DataArrayType& x_array, const DataArrayType& y_array, 
    const MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);