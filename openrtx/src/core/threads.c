/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <hwconfig.h>
#include <pthread.h>
#include <sched.h>
#include <ui.h>
#include <state.h>
#include <threads.h>
#include <graphics.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <event.h>
#include <rtx.h>
#include <string.h>
#include <utils.h>
#include <input.h>
#include <backup.h>
#ifdef GPS_PRESENT
#include <interfaces/gps.h>
#include <gps.h>
#endif
#include <voicePrompts.h>


/* Mutex for concurrent access to RTX state variable */
pthread_mutex_t rtx_mutex;

/**
 * \internal Thread managing user input and UI
 */
void *ui_threadFunc(void *arg)
{
    (void) arg;

    kbd_msg_t   kbd_msg;
    rtxStatus_t rtx_cfg;
    bool        sync_rtx = true;
    long long   time     = 0;

    // Load initial state and update the UI
    ui_saveState();
    ui_updateGUI();

    // Keep the splash screen for one second  before rendering the new UI screen
    sleepFor(1u, 0u);
    gfx_render();

    while(state.devStatus != SHUTDOWN)
    {
        time = getTick();

        if(input_scanKeyboard(&kbd_msg))
        {
            ui_pushEvent(EVENT_KBD, kbd_msg.value);
        }

        pthread_mutex_lock(&state_mutex);   // Lock r/w access to radio state
        ui_updateFSM(&sync_rtx);            // Update UI FSM
        ui_saveState();                     // Save local state copy
        pthread_mutex_unlock(&state_mutex); // Unlock r/w access to radio state

        vp_tick();                           // continue playing voice prompts in progress if any.

        // If synchronization needed take mutex and update RTX configuration
        if(sync_rtx)
        {
            float power = dBmToWatt(state.channel.power);

            pthread_mutex_lock(&rtx_mutex);
            rtx_cfg.opMode      = state.channel.mode;
            rtx_cfg.bandwidth   = state.channel.bandwidth;
            rtx_cfg.rxFrequency = state.channel.rx_frequency;
            rtx_cfg.txFrequency = state.channel.tx_frequency;
            rtx_cfg.txPower     = power;
            rtx_cfg.sqlLevel    = state.settings.sqlLevel;
            rtx_cfg.rxToneEn    = state.channel.fm.rxToneEn;
            rtx_cfg.rxTone      = ctcss_tone[state.channel.fm.rxTone];
            rtx_cfg.txToneEn    = state.channel.fm.txToneEn;
            rtx_cfg.txTone      = ctcss_tone[state.channel.fm.txTone];

            // Copy new M17 CAN, source and destination addresses
            rtx_cfg.can = state.settings.m17_can;
            strncpy(rtx_cfg.source_address,      state.settings.callsign, 10);
            strncpy(rtx_cfg.destination_address, state.m17_dest, 10);

            pthread_mutex_unlock(&rtx_mutex);

            rtx_configure(&rtx_cfg);
            sync_rtx = false;
        }

        // Update UI and render on screen, if necessary
        if(ui_updateGUI() == true)
        {
            gfx_render();
        }

        // 40Hz update rate for keyboard and UI
        time += 25;
        sleepUntil(time);
    }

    ui_terminate();
    gfx_terminate();

    return NULL;
}

/**
 * \internal Thread managing the device and update the global state variable.
 */
void *main_thread(void *arg)
{
    (void) arg;

    long long time     = 0;

    while(state.devStatus != SHUTDOWN)
    {
        time = getTick();

        // Check if power off is requested
        pthread_mutex_lock(&state_mutex);
        if(platform_pwrButtonStatus() == false)
            state.devStatus = SHUTDOWN;
        pthread_mutex_unlock(&state_mutex);

        // Handle external flash backup/restore
        #if !defined(PLATFORM_LINUX) && !defined(PLATFORM_MOD17)
        if(state.backup_eflash)
        {
            eflash_dump();

            pthread_mutex_lock(&state_mutex);
            state.backup_eflash = false;
            state.devStatus     = SHUTDOWN;
            pthread_mutex_unlock(&state_mutex);
        }

        if(state.restore_eflash)
        {
            eflash_restore();

            pthread_mutex_lock(&state_mutex);
            state.restore_eflash = false;
            state.devStatus      = SHUTDOWN;
            pthread_mutex_unlock(&state_mutex);
        }
        #endif

        // Run GPS task
        #if defined(GPS_PRESENT) && !defined(MD3x0_ENABLE_DBG)
        gps_task();
        #endif

        // Run state update task
        state_task();

        // Run this loop once every 5ms
        time += 5;
        sleepUntil(time);
    }

    #if defined(GPS_PRESENT)
    gps_terminate();
    #endif

    return NULL;
}

/**
 * \internal Thread for RTX management.
 */
void *rtx_threadFunc(void *arg)
{
    (void) arg;

    rtx_init(&rtx_mutex);

    while(state.devStatus == RUNNING)
    {
        rtx_task();
    }

    rtx_terminate();

    return NULL;
}

/**
 * \internal This function creates all the system tasks and mutexes.
 */
void create_threads()
{
    // Create RTX state mutex
    pthread_mutex_init(&rtx_mutex, NULL);

    // Create rtx radio thread
    pthread_t      rtx_thread;
    pthread_attr_t rtx_attr;

    pthread_attr_init(&rtx_attr);
    pthread_attr_setstacksize(&rtx_attr, RTX_TASK_STKSIZE);

    #ifdef _MIOSIX
    // Max priority for RTX thread when running with miosix rtos
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(0);
    pthread_attr_setschedparam(&rtx_attr, &param);
    #endif

    pthread_create(&rtx_thread, &rtx_attr, rtx_threadFunc, NULL);

    // Create UI thread
    pthread_t      ui_thread;
    pthread_attr_t ui_attr;

    pthread_attr_init(&ui_attr);
    pthread_attr_setstacksize(&ui_attr, UI_TASK_STKSIZE);
    pthread_create(&ui_thread, &ui_attr, ui_threadFunc, NULL);
}
