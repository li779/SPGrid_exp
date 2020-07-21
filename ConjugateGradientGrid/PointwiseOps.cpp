#include "PointwiseOps.h"

void Copy(const float (&x)[XDIM][YDIM][ZDIM], float (&y)[XDIM][YDIM][ZDIM])
{
#pragma omp parallel for    
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        y[i][j][k] = x[i][j][k];
}

void Saxpy(const float (&x)[XDIM][YDIM][ZDIM], const float (&y)[XDIM][YDIM][ZDIM],
    float (&z)[XDIM][YDIM][ZDIM],
    const float scale)
{
    // Should we use OpenMP parallel for here?
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        z[i][j][k] = x[i][j][k] * scale + y[i][j][k];
}



void SPGridCopy(DataArrayType& x_array, DataArrayType& y_array, MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks){
    
    #pragma omp parallel for
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
        const auto maskPtr = &mask_array(blockOffset);
        auto offset = blockOffset;

        for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
            if (maskPtr[e] & MyFlags::zExistsFlag)
                y_array(offset) = x_array(offset);
        }
}


void SPGridSaxpy(DataArrayType& x_array, DataArrayType& y_array, DataArrayType& z_array, 
    MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks, const float scale){

    //#pragma omp parallel for
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
        const auto maskPtr = &mask_array(blockOffset);
        auto offset = blockOffset;

        for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
            if (maskPtr[e] & MyFlags::zExistsFlag)
                z_array(offset) = x_array(offset) * scale + y_array(offset);
        }
}
