#include <SPGrid/Core/SPGrid_Allocator.h>
#include <SPGrid/Core/SPGrid_Page_Map.h>
#include "vdb_nano.h"

#define XDIM 512
#define YDIM 512
#define ZDIM 512
#define LIMIT 0.001

struct MyStruct
{
    float u;
    float Lu;
    uint32_t mask;

    bool operator!= (const MyStruct& compStruct) const{
        return (std::abs(u-compStruct.u)>LIMIT) ||
               (std::abs(Lu-compStruct.Lu)>LIMIT) ||
               (mask!=compStruct.mask);
    }
    void copy(const MyStruct& compStruct){u=compStruct.u; Lu=compStruct.Lu; mask=compStruct.mask;}
};

using AllocatorType = SPGrid::SPGrid_Allocator<MyStruct,3>;
using PageMapType = SPGrid::SPGrid_Page_Map<>;
using DataArrayType = AllocatorType::Array_type<float>;
using MaskArrayType = AllocatorType::Array_type<uint32_t>;

template<class VDBTree>
class SPGridOperator
{
public:
    SPGridOperator():allocator(AllocatorType(XDIM, YDIM, ZDIM)),pagemap(PageMapType(allocator)){}
    void allocate_SPGrid(){
        DataArrayType u_array = allocator.Get_Array(&MyStruct::u);
        DataArrayType Lu_array = allocator.Get_Array(&MyStruct::Lu);
        MaskArrayType mask_array = allocator.Get_Array(&MyStruct::mask);

    };
private:
    AllocatorType allocator;
    PageMapType pagemap;
};