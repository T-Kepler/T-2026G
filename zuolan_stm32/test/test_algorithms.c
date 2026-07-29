#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "kalman.h"
#include "phase_measure.h"


int main(void)
{
    kalman_state filter;
    Kalman_init(&filter, 0.0f, 1.0f);
    float estimate = kalman_filter(&filter, 10.0f);
    assert(estimate > 9.0f && estimate < 10.0f);

    assert(fabsf(kalman(0, 5.0f) - 5.0f) < 1e-6f);
    assert(kalman(0, 7.0f) > 5.0f);
    assert(fabsf(kalman_thd(3.0f) - 3.0f) < 1e-6f);

    float channel1[8] = {-1, -1, 1, 1, 1, 1, -1, -1};
    float channel2[8] = {-1, -1, -1, -1, 1, 1, 1, 1};
    calculate_phase_diff(channel1, channel2, 8);
    assert(fabsf(phase_diff - 90.0f) < 1e-6f);

    puts("Kalman and zero-crossing phase tests passed");
    return 0;
}
