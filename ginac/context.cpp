/* 
 * File:   context.cpp
 * Author: Ralf Stephan <ralf@ark.in-berlin.de>
 */

#include <stdexcept>
#include "context.h"

namespace GiNaC {


bool global_hold = false;
bool global_eval_fp = true;

void set_state(const std::string& name, bool state)
{
        std::hash<std::string> hash_fn;
        static const auto hold = hash_fn("hold");
        static const auto eval_fp = hash_fn("eval_fp");
        const auto hash = hash_fn(name);

        // the following can be changed to switch/case in C++14
        if (hash == hold)
                global_hold = state;
        else if (hash == eval_fp)
                global_eval_fp = state;
        else
                throw std::runtime_error("set_state: unknown name");
}

} // namespace GiNaC
