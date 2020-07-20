#include "Reductions.h"

#include <algorithm>

float Norm(const float (&x)[XDIM][YDIM][ZDIM])
{
    float result = 0.;

#pragma omp parallel for reduction(max:result)
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        result = std::max(result, std::abs(x[i][j][k]));

    return result;
}

float InnerProduct(const float (&x)[XDIM][YDIM][ZDIM], const float (&y)[XDIM][YDIM][ZDIM])
{
    double result = 0.;

#pragma omp parallel for reduction(+:result)
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++)
        result += (double) x[i][j][k] * (double) y[i][j][k];

    return (float) result;
}

float SPGridNorm(const DataArrayType& x_array, MaskArrayType& mask_array,
    const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks){
        float result = 0.;

#pragma omp parallel for reduction(max:result)
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
        const auto maskPtr = &mask_array(blockOffset);
        auto offset = blockOffset;

        for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
            if (maskPtr[e] & MyFlags::zExistsFlag)
                result = std::max(result, std::abs(x_array(offset)));
        }
    return result;
}

float SPGridInnerProduct(const DataArrayType& x_array, const DataArrayType& y_array, 
    MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks){

    double result = 0.;

#pragma omp parallel for reduction(+:result)
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
        const auto maskPtr = &mask_array(blockOffset);
        auto offset = blockOffset;

        for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
            if (maskPtr[e] & MyFlags::zExistsFlag)
                result += (double) x_array(offset) * (double) y_array(offset);
        }
    return (float) result;
}
