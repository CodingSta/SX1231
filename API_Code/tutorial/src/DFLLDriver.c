/*******************************************************************
** File        : DFLLDriver.c                                     **
********************************************************************
**                                                                **
** Version     : V 1.0                                            **
**                                                                **
** Written by   : Miguel Luis                                     **
**                                                                **
** Date        : 14-02-2002                                       **
**                                                                **
** Project     : -                                                **
**                                                                **
********************************************************************
** Changes     : V 1.1 - Add the 2 following lines:               **
**                       StaticRegCntOn = RegCntOn;               **
**                       RegCntOn = 0;                            **
**               V 1.2 - Add the instruction "FREQ div2" and      **
**                       "Freq nodiv"                             **
********************************************************************
** Description : DFLL (Digital Frequency Locked Loop)             **
**               This DFLL implementation is precise at 2%        **
**               The tests were made at ambient temperature       **
**               (See Excel sheet for more information)           **
** Code Size :                                                    **
** Tested version Optimization None                               **
** Optimization None    : DFLLDriver.c    254 instructions        **
**                                                                **
** Optimization Level 1 : DFLLDriver.c    182 instructions        **
**                                                                **
** Optimization Level 2 : DFLLDriver.c    185 instructions        **
**                                                                **
** Optimization Level 3 : DFLLDriver.c    230 instructions        **
*******************************************************************/
#include "DFLLDriver.h"

/*******************************************************************
** Global variables definitions                                   **
*******************************************************************/
static _U8 StaticRegCntOn, StaticRegCntA, StaticRegCntB,
StaticRegCntC, StaticRegCntD, StaticRegCntCtrlCk, StaticRegCntConfig1,
StaticRegCntConfig2;

/*******************************************************************
** CalibrateRC : Performs the DFLL                                **
********************************************************************
** In          : NbRCtoReach                                      **
** Out         : -                                                **
********************************************************************
** NbRCtoReach is the value that the measurement counter needs to **
** reach to get the given frequency generated by the RC oscillator**
*******************************************************************/
extern void CalibrateRC (_U16 NbRCtoReach);

/*******************************************************************
** DFLLRun : Makes measurement of RC oscillator until the wished  **
**           frequency is found                                   **
********************************************************************
** In      : Frequency (in Hz)                                    **
** Out     : -                                                    **
*******************************************************************/
void DFLLRun (_U32 Frequency){
    _U8 RegEvnEn_old;
    _U16 NbRCtoReach;
    _U8 SaveStatReg;

    // Saves the configuration of Counters
    StaticRegCntOn = RegCntOn;
    RegCntOn = 0;
    StaticRegCntCtrlCk  = RegCntCtrlCk;
    StaticRegCntConfig1 = RegCntConfig1;
    StaticRegCntConfig2 = RegCntConfig2;
    StaticRegCntA = RegCntA;
    StaticRegCntB = RegCntB;
    StaticRegCntC = RegCntC;
    StaticRegCntD = RegCntD;

    //Save the Status register of CR816
    asm("move %0, %%stat"::"m" (SaveStatReg));

    // Save the current of the Enabled events
    RegEvnEn_old = RegEvnEn;
/**/
    // Calculates the value that the counter A should reach
    // NbQuartz = 512
    // NbRCtoReach = (Frequency /32768) * 512
    NbRCtoReach = Frequency >> 6;
/**/
/** /
    // Calculates the value that the counter A should reach
    // NbQuartz = 64
    // NbRCtoReach = (Frequency /32768) * 64
    NbRCtoReach = Frequency >> 9;
/**/
    asm("move %stat, #0");

    CalibrateRC(NbRCtoReach);

    //Restore the Status register of CR816
    asm("move %%stat, %0"::"m" (SaveStatReg));

    // Restore the old settings for events
    RegEvnEn = RegEvnEn_old;

    // Restore the old settings of counters
    RegCntOn = StaticRegCntOn;
    RegCntCtrlCk = StaticRegCntCtrlCk;
    RegCntConfig1 = StaticRegCntConfig1;
    RegCntConfig2 = StaticRegCntConfig2;
    RegCntA = StaticRegCntA;
    RegCntB = StaticRegCntB;
    RegCntC = StaticRegCntC;
    RegCntD = StaticRegCntD;
}

/*******************************************************************
** FrequencyMeasure : Measure the frequency of RC oscillator      **
********************************************************************
** In               : -                                           **
** Out              : CounterValue                                **
*******************************************************************/
_U16 FrequencyMeasure(void){
    // Enable event on Counter C
    RegEvnEn = 0x40;
    // A counts up to 0xFF and C counts down from Quartz
    RegCntA = 0xFF;
    RegCntB = 0xFF;
/**/    
    // NbQuartz = 512
    RegCntC = 0x00;
    RegCntD = 0x02;
/**/
/** /    
    // NbQuartz = 64
    RegCntC = 0x40;
    RegCntD = 0x00;
/**/    
    // Start Counters C
    RegCntOn = 0x04;
    // Put the CPU in HALT mode
    asm("HALT");
    // Start Counters A
    RegCntOn = 0x05;
    // Clear event on Counter C
    RegEvn = 0x40;
    // Clear event in stat register
    asm("clrb %stat, #0");
    // Put the CPU in HALT mode
    // In order to synchronize both counters on a
    // falling edge of the Xtal oscillator
    asm("HALT");
    // Stop Counters A and C
    RegCntOn = 0x00;
    // Clear event on Counter C
    RegEvn = 0x40;
    // Clear event in stat register
    asm("clrb %stat, #0");
    // Disable event on Counter C
    RegEvnEn = 0x00;

    return ((RegCntB << 8) | RegCntA);
}

/*******************************************************************
** RCSetDelay : Generates a delay to get the RC osc. stabilized   **
**              (8 RC clock cycles)                               **
********************************************************************
** In         : -                                                 **
** Out        : -                                                 **
*******************************************************************/
void RCSetDelay(void){
	asm("nop");
	asm("nop");
	asm("nop");
	asm("nop");
}

/*******************************************************************
** CalibrateRC : Performs the DFLL                                **
********************************************************************
** In          : NbRCtoReach                                      **
** Out         : -                                                **
********************************************************************
** NbRCtoReach is the value that the measurement counter needs to **
** reach to get the given frequency generated by the RC oscillator**
*******************************************************************/
void CalibrateRC (_U16 NbRCtoReach){
    _U16 NewCounter = 0;
    _U16 Diff = 0;
    _U16 BitPos;

	asm("freq div2");
    // RC oscillator initialization
    RegSysRcTrim1 = 0x10;
    RegSysRcTrim2 = 0;
    // Counters initialization
    RegCntCtrlCk  = 0x11;
    RegCntConfig1 = 0x1C;
    RegCntConfig2 = 0x00;
    
    for(BitPos = 0x400; BitPos != 0x00; BitPos >>= 1){
        RegSysRcTrim1 |= (_U8)(BitPos >> 6);
        RegSysRcTrim2 |= (_U8)(BitPos & 0x3F);

        RCSetDelay();
        NewCounter = FrequencyMeasure();
        if(NewCounter > NbRCtoReach){
            RegSysRcTrim1 &= (_U8)(~BitPos >> 6);
            RegSysRcTrim2 &= (_U8)(~(BitPos & 0x3F));
        } 
    }    
    
    Diff = NbRCtoReach - NewCounter;
	if (Diff > 0){
		RegSysRcTrim2++;    
    }
    else{
        RegSysRcTrim2--;    
    }
    
    RCSetDelay();
    NewCounter = FrequencyMeasure();
    
    if(abs(NbRCtoReach - NewCounter) > abs(Diff)){
	    if (Diff > 0){
		    RegSysRcTrim2--;    
	    }
	    else{
	        RegSysRcTrim2++;    
	    }
    }
    asm("freq nodiv");
}

