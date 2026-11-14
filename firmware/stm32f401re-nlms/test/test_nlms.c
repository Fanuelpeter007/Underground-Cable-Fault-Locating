/**
 * @file test_nlms.c
 * @brief Host-side test of the NLMS noise canceller. No hardware needed.
 *
 * Builds a synthetic case like the field setup:
 *   - wanted signal: 150 Hz thump bursts, present only at the primary mic
 *   - noise: white noise at the reference mic, reaching the primary mic
 *     through a short unknown acoustic path (FIR)
 *
 * Passes if the noise power left in the output is at least 20 dB below the
 * noise power at the primary mic, measured over the last second.
 *
 * Build and run (any C99 compiler):
 *   cc -std=c99 -O2 -I../Core/Inc test_nlms.c ../Core/Src/nlms.c -lm -o test_nlms
 *   ./test_nlms
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "nlms.h"

#define FS_HZ     16000
#define SECONDS   5
#define N         (FS_HZ * SECONDS)
#define PI_F      3.14159265f

/* Small linear congruential generator so the test is repeatable everywhere. */
static unsigned long rng_state = 12345UL;
static float uniform(void)
{
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (float)((rng_state >> 16) & 0x7FFF) / 32768.0f;
}

/* Box-Muller transform for Gaussian noise. */
static float gaussian(void)
{
    float u1 = uniform();
    float u2 = uniform();
    if (u1 < 1e-7f) {
        u1 = 1e-7f;
    }
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * PI_F * u2);
}

int main(void)
{
    /* Unknown path from the noise source to the primary mic. */
    static const float path[] = {0.0f, 0.6f, -0.3f, 0.15f, 0.05f};
    const int path_len = (int)(sizeof(path) / sizeof(path[0]));

    static float ref[N];
    for (int n = 0; n < N; n++) {
        ref[n] = 0.1f * gaussian();
    }

    nlms_t f;
    nlms_init(&f, 32, 0.05f, 1e-6f);

    double noise_in = 0.0;    /* noise power at the primary mic */
    double noise_out = 0.0;   /* noise power left after the filter */
    const int start = N - FS_HZ;

    for (int n = 0; n < N; n++) {
        /* 150 Hz burst for 50 ms every 0.5 s: a simple stand-in for a thump. */
        const int in_burst = (n % (FS_HZ / 2)) < (FS_HZ / 20);
        const float thump = in_burst ? 0.05f * sinf(2.0f * PI_F * 150.0f * n / FS_HZ) : 0.0f;

        float noise_at_primary = 0.0f;
        for (int k = 0; k < path_len; k++) {
            if (n - k >= 0) {
                noise_at_primary += path[k] * ref[n - k];
            }
        }

        const float e = nlms_process(&f, thump + noise_at_primary, ref[n]);

        if (n >= start) {
            noise_in += (double)noise_at_primary * noise_at_primary;
            const double residual = (double)e - thump;
            noise_out += residual * residual;
        }
    }

    const double reduction_db = 10.0 * log10(noise_in / noise_out);
    printf("Noise reduction over the last second: %.1f dB\n", reduction_db);

    if (reduction_db < 20.0) {
        printf("FAIL: expected at least 20 dB\n");
        return EXIT_FAILURE;
    }
    printf("PASS\n");
    return EXIT_SUCCESS;
}
