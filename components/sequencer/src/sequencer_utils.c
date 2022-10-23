#include <sequencer_utils.h>


static unsigned long seq_rand_table[3] = { 123456789, 362436069, 521288629 };


unsigned long seq_rand() {
    unsigned long x = seq_rand_table[0];
    unsigned long y = seq_rand_table[1];
    unsigned long z = seq_rand_table[2];
    unsigned long t;

    // using Marsaglia's xorshf
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    seq_rand_table[0] = x;
    seq_rand_table[1] = y;
    seq_rand_table[2] = z;

    return z;
}