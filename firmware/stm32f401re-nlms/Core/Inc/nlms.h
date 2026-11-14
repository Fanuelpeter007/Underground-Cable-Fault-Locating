/**
 * @file nlms.h
 * @brief Normalized LMS adaptive noise canceller.
 *
 * Two inputs are used:
 *   - primary   d(n): microphone near the ground (thump + ambient noise)
 *   - reference x(n): microphone facing away from the ground (ambient noise)
 *
 * The filter estimates the part of d(n) that is correlated with x(n) and
 * removes it:
 *
 *   y(n)   = w(n)^T x(n)                       filter output (noise estimate)
 *   e(n)   = d(n) - y(n)                       error = cleaned signal
 *   w(n+1) = w(n) + mu * e(n) * x(n) / (eps + x(n)^T x(n))
 *
 * The error e(n) is the signal sent to the headphones.
 *
 * This file has no hardware dependencies, so it can be unit tested on a PC.
 */
#ifndef NLMS_H
#define NLMS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of filter taps. Sets the size of the state arrays. */
#define NLMS_MAX_TAPS 64

typedef struct {
    uint16_t taps;               /**< Number of taps in use (<= NLMS_MAX_TAPS). */
    float    mu;                 /**< Step size, 0 < mu < 2. */
    float    eps;                /**< Small constant that avoids division by zero. */
    float    w[NLMS_MAX_TAPS];   /**< Filter weights. */
    float    x[NLMS_MAX_TAPS];   /**< Delay line of reference samples, x[0] is newest. */
    float    power;              /**< Running sum of x^2 over the delay line. */
} nlms_t;

/**
 * @brief Reset the filter and set its parameters.
 * @param f    Filter state.
 * @param taps Number of taps, clamped to 1..NLMS_MAX_TAPS.
 * @param mu   Step size (0 < mu < 2). Smaller is slower but more stable.
 * @param eps  Regularization constant, for example 1e-6.
 */
void nlms_init(nlms_t *f, uint16_t taps, float mu, float eps);

/**
 * @brief Process one sample pair and adapt the weights.
 * @param f         Filter state.
 * @param primary   d(n), the noisy signal.
 * @param reference x(n), the noise reference.
 * @return e(n), the noise-reduced signal.
 */
float nlms_process(nlms_t *f, float primary, float reference);

#ifdef __cplusplus
}
#endif

#endif /* NLMS_H */
