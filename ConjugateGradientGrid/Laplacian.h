#pragma once

#include "CSRMatrix.h"
#include "Parameters.h"
#include "Config.h"
#include <iostream>

CSRMatrix BuildLaplacianMatrix();
CSRMatrix BuildLaplacianMatrixLowerTriangular();

void ComputeLaplacian(CSRMatrix& laplacianMatrix,
    const float (&u)[XDIM][YDIM][ZDIM], float (&Lu)[XDIM][YDIM][ZDIM],
    bool usingSymmetricLowerTriangular=false);

void SPGridLaplacian(DataArrayType& u_array, DataArrayType& Lu_array, MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks);