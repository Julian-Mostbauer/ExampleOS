#include "rand.h"
#include "types.h"

static uint32_t next = 1;

int32_t rand(void)  // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (uint32_t) (next / 65536) % 32768;
}

void srand(uint32_t seed)
{
    next = seed;
}