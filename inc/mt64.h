#define __STDC_FORMAT_MACROS 1
#include "../inc/mt64.hpp"
#include <inttypes.h>
int testmt64 (void) {
	    printf("%" PRIu64 "\n", rps_random_uint64());
	        return 0;
}