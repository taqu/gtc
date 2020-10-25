#ifndef INC_QDISC_H_
#define INC_QDISC_H_
#include "gtc.h"

namespace gtc
{
    struct QDisc
    {
        bool exists_;
        s32 delay_;
        s32 delayJitter_;
        f32 loss_;
        f32 corrupt_;
        s32 bandwidth_; //< k bits per second

        bool hasNetem() const;
        bool hasTbf() const;
    };

    QDisc checkQDisc(const char* name);
    bool deleteQDisc(const char* name);
    QDisc addQDisc(const char* name, const QDisc& qdisc);
}
#endif //INC_QDISC_H_

