#include "ConjugateGradients.h"
#include "Laplacian.h"
#include "PointwiseOps.h"
#include "Reductions.h"
#include "Utilities.h"
#include "Timer.h"

#include <iostream>

extern Timer timerLaplacian;

void ConjugateGradients(
    CSRMatrix& matrix1,
    CSRMatrix& matrix2,
    float (&x)[XDIM][YDIM][ZDIM],
    const float (&f)[XDIM][YDIM][ZDIM],
    float (&p)[XDIM][YDIM][ZDIM],
    float (&r)[XDIM][YDIM][ZDIM],
    float (&z)[XDIM][YDIM][ZDIM],
    const bool writeIterations)
{
    // Algorithm : Line 2
    timerLaplacian.Restart(); ComputeLaplacian(matrix1, x, z); timerLaplacian.Pause();
    Saxpy(z, f, r, -1);
    float nu = Norm(r);

    // Algorithm : Line 3
    if (nu < nuMax) return;
        
    // Algorithm : Line 4
    Copy(r, p);
    float rho=InnerProduct(p, r);
        
    // Beginning of loop from Line 5
    for(int k=0;;k++)
    {
        std::cout << "Residual norm (nu) after " << k << " iterations = " << nu << std::endl;

        // Algorithm : Line 6
        timerLaplacian.Restart(); ComputeLaplacian(matrix2, p, z, true); timerLaplacian.Pause();
        float sigma=InnerProduct(p, z);

        // Algorithm : Line 7
        float alpha=rho/sigma;

        // Algorithm : Line 8
        Saxpy(z, r, r, -alpha);
        nu=Norm(r);

        // Algorithm : Lines 9-12
        if (nu < nuMax || k == kMax) {
            Saxpy(p, x, x, alpha);
            std::cout << "Conjugate Gradients terminated after " << k << " iterations; residual norm (nu) = " << nu << std::endl;
            if (writeIterations) WriteAsImage("x", x, k, 0, 127);
            return;
        }
            
        // Algorithm : Line 13
        Copy(r, z);
        float rho_new = InnerProduct(z, r);

        // Algorithm : Line 14
        float beta = rho_new/rho;

        // Algorithm : Line 15
        rho=rho_new;

        // Algorithm : Line 16
        Saxpy(p, x, x, alpha);
        Saxpy(p, r, p, beta);

        if (writeIterations) WriteAsImage("x", x, k, 0, 127);
    }
}


void SPGridConjugateGradients(
    DataArrayType& x_array, 
    DataArrayType& f_array, 
    DataArrayType& p_array, 
    DataArrayType& r_array, 
    DataArrayType& z_array, 
    MaskArrayType& mask_array, 
    const unsigned nElementsPerBlock, 
    const unsigned block_size, 
    const uint64_t* blocks){
    // Algorithm : Line 2
    timerLaplacian.Restart(); SPGridLaplacian(x_array, z_array, mask_array, nElementsPerBlock, block_size, blocks); timerLaplacian.Pause();
    SPGridSaxpy(z_array, f_array, r_array, mask_array, nElementsPerBlock, block_size, blocks, -1);
    float nu = SPGridNorm(r_array, mask_array, nElementsPerBlock, block_size, blocks);

    // Algorithm : Line 3
    if (nu < nuMax) return;
        
    // Algorithm : Line 4
    SPGridCopy(r_array, p_array, mask_array, nElementsPerBlock, block_size, blocks);
    float rho=SPGridInnerProduct(p_array, r_array, mask_array, nElementsPerBlock, block_size, blocks);
        
    // Beginning of loop from Line 5
    for(int k=0;;k++)
    {
        std::cout << "Residual norm (nu) after " << k << " iterations = " << nu << std::endl;

        // Algorithm : Line 6
        timerLaplacian.Restart(); SPGridLaplacian(p_array, z_array, mask_array, nElementsPerBlock, block_size, blocks); timerLaplacian.Pause();
        float sigma=SPGridInnerProduct(p_array, z_array, mask_array, nElementsPerBlock, block_size, blocks);

        // Algorithm : Line 7
        float alpha=rho/sigma;

        // Algorithm : Line 8
        SPGridSaxpy(z_array, r_array, r_array, mask_array, nElementsPerBlock, block_size, blocks, -alpha);
        nu=SPGridNorm(r_array, mask_array, nElementsPerBlock, block_size, blocks);

        // Algorithm : Lines 9-12
        if (nu < nuMax || k == kMax) {
            SPGridSaxpy(p_array, x_array, x_array, mask_array, nElementsPerBlock, block_size, blocks, alpha);
            std::cout << "Conjugate Gradients terminated after " << k << " iterations; residual norm (nu) = " << nu << std::endl;
            return;
        }
            
        // Algorithm : Line 13
        SPGridCopy(r_array, z_array, mask_array, nElementsPerBlock, block_size, blocks);
        float rho_new = SPGridInnerProduct(z_array, r_array, mask_array, nElementsPerBlock, block_size, blocks);

        // Algorithm : Line 14
        float beta = rho_new/rho;

        // Algorithm : Line 15
        rho=rho_new;

        // Algorithm : Line 16
        SPGridSaxpy(p_array, x_array, x_array,mask_array, nElementsPerBlock, block_size, blocks, alpha);
        SPGridSaxpy(p_array, r_array, p_array,mask_array, nElementsPerBlock, block_size, blocks, beta);

        
    }

}
