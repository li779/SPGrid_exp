#pragma once

#include "Parameters.h"
#include "CSRMatrix.h"
#include "Config.h"

void ConjugateGradients(
    CSRMatrix& matrix1,
    CSRMatrix& matrix2,
    float (&x)[XDIM][YDIM][ZDIM],
    const float (&f)[XDIM][YDIM][ZDIM],
    float (&p)[XDIM][YDIM][ZDIM],
    float (&r)[XDIM][YDIM][ZDIM],
    float (&z)[XDIM][YDIM][ZDIM],
    const bool writeIterations = true);

void SPGridConjugateGradients(
    DataArrayType& x_array, 
    DataArrayType& f_array, 
    DataArrayType& p_array,
    DataArrayType& r_array, 
    DataArrayType& z_array, 
    MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, 
    const unsigned block_size, 
    const uint64_t* blocks);

