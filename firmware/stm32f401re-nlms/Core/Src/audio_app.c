/**
 * @file audio_app.c
 * @brief Real-time adaptive noise cancelling on the STM32F401RE.
 *
 * Signal path:
 *
 *   INMP441 primary   (L/R = GND, left slot)  --\
 *                                                 I2S2ext RX (PC2) -> DC block -> NLMS -> gain -> I2S2 TX (PC3) -> Pmod I2S2 DAC -> 3.5 mm jack
 *   INMP441 reference (L/R = 3V3, right slot) --/
 *
 * I2S2 runs in full-duplex master mode, so receive and transmit share the
 * same bit clock (PB10), word select (PB12) and master clock (PC6). Both
 * directions use 24-bit data in 32-bit frames (Philips standard).
 *
 * The DMA buffers are circular. Each half is processed in the matching DMA
 * callback while the other half is being transferred.
 */
#include "audio_app.h"

#include <string.h>

#include "nlms.h"

/* Each stereo frame is 2 channels x 32 bits = 4 half-words in the DMA buffer. */
#define HALFWORDS_PER_FRAME 4u
#define HALF_LEN            (AUDIO_FRAMES_PER_HALF * HALFWORDS_PER_FRAME)
#define BUF_LEN             (2u * HALF_LEN)

/* Full-scale value of a signed 24-bit sample. */
#define FULL_SCALE_24       8388608.0f

static uint16_t rx_buf[BUF_LEN];
static uint16_t tx_buf[BUF_LEN];

static nlms_t anc;

/* DC-blocker state: previous input and previous output for each mic. */
static float dc_x1[2];
static float dc_y1[2];

/**
 * @brief Convert two DMA half-words (MSB half first) into a float in [-1, 1).
 *
 * The 24-bit sample sits in bits 31..8 of the 32-bit slot, so an arithmetic
 * shift right by 8 gives the signed 24-bit value.
 */
static inline float slot_to_float(uint16_t hi, uint16_t lo)
{
    const int32_t raw = (int32_t)(((uint32_t)hi << 16) | lo);
    return (float)(raw >> 8) / FULL_SCALE_24;
}

/** @brief Convert a float in [-1, 1] into two DMA half-words (MSB half first). */
static inline void float_to_slot(float v, uint16_t *hi, uint16_t *lo)
{
    if (v > 0.999999f) {
        v = 0.999999f;
    } else if (v < -1.0f) {
        v = -1.0f;
    }
    /* Shift as unsigned: left-shifting a negative signed value is undefined in C. */
    const uint32_t raw = (uint32_t)(int32_t)(v * FULL_SCALE_24) << 8;
    *hi = (uint16_t)(raw >> 16);
    *lo = (uint16_t)(raw & 0xFFFFu);
}

/** @brief First-order DC blocker: y(n) = x(n) - x(n-1) + p * y(n-1). */
static inline float dc_block(uint32_t ch, float x)
{
    const float y = x - dc_x1[ch] + AUDIO_DC_POLE * dc_y1[ch];
    dc_x1[ch] = x;
    dc_y1[ch] = y;
    return y;
}

/** @brief Run the noise canceller on one half of the buffers. */
static void process_half(uint32_t offset)
{
    const uint16_t *in = &rx_buf[offset];
    uint16_t *out = &tx_buf[offset];

    for (uint32_t i = 0; i < AUDIO_FRAMES_PER_HALF; i++) {
        const uint32_t k = i * HALFWORDS_PER_FRAME;

        const float primary   = dc_block(0, slot_to_float(in[k + 0], in[k + 1]));
        const float reference = dc_block(1, slot_to_float(in[k + 2], in[k + 3]));

        const float cleaned = AUDIO_OUTPUT_GAIN * nlms_process(&anc, primary, reference);

        /* Same signal to both ears. */
        float_to_slot(cleaned, &out[k + 0], &out[k + 1]);
        out[k + 2] = out[k + 0];
        out[k + 3] = out[k + 1];
    }
}

HAL_StatusTypeDef audio_app_start(I2S_HandleTypeDef *hi2s)
{
    nlms_init(&anc, AUDIO_NLMS_TAPS, AUDIO_NLMS_MU, AUDIO_NLMS_EPS);
    memset(dc_x1, 0, sizeof(dc_x1));
    memset(dc_y1, 0, sizeof(dc_y1));
    memset(tx_buf, 0, sizeof(tx_buf));

    /* For 24/32-bit formats the HAL size argument counts 32-bit samples,
     * which is half the number of half-words in each buffer. */
    return HAL_I2SEx_TransmitReceive_DMA(hi2s, tx_buf, rx_buf, BUF_LEN / 2u);
}

/* ---- HAL DMA callbacks ------------------------------------------------- */

void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        process_half(0u);
    }
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2) {
        process_half(HALF_LEN);
    }
}
