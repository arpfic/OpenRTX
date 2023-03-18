/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef STM32_PWM_H
#define STM32_PWM_H

#include <stdbool.h>
#include <stdint.h>
#include <stm32f4xx.h>
#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Driver STM32 PWM used as audio output stream device.
 * Input data format is signed 16-bit, internally converted to unsigned 8-bit
 * values for compatibility with the hardware.
 *
 * The driver uses the following peripherals: DMA1_Stream2, TIM7
 */

/**
 * Data structure holding the configuration for a given DAC channel.
 */
struct PwmChannelCfg
{
    volatile uint32_t *pwmReg;  ///< Address of PWM duty cycle register.
    void (*startCbk)(void);     ///< Callback function for additional setup operations.
    void (*stopCbk)(void);      ///< Callback function for additional end operations.
};


extern const struct audioDriver stm32_pwm_audio_driver;


/**
 * Initialize the driver and the peripherals.
 */
void stm32pwm_init();

/**
 * Shutdown the driver and the peripherals.
 */
void stm32pwm_terminate();

/**
 * Start sending an audio stream from a DAC channel.
 *
 * @param instance: driver instance.
 * @param cfg: pointer to the configuration of the output channel.
 * @param ctx: pointer to the audio stream descriptor.
 * @return -1 if channel is already in use, zero otherwise.
 */
int stm32pwm_start(const uint8_t instance, const void *cfg, struct streamCtx *ctx);

/**
 * Get a pointer to the section of the sample buffer not currently being read
 * by the DMA peripheral. To be used primarily when the output stream is running
 * in double-buffered circular mode for filling a new block of data to the stream.
 *
 * @param ctx: pointer to the audio stream descriptor.
 * @param buf: pointer to a stream_sample_t * variable which will point to the
 * idle section of the data buffer.
 * @return size of the idle data buffer or -1 in case of errors.
 */
int stm32pwm_idleBuf(struct streamCtx *ctx, stream_sample_t **buf);

/**
 * Synchronise with the output stream DMA transfer, blocking function.
 * When the stream is running in circular mode, execution is blocked until
 * either the half or the end of the buffer is reached. In linear mode execution
 * is blocked until the end of the buffer is reached.
 * If the stream is not running or there is another thread waiting at the
 * synchronisation point, the function returns immediately.
 *
 * @param ctx: pointer to the audio stream descriptor.
 * @param dirty: set to 1 if the idle data buffer has been filled with new data.
 * @return -1 in case of errors, zero otherwise.
 */
int stm32pwm_sync(struct streamCtx *ctx, uint8_t dirty);

/**
 * Request termination of a currently ongoing output stream.
 * Stream is effectively stopped only when all the remaining data are sent.
 *
 * @param ctx: pointer to the audio stream descriptor.
 */
void stm32pwm_stop(struct streamCtx *ctx);

/**
 * Immediately stop a currently ongoing output stream before its natural ending.
 *
 * @param ctx: pointer to the audio stream descriptor.
 */
void stm32pwm_halt(struct streamCtx *ctx);


#ifdef __cplusplus
}
#endif

#endif /* STM32_PWM_H */
