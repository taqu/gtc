#include "NIC.h"
#include <net/if.h>
#include <utility>

namespace gtc
{
    NIC::NIC()
    {}
    
    NIC::NIC(const char* name)
        :name_(name)
    {}

    NIC::NIC(const NIC& rhs)
        :name_(rhs.name_)
    {}

    NIC::NIC(NIC&& rhs)
        :name_(std::move(rhs.name_))
    {}

    NIC::~NIC()
    {}

    NIC& NIC::operator=(const NIC& rhs)
    {
        if(this != &rhs){
            name_ = rhs.name_;
        }
        return *this;
    }

    NIC& NIC::operator=(NIC&& rhs)
    {
        if(this != &rhs){
            name_ = std::move(rhs.name_);
        }
        return *this;
    }

    void getNICs(std::vector<NIC>& nics)
    {
        nics.clear();
        struct if_nameindex* nameindices = if_nameindex();
        if(GTC_NULL == nameindices){
            return;
        }
        for(struct if_nameindex* i=nameindices; 0 != i->if_index && GTC_NULL != i->if_name; ++i){
            NIC nic(i->if_name);
            nics.push_back(std::move(nic));
        }
        if_freenameindex(nameindices);
    }
}

