
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "msp432_boostxl_init.h"
#include "msp432_arm_dsp.h"

typedef enum {IDLE, RECORDING, PLAYBACK_1, PLAYBACK_2, PLAYBACK_3} audiostate_t;

#define BUFSIZE 16384

int8_t    buffer[BUFSIZE];
uint32_t  bufindex  = 0;
audiostate_t pbmode = PLAYBACK_1;

audiostate_t next_state(audiostate_t current) {
    audiostate_t next;

    next = current;    // by default, maintain the state

    switch (current) {
    case IDLE:
        colorledoff();
        errorledoff();
        bufindex = 0;
        if (pushButtonLeftDown())
            next = RECORDING;
        else if (pushButtonRightDown())
            next = pbmode;
        break;

    case RECORDING:
        errorledon();
        bufindex++;
        if (bufindex == BUFSIZE) {
            bufindex = 0;
            next = IDLE;
        }
        break;

    case PLAYBACK_1:
        colorledgreen();
        bufindex++;
        if (bufindex == BUFSIZE) {
            bufindex = 0;
            if (pushButtonRightUp()) {
                pbmode = PLAYBACK_2;
                next = IDLE;
            }
        }
        break;

    case PLAYBACK_2:
        colorledblue();
        bufindex++;
        bufindex++;
        if (bufindex == BUFSIZE) {
            bufindex = 0;
            if (pushButtonRightUp()) {
                pbmode = PLAYBACK_3;
                next = IDLE;
            }
        }
        break;

    case PLAYBACK_3:
        colorledred();
        bufindex = (bufindex + BUFSIZE - 1) % BUFSIZE; // play backward
        if (bufindex == 0) {
            if (pushButtonRightUp()) {
                pbmode = PLAYBACK_1;
                next = IDLE;
            }
        }
        break;

    default:
        next = IDLE;
        break;
    }

    return next;
}

audiostate_t glbAudioState = IDLE;

uint16_t processSample(uint16_t x) {
    uint16_t y;

    glbAudioState = next_state(glbAudioState);         // called for every new sample @16KHz

    switch (glbAudioState) {
    case RECORDING:
             buffer[bufindex] = adc14_to_q15(x) >> 6;  // only keep upper byte to save storage
             y = q15_to_dac14(0);                      // keep silent during recording
             break;
    case PLAYBACK_1:
    case PLAYBACK_2:
    case PLAYBACK_3:
             y = q15_to_dac14(((int16_t) buffer[bufindex]) << 6);
             break;
    default:
             y = q15_to_dac14(0);                     // keep silent in IDLE
             break;
    }
    return y;
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    msp432_boostxl_init_intr(FS_16000_HZ, BOOSTXL_MIC_IN, processSample);
    msp432_boostxl_run();

    return 1;
}
