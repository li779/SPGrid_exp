#include "ConjugateGradients.h"
#include "Laplacian.h"
#include "Timer.h"
#include "Utilities.h"
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Set.h>
#include "Blocked_Copy_Helper.h"
#include "Laplace_Helper.h"
#include <SPGrid/Data_Structures/std_array.h>

#include "PTHREAD_QUEUE.h"
#include "ADAPTIVE_SPHERE_RASTERIZER.h"
#include "DENSE_CUBE_RASTERIZER.h"
#include "FACE_INITIALIZER.h"
#include "GEOMETRY_BLOCK.h"
#include "HIERARCHICAL_RASTERIZER.h"

Timer timerLaplacian;
extern PTHREAD_QUEUE* pthread_queue;
using namespace SPGrid;

typedef float T;
typedef struct CG_struct {
    T x,f,p,r,z,array1;
    unsigned flags;
} CG;

typedef SPGrid_Allocator<CG,3> CG_Allocator;
typedef SPGrid_Allocator<CG,3>::Array<>::mask CG_Mask;
typedef SPGrid_Allocator<CG,3>::Array<T>::type Data_array_type;
typedef SPGrid_Allocator<CG,3>::Array<const T>::type Const_data_array_type;
typedef SPGrid_Allocator<CG,3>::Array<unsigned>::type Flags_array_type;
typedef SPGrid_Set<Flags_array_type> Flags_set_type;
typedef std_array<int,3> Vec3i;
typedef std_array<float,3> Vec3f;

int main(int argc, char *argv[])
{

    if (argc != 3) {
        printf("Please specify size (power of two), and number of threads\n");
        exit(1);
    }
    int size = atoi(argv[1]);
    if ((size & (size-1)) != 0) {
        printf("For this limited demo, size must be a power of two.\n");
        exit(1);
    }
    int n_threads = atoi(argv[2]);
    pthread_queue = new PTHREAD_QUEUE(n_threads);

    
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

    // Initialization
    {
        Timer timer;
        timer.Start();
        InitializeProblem(x, f);
        matrix1 = BuildLaplacianMatrix(); // This takes a while ...
        matrix2 = BuildLaplacianMatrixLowerTriangular(); // This takes a while ...
        timer.Stop("Initialization : ");
    }

    // Call Conjugate Gradients algorithm
    timerLaplacian.Reset();
    ConjugateGradients(matrix1, matrix2, x, f, p, r, z, false);
    timerLaplacian.Print("Total Laplacian Time : ");

    return 0;
}
