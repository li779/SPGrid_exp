#include <SPGrid/Core/SPGrid_Allocator.h>

template<typename DataType>
struct Buffer
{
    DataType* mdata_u;
    float operator[] (int n){mdata()}
};
