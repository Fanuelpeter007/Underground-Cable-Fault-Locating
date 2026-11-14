/**
 * @file nlms.c
 * @brief Normalized LMS adaptive noise canceller. See nlms.h for the equations.
 */
#include "nlms.h"

#include <string.h>

void nlms_init(nlms_t *f, uint16_t taps, float mu, float eps)
{
    if (taps < 1) {
        taps = 1;
    }
    if (taps > NLMS_MAX_TAPS) {
        taps = NLMS_MAX_TAPS;
    }
    f->taps  = taps;
    f->mu    = mu;
    f->eps   = eps;
    f->power = 0.0f;
    memset(f->w, 0, sizeof(f->w));
    memset(f->x, 0, sizeof(f->x));
}

float nlms_process(nlms_t *f, float primary, float reference)
{
    const uint16_t n = f->taps;

    /* Update the running input power: drop the oldest sample, add the newest.
     * This keeps the cost at O(taps) per sample instead of O(2 * taps). */
    const float oldest = f->x[n - 1];
    f->power += reference * reference - oldest * oldest;
    if (f->power < 0.0f) {
        f->power = 0.0f;   /* Guard against float rounding drift. */
    }

    /* Shift the delay line and insert the new reference sample. */
    memmove(&f->x[1], &f->x[0], (size_t)(n - 1) * sizeof(float));
    f->x[0] = reference;

    /* y(n) = w^T x : estimate of the noise that reached the primary mic. */
    float y = 0.0f;
    for (uint16_t i = 0; i < n; i++) {
        y += f->w[i] * f->x[i];
    }

    /* e(n) = d(n) - y(n) : what is left is the wanted signal. */
    const float e = primary - y;

    /* Normalized weight update. */
    const float g = f->mu * e / (f->eps + f->power);
    for (uint16_t i = 0; i < n; i++) {
        f->w[i] += g * f->x[i];
    }

    return e;
}
