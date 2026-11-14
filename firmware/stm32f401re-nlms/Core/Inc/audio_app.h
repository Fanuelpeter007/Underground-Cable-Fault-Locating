/**
 * @file audio_app.h
 * @brief Real-time audio path: two INMP441 mics in, NLMS, Pmod I2S2 DAC out.
 */
#ifndef AUDIO_APP_H
#define AUDIO_APP_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Tuning ------------------------------------------------------------ */

/** Sample rate in Hz. Must match the I2S2 setting in CubeMX. */
#define AUDIO_FS_HZ          16000u

/** Stereo frames processed per DMA half-buffer (128 frames = 8 ms at 16 kHz). */
#define AUDIO_FRAMES_PER_HALF 128u

/** NLMS filter length. 32 taps cover 2 ms of acoustic delay at 16 kHz. */
#define AUDIO_NLMS_TAPS      32u

/** NLMS step size (0 < mu < 2). */
#define AUDIO_NLMS_MU        0.05f

/** NLMS regularization constant. */
#define AUDIO_NLMS_EPS       1.0e-6f

/** Pole of the DC-blocking filter applied to both mic signals. */
#define AUDIO_DC_POLE        0.995f

/** Linear gain applied to the cleaned signal before the DAC. */
#define AUDIO_OUTPUT_GAIN    4.0f

/* ------------------------------------------------------------------------ */

/**
 * @brief Start the audio path. Call once after MX_I2S2_Init().
 * @param hi2s I2S2 handle in full-duplex master mode. Its receive line (I2S2ext)
 *             reads both INMP441 mics; its transmit line drives the Pmod I2S2 DAC.
 * @return HAL_OK on success.
 */
HAL_StatusTypeDef audio_app_start(I2S_HandleTypeDef *hi2s);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_APP_H */
