#ifndef INC_NIC_H
#define INC_NIC_H
#include <vector>
#include <string>
#include "gtc.h"

namespace gtc
{
    class NIC
    {
    public:
        NIC();
        explicit NIC(const char* name);
        NIC(const NIC& rhs);
        NIC(NIC&& rhs);
        ~NIC();

        NIC& operator=(const NIC& rhs);
        NIC& operator=(NIC&& rhs);

        inline const char* name() const;
    private:
        std::string name_;
    };

    inline const char* NIC::name() const
    {
        return name_.c_str();
    }

    void getNICs(std::vector<NIC>& nics);
}
#endif //INC_NIC_H

