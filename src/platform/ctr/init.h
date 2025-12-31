#pragma once

#include <3ds.h>
#include <cstdio>
#include <cstdlib>

namespace CTR 
{
    bool ctr_check_dsp()
    {
        FILE *dsp = fopen("sdmc:/3ds/dspfirm.cdc", "r");
        if (dsp == NULL) {
            gfxInitDefault();
            errorConf error;
            errorInit(&error, ERROR_TEXT, CFG_LANGUAGE_EN);
            errorText(&error, "Cannot find DSP firmware!\n\n\"sdmc:/3ds/dspfirm.cdc\"\n\nRun \'DSP1\' at least once to\ndump your DSP firmware.");
            errorDisp(&error);
            gfxExit();
            return false;
        }
        fclose(dsp);
        return true;
    }

    bool ctr_is_n3ds()
    {
        bool isN3DS;
        Result res = APT_CheckNew3DS(&isN3DS);
        return R_SUCCEEDED(res) && isN3DS;
    }

    void ctr_sys_init()
    {
        if (ctr_check_dsp() == false) {
            exit(0);
        }

        if (ctr_is_n3ds()) {
            osSetSpeedupEnable(true);
        }

        romfsInit();
        atexit([]() { 
            romfsExit(); 
        });

        acInit();
        atexit([]() {
            acExit();
        });
    }
}