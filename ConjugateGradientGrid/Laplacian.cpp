#include "CSRMatrixHelper.h"
#include "Laplacian.h"
#include "MatVecMultiply.h"

void SPGridLaplacian(DataArrayType& u_array, DataArrayType& Lu_array,MaskArrayType& mask_array,const unsigned nElementsPerBlock, const unsigned block_size, const uint64_t* blocks)
{
    
    std::cout << "blocks.second = " <<block_size << std::endl;

    #pragma omp parallel for
    for (int b = 0; b < block_size; b++) {

        const auto blockOffset = blocks[b];
        const auto maskPtr = &mask_array(blockOffset);
        auto offset = blockOffset;

        for (int e = 0; e < nElementsPerBlock; e++, offset += sizeof(float))
            if (maskPtr[e] & MyFlags::zExistsFlag)
                Lu_array(offset) =
                    -6 * u_array.operator()<0,0,0>(offset)
                        + u_array.operator()<+1,0,0>(offset)
                        + u_array.operator()<-1,0,0>(offset)
                        + u_array.operator()<0,+1,0>(offset)
                        + u_array.operator()<0,-1,0>(offset)
                        + u_array.operator()<0,0,+1>(offset)
                        + u_array.operator()<0,0,-1>(offset);


    }
            
}

inline int LinearIndex(const int i, const int j, const int k)
{ return ((i * YDIM) + j) * ZDIM + k; }

CSRMatrix BuildLaplacianMatrix()
{
    static constexpr int matSize = XDIM * YDIM * ZDIM;
    CSRMatrixHelper matrixHelper(matSize);
    
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++) {
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j, k) ) = -6.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i+1, j, k) ) = 1.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i-1, j, k) ) = 1.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j+1, k) ) = 1.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j-1, k) ) = 1.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j, k+1) ) = 1.;
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j, k-1) ) = 1.;
    }

    return matrixHelper.ConvertToCSRMatrix();
}

CSRMatrix BuildLaplacianMatrixLowerTriangular()
{
    static constexpr int matSize = XDIM * YDIM * ZDIM;
    CSRMatrixHelper matrixHelper(matSize);
    
    for (int i = 1; i < XDIM-1; i++)
    for (int j = 1; j < YDIM-1; j++)
    for (int k = 1; k < ZDIM-1; k++) {
        matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j, k) ) = -6.;
        if (i > 1)
            matrixHelper( LinearIndex(i, j, k), LinearIndex(i-1, j, k) ) = 1.;
        if (j > 1)
            matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j-1, k) ) = 1.;
        if (k > 1)
            matrixHelper( LinearIndex(i, j, k), LinearIndex(i, j, k-1) ) = 1.;
    }

    return matrixHelper.ConvertToCSRMatrix();
}

void ComputeLaplacian(CSRMatrix& laplacianMatrix,
    const float (&u)[XDIM][YDIM][ZDIM], float (&Lu)[XDIM][YDIM][ZDIM],
    bool usingSymmetricLowerTriangular)
{   
    // Treat the arrays u & Lu as flattened vectors, and apply matrix-vector multiplication 
    if(usingSymmetricLowerTriangular)
        SymmetricLowerTriangularMatVecMultiply(laplacianMatrix, &u[0][0][0], &Lu[0][0][0]);
    else
        MatVecMultiply(laplacianMatrix, &u[0][0][0], &Lu[0][0][0]);
}

