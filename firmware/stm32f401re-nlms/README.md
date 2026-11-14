# STM32F401RE NLMS noise canceller

Firmware for the Nucleo-F401RE. It reads two INMP441 microphones over I2S,
removes the ambient noise with a normalized LMS filter, and plays the cleaned
signal through the Pmod I2S2 DAC to a 3.5 mm headphone jack.

| File | Purpose |
|---|---|
| `Core/Inc/nlms.h`, `Core/Src/nlms.c` | NLMS filter. No hardware dependencies. |
| `Core/Inc/audio_app.h`, `Core/Src/audio_app.c` | I2S DMA buffers, sample conversion, DC blocker, filter call. Tuning values are at the top of `audio_app.h`. |
| `test/test_nlms.c` | PC test of the filter on a synthetic thump in noise. |

## Create the project in STM32CubeIDE

1. New STM32 project, board **NUCLEO-F401RE**, initialize peripherals with their default mode: **No**.
2. **RCC**: HSE = BYPASS Clock Source (8 MHz from the ST-LINK).
3. **I2S2**:
   - Mode: Full-Duplex Master
   - Transmission Mode: Mode Master Transmit (the I2S2ext block receives)
   - Communication Standard: I2S Philips
   - Data and Frame Format: 24 Bits Data on 32 Bits Frame
   - Selected Audio Frequency: 16 kHz
   - Master Clock Output: Enabled (the Pmod I2S2 DAC needs MCLK)
   - DMA: SPI2_TX and I2S2_EXT_RX, both Circular, Half Word / Half Word
4. Set the I2S2 pins to PB10 (CK), PB12 (WS), PC6 (MCK), PC3 (SD) and PC2 (ext_SD).
5. **Clock Configuration**: system clock 84 MHz. Let CubeMX solve the PLLI2S settings for 16 kHz.
6. Generate code.
7. Copy the four files from `Core/` into the generated `Core/Inc` and `Core/Src` folders.
8. In `main.c`, add:

```c
/* USER CODE BEGIN Includes */
#include "audio_app.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 2 */
if (audio_app_start(&hi2s2) != HAL_OK) {
    Error_Handler();
}
/* USER CODE END 2 */
```

All processing runs in the DMA callbacks, so the main loop stays empty.

## Pins

I2S2 runs in full-duplex mode: one clock pair drives the microphones and the DAC.

| Signal | Nucleo pin | Connects to |
|---|---|---|
| I2S2_CK (bit clock) | PB10 | SCK on both INMP441, Pmod I2S2 pin 3 (D/A SCLK) |
| I2S2_WS (word select) | PB12 | WS on both INMP441, Pmod I2S2 pin 2 (D/A LRCK) |
| I2S2_MCK (master clock) | PC6 | Pmod I2S2 pin 1 (D/A MCLK) |
| I2S2_SD (data out) | PC3 | Pmod I2S2 pin 4 (D/A SDIN) |
| I2S2ext_SD (data in) | PC2 | SD on both INMP441 |
| 3V3 | CN6 3V3 | INMP441 VDD, Pmod I2S2 pin 6 (VCC) |
| GND | CN6 GND | INMP441 GND, Pmod I2S2 pin 5 (GND) |

The Pmod I2S2 line-in (A/D, bottom row) is not used.

Microphone channel select:

| Microphone | L/R pin | I2S slot | Role |
|---|---|---|---|
| Primary (pointed at the ground) | GND | Left | Thump plus noise, d(n) |
| Reference (pointed away from the ground) | 3V3 | Right | Noise only, x(n) |

Fit a 100 nF capacitor between VDD and GND at each microphone and a 100 kΩ
pull-down from each SD pin to GND. Plug the headphones into the Pmod I2S2
line-out jack (green, top row).

## Tuning

| Setting | Default | Effect |
|---|---|---|
| `AUDIO_NLMS_TAPS` | 32 | Longer filters can cancel noise with longer acoustic delays. 32 taps = 2 ms at 16 kHz. |
| `AUDIO_NLMS_MU` | 0.05 | Larger adapts faster to changing noise but leaves more residual noise. |
| `AUDIO_OUTPUT_GAIN` | 4.0 | Headphone level. Lower it if the output clips. |
| `AUDIO_DC_POLE` | 0.995 | DC blocker cutoff, about 13 Hz at 16 kHz. |

## Test the filter on a PC

```bash
cd test
cc -std=c99 -O2 -I../Core/Inc test_nlms.c ../Core/Src/nlms.c -lm -o test_nlms
./test_nlms
```

The test passes when the filter removes at least 20 dB of noise.
