#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "my_filter.h"


int main(void)
{
    float impulse[64] = {1.0f};
    float output[64];

    for (int i = 0; i < 64; ++i)
        output[i] = -1.0f;

    arm_fir_f32_lp(impulse, 64, output);

    assert(fabsf(output[0] - 0.002915534889f) < 1e-7f);
    assert(fabsf(output[25] - 0.03702224046f) < 1e-7f);
    assert(fabsf(output[50] - 0.002915534889f) < 1e-7f);
    assert(fabsf(output[51]) < 1e-7f);

    puts("FIR low-pass test passed");
    return 0;
}
