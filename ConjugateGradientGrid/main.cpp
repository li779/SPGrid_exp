#include "ConjugateGradients.h"
#include "Laplacian.h"
#include "Timer.h"
#include "Utilities.h"
#include "Init.h"

Timer timerLaplacian;

int main(int argc, char *argv[])
{
    using array_t = float (&) [XDIM][YDIM][ZDIM];

    float *xRaw = new float [XDIM*YDIM*ZDIM];
    float *fRaw = new float [XDIM*YDIM*ZDIM];
    float *pRaw = new float [XDIM*YDIM*ZDIM];
    float *rRaw = new float [XDIM*YDIM*ZDIM];
    float *zRaw = new float [XDIM*YDIM*ZDIM];
    
    array_t x = reinterpret_cast<array_t>(*xRaw);
    array_t f = reinterpret_cast<array_t>(*fRaw);
    array_t p = reinterpret_cast<array_t>(*pRaw);
    array_t r = reinterpret_cast<array_t>(*rRaw);
    array_t z = reinterpret_cast<array_t>(*zRaw);
    
    CSRMatrix matrix1;
    CSRMatrix matrix2;

    AllocatorType allocator(XDIM, YDIM, ZDIM);
    PageMapType pageMap(allocator);

    DataArrayType x_array = allocator.Get_Array(&ConjugateGradientStruct::x);
    DataArrayType f_array = allocator.Get_Array(&ConjugateGradientStruct::f);
    DataArrayType p_array = allocator.Get_Array(&ConjugateGradientStruct::p);
    DataArrayType r_array = allocator.Get_Array(&ConjugateGradientStruct::r);
    DataArrayType z_array = allocator.Get_Array(&ConjugateGradientStruct::z);
    MaskArrayType mask_array = allocator.Get_Array(&ConjugateGradientStruct::mask);


    // Initialization
    {
        Timer timer;
        timer.Start();
        InitializeProblem(x, f);
        matrix1 = BuildLaplacianMatrix(); // This takes a while ...
        matrix2 = BuildLaplacianMatrixLowerTriangular(); // This takes a while ...
        timer.Stop("Initialization : ");
    }

    {
        Timer timer;
        timer.Start();
        initialize(x, f, p, r, z, mask_array, pageMap, x_array, f_array, p_array, r_array, z_array);
        timer.Stop("Initialization : ");
    }

    // Call Conjugate Gradients algorithm
    timerLaplacian.Reset();
    ConjugateGradients(matrix1, matrix2, x, f, p, r, z, false);
    timerLaplacian.Print("Total Laplacian Time : ");

    return 0;
}
