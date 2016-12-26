// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_hash.h"

namespace tb {

uint32 TBGetHash(const char *str)
{
	if (!str || !*str)
		return 0;
	// FNV-1a hash
    uint32 hash = 2166136261u;
    while (*str)
    {
        hash ^= *str++;
        hash *= 16777619u;
    }

    return hash;
}

} // namespace tb
