/****************************************************************************
 * FILE NAME: etec_spi_master.c                                             *
 * DESCRIPTION:                                                             *
 * This function uses 3 channels, and optionally slave select               *
 * channels/outputs, to form a uni or bi directional synchronous serial     *
 * port that can be used to communicate with a wide variety of devices.     *                                                 *
 ****************************************************************************/

#include <ETpu_Std.h>

/* SPI Definitions */
/* Host Service Requests */
#define  SPI_MASTER_INIT_TCR1_HSR      7
#define  SPI_MASTER_INIT_TCR2_HSR      5
#define  SPI_MASTER_RUN_HSR            3
/* Function Modes */
#define  SPI_MASTER_CPHA_0_FM0         0
#define  SPI_MASTER_CPHA_1_FM0         1
#define  SPI_MASTER_SHIFT_DIR_MSB_FM1  0
#define  SPI_MASTER_SHIFT_DIR_LSB_FM1  1
/* other definitions */
#define  SPI_MASTER_MAX_SLAVE_SELECT_CNT 4

/***********************************/
/* Verify performance requirements */
/***********************************/
#pragma verify_wctl  SPI_master                 35  steps  10 rams
#pragma exclude_wctl SPI_master::InitTCR1
#pragma exclude_wctl SPI_master::InitTCR2

/* provide hint that channel frame base addr same on all chans touched by func */
#pragma same_channel_frame_base SPI_master

/*
   MISO     - channel - 1
   SCLK     - channel
   MOSI     - channel + 1
   SS       - slave_select_chan [optional]
*/

#if 0
/* beyond ETEC 2.62D, the below will need to be removed */
typedef int8            int8_t;
typedef int16           int16_t;
typedef int24           int24_t;
typedef int32           int32_t;
typedef unsigned int8   uint8_t;
typedef unsigned int16  uint16_t;
typedef unsigned int24  uint24_t;
typedef unsigned int32  uint32_t;
#endif

_eTPU_class SPI_master
{
    /* channel frame */
public:
    int24_t     _half_period;
    _Bool       _CPOL;
    /*_Bool       _CPHA;*/      /* held in FM0 */
    /*_Bool       _LSB_first;*/ /* held in FM1 */
    int8_t      _bit_count;
    uint24_t    _data_out_reg;
    uint24_t    _data_in_reg;

    uint8_t     _slave_select_chan_list[SPI_MASTER_MAX_SLAVE_SELECT_CNT];
    uint8_t     _slave_select_chan;
    int24_t     _slave_select_delay;

private:
    int8_t      _bit_count_current;
    uint24_t    _data_out_shift_reg;
    uint24_t    _data_in_shift_reg;
    uint8_t     _slave_select_end;

    /* threads */
    
    /* initialization */
    _eTPU_thread InitTCR1(_eTPU_matches_disabled);
    _eTPU_thread InitTCR2(_eTPU_matches_disabled);

    /* trigger word transmit */
    _eTPU_thread RunTCR1(_eTPU_matches_disabled);
    _eTPU_thread RunTCR2(_eTPU_matches_disabled);

    /* SCLK working threads */
    _eTPU_thread ClockLeadingLSB(_eTPU_matches_enabled);
    _eTPU_thread ClockLeadingMSB(_eTPU_matches_enabled);
    _eTPU_thread ClockTrailingLSB(_eTPU_matches_enabled);
    _eTPU_thread ClockTrailingMSB(_eTPU_matches_enabled);

    /* fragments */
    _eTPU_fragment CommonInit();
    _eTPU_fragment CommonRun();
    _eTPU_fragment SetTrailingEdge();
    _eTPU_fragment WriteData_CPHA0();
    _eTPU_fragment ReadData_CPHA1();
    _eTPU_fragment FinishWord();
    
    /* methods */
    /* none */

    /* entry table(s) */
    _eTPU_entry_table SPI_master;
};

DEFINE_ENTRY_TABLE(SPI_master, SPI_master, alternate, outputpin, autocfsr)
{
	/*           HSR    LSR M1 M2 PIN F0 F1 vector */
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  0, x, RunTCR1),
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  1, x, RunTCR2),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  0, x, RunTCR1),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  1, x, RunTCR2),
	ETPU_VECTOR3(1,4,5, x,  x, x, x,  x, x, InitTCR2),
	ETPU_VECTOR2(6,7,   x,  x, x, x,  x, x, InitTCR1),
	ETPU_VECTOR1(0,     1,  0, 0, 0,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     1,  0, 0, 1,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, ClockLeadingLSB),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, ClockLeadingLSB),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, ClockLeadingMSB),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, ClockLeadingMSB),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, ClockLeadingLSB),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, ClockLeadingLSB),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, ClockLeadingMSB),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, ClockLeadingMSB),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, ClockTrailingLSB),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, ClockTrailingLSB),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, ClockTrailingMSB),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, ClockTrailingMSB),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, ClockTrailingLSB),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, ClockTrailingLSB),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, ClockTrailingMSB),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, ClockTrailingMSB),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, _Error_handler_entry),
};


_eTPU_thread SPI_master::InitTCR1(_eTPU_matches_disabled)
{
    /* SET UP TO USE TCR1 */
    channel.TBSA = TBS_M1C1GE;
    channel.TBSB = TBS_M1C1GE;
    channel.FLAG0 = 0;
    
    CommonInit();
}

_eTPU_fragment SPI_master::CommonInit()
{
    uint24_t i;

    channel.PDCM = PDCM_EM_NB_ST;

    /* SET FUNCTION MODE SHIFT DIRECTION AS FLAG 1 */
    channel.FLAG1 = 1;      /* SHIFT MSB FIRST : default*/
    if (channel.FM1 == SPI_MASTER_SHIFT_DIR_LSB_FM1)
    {
        channel.FLAG1 = 0;      /* SHIFT LSB FIRST */
    }

    /* clear all latches */
    channel.LSR = LSR_CLEAR;
    channel.MRLA = MRL_CLEAR;
    channel.MRLB = MRL_CLEAR;
    channel.TDL = TDL_CLEAR;

    /* SET PIN HIGH OR LOW ON INITIALIZATION */
    if (_CPOL == 0) /* POLARITY = 0 SO +VE 1ST EDGE */
    {
        channel.PIN = PIN_SET_LOW; /* Ready for 1st -ve edge */
        channel.OPACA = OPAC_MATCH_HIGH;
        channel.OPACB = OPAC_MATCH_LOW;
    }
    else            /* POLARITY = 1 SO -VE 1ST EDGE */
    {
        channel.PIN = PIN_SET_HIGH; /* Ready for first +ve edge */
        channel.OPACA = OPAC_MATCH_LOW;
        channel.OPACB = OPAC_MATCH_HIGH;
    }

    /* Enable Output Buffer - for Puma */
    channel.TBSA = TBSA_SET_OBE;

    /* ENABLE EVENT HANDLING */
    channel.MTD = MTD_ENABLE;

    /* turn off outout buffer on MISO chan */
    chan -= 1;
    channel.TBSA = TBSA_CLR_OBE;
    
    /* initialize any slave select outputs */
    for (i = 0; i < SPI_MASTER_MAX_SLAVE_SELECT_CNT; i++)
    {
        if (_slave_select_chan_list[i] != 0xff)
        {
            chan = _slave_select_chan_list[i];
            channel.TBSA = TBSA_SET_OBE;
            channel.PIN = PIN_SET_HIGH;
        }
    }
}

_eTPU_thread SPI_master::InitTCR2(_eTPU_matches_disabled)
{
    /* SET UP TO USE TCR2 */
    channel.TBSA = TBS_M2C2GE;
    channel.TBSB = TBS_M2C2GE;
    channel.FLAG0 = 1;

    CommonInit();
}

_eTPU_thread SPI_master::RunTCR1(_eTPU_matches_disabled)
{
    erta = tcr1;
    
    CommonRun();
}

_eTPU_fragment SPI_master::CommonRun()
{
    /* clear all latches */
    channel.LSR = LSR_CLEAR;
    channel.MRLA = MRL_CLEAR;
    channel.MRLB = MRL_CLEAR;
    channel.TDL = TDL_CLEAR;

    if (_slave_select_chan != 0xff)
    {
        uint8_t tmp;
        
        _slave_select_end = 0;
        erta += _slave_select_delay;
        channel.ERWA = ERW_WRITE_ERT_TO_MATCH;

        tmp = chan;
        chan = _slave_select_chan;
        channel.PIN = PIN_SET_LOW;
        chan = tmp;
    }
    else
    {
        erta += _half_period;
        channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
    }

    _data_out_shift_reg = _data_out_reg;
     if (channel.FM0 == SPI_MASTER_CPHA_0_FM0)
    {
        /* move to MOSI channel */
        chan += 1;

        /* Enable Output Buffer - for Puma */
        channel.TBSA = TBSA_SET_OBE;
        if (channel.FM1 == SPI_MASTER_SHIFT_DIR_LSB_FM1)
        {
            _data_out_shift_reg  >>= 1;       /* SHIFT LSB FIRST */
        }
        else
        {
            _data_out_shift_reg  <<= 1;       /* SHIFT MSB FIRST */
        }

        /* PLACE data_out ON OUTPUT CHANNEL FOR ACCESS ON 1ST CLK EDGE */
        if (CC.C != 0)
        {
            channel.PIN = PIN_SET_HIGH;
        }
        else
        {
            channel.PIN = PIN_SET_LOW;
        }
    }

    _bit_count_current = _bit_count;  /* RECORD BIT_COUNT AS BIT_COUNT_CURRENT FOR CALCULATIONS */
}

_eTPU_thread SPI_master::RunTCR2(_eTPU_matches_disabled)
{
    erta = tcr2;
    
    CommonRun();
}

_eTPU_thread SPI_master::ClockLeadingLSB(_eTPU_matches_enabled)
{
    channel.MRLA = MRL_CLEAR;
    
    if (channel.FM0 == SPI_MASTER_CPHA_0_FM0)
    {
        /* CODE TO READ INPUT PIN - AND ADD DATA TO DATA_REG */
        /* RECEIVE DATA  CHANNEL IS CHANNEL BELOW CLOCK */
        chan -= 1;
        _data_in_shift_reg >>= 1;
        if (channel.PSTI == 1)
        {
            _data_in_shift_reg += 0x800000;
        }

        SetTrailingEdge();
    }
    else
    {
        /* setup trailing clock edge first */
        ertb = erta + _half_period;    /* update ertb for next match B */
        channel.ERWB = ERW_WRITE_ERT_TO_MATCH;
        
        _bit_count_current -= 1;

        /* put data out on this edge */
        chan += 1;
        _data_out_shift_reg >>= 1;

        if (CC.C != 0)
        {
            channel.PIN = PIN_SET_HIGH;
        }
        else
        {
            channel.PIN = PIN_SET_LOW;
        }
    }
}

_eTPU_fragment SPI_master::SetTrailingEdge()
{
    chan += 1;

    /* CODE TO TOGGLE THE CLOCK PIN AT THE NEXT EDGE/SET UP THE NEXT MATCH */
    ertb = erta + _half_period;    /* update ertb for next match B */
    channel.ERWB = ERW_WRITE_ERT_TO_MATCH;

    _bit_count_current -= 1;
}

_eTPU_thread SPI_master::ClockLeadingMSB(_eTPU_matches_enabled)
{
    channel.MRLA = MRL_CLEAR;

    if (channel.FM0 == SPI_MASTER_CPHA_0_FM0)
    {
        /* CODE TO READ INPUT PIN - AND ADD DATA TO DATA_REG */
        /* DATA IN CHANNEL IS CHANNEL BELOW CLOCK */
        chan -= 1;
        _data_in_shift_reg <<= 1;
        if (channel.PSTI == 1)
        {
            _data_in_shift_reg += 1;
        }

        SetTrailingEdge();
    }
    else
    {
        /* setup trailing clock edge first */
        ertb = erta + _half_period;    /* update ertb for next match B */
        channel.ERWB = ERW_WRITE_ERT_TO_MATCH;

        _bit_count_current -= 1;
        
        /* put data out on this edge */
        chan += 1;
        _data_out_shift_reg <<= 1;

        if (CC.C != 0)
        {
            channel.PIN = PIN_SET_HIGH;
        }
        else
        {
            channel.PIN = PIN_SET_LOW;
        }
    }
}

_eTPU_thread SPI_master::ClockTrailingLSB(_eTPU_matches_enabled)
{
    channel.MRLB = MRL_CLEAR;

    if (_slave_select_end == 1)
    {
        _slave_select_end = 0;
        /* this only occurs on a final match to disable slave select */
        chan = _slave_select_chan;
        channel.PIN = PIN_SET_HIGH;
        channel.CIRC = CIRC_INT_FROM_SERVICED;
        channel.CIRC = CIRC_DATA_FROM_SERVICED;
        return;
    }

    if (channel.FM0 == SPI_MASTER_CPHA_0_FM0)
    {
        if (_bit_count_current != 0)
        {
            /* PUT data_out ON DATA OUT PIN */
            chan += 1;
            _data_out_shift_reg >>= 1;

            WriteData_CPHA0();
        }
        else
        {
            FinishWord();
        }
    }
    else
    {
        /* need to sample input */
        chan -= 1;
        _data_in_shift_reg >>= 1;
        if (channel.PSTI == 1)
        {
            _data_in_shift_reg += 0x800000;
        }

        ReadData_CPHA1();
    }
}

_eTPU_thread SPI_master::ClockTrailingMSB(_eTPU_matches_enabled)
{
    channel.MRLB = MRL_CLEAR;

    if (_slave_select_end == 1)
    {
        _slave_select_end = 0;
        /* this only occurs on a final match to disable slave select */
        chan = _slave_select_chan;
        channel.PIN = PIN_SET_HIGH;
        channel.CIRC = CIRC_INT_FROM_SERVICED;
        channel.CIRC = CIRC_DATA_FROM_SERVICED;
        return;
    }

    if (channel.FM0 == SPI_MASTER_CPHA_0_FM0)
    {
        if (_bit_count_current != 0)
        {
            /* PUT data_out ON DATA OUT PIN */

            chan += 1;
            _data_out_shift_reg <<= 1;

            WriteData_CPHA0();
        }
        else
        {
            FinishWord();
        }
    }
    else
    {
        /* need to sample input */
        chan -= 1;
        _data_in_shift_reg <<= 1;
        if (channel.PSTI == 1)
        {
            _data_in_shift_reg += 1;
        }

        ReadData_CPHA1();
    }
}

_eTPU_fragment SPI_master::WriteData_CPHA0()
{
    if (CC.C != 0)
    {
        channel.PIN = PIN_SET_HIGH;
    }
    else
    {
        channel.PIN = PIN_SET_LOW;
    }
    chan -= 1;
    erta = ertb + _half_period;      /* 2nd clock edge follows 1st */
    channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
}

_eTPU_fragment SPI_master::ReadData_CPHA1()
{
    /* set up next clock pulse if not done yet */        
    if (_bit_count_current != 0)
    {
        chan += 1;
        erta = ertb + _half_period;      /* 2nd clock edge follows 1st */
        channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
    }
    else
    {
        chan += 1;
        FinishWord();
    }
}

_eTPU_fragment SPI_master::FinishWord()
{
    _data_in_reg = _data_in_shift_reg;
    if (_slave_select_chan != 0xff)
    {
        /* set up to disable slave select - hold for half a bit */
        _slave_select_end = 1;
        ertb = ertb + _half_period;
        channel.ERWB = ERW_WRITE_ERT_TO_MATCH;
        return;
    }
    channel.CIRC = CIRC_INT_FROM_SERVICED;
    channel.CIRC = CIRC_DATA_FROM_SERVICED;
}


#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_INIT_TCR1_HSR", SPI_MASTER_INIT_TCR1_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_INIT_TCR2_HSR", SPI_MASTER_INIT_TCR2_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_RUN_HSR", SPI_MASTER_RUN_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_CPHA_0_FM0", SPI_MASTER_CPHA_0_FM0
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_CPHA_1_FM0", SPI_MASTER_CPHA_1_FM0
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_SHIFT_DIR_MSB_FM1", SPI_MASTER_SHIFT_DIR_MSB_FM1
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_SHIFT_DIR_LSB_FM1", SPI_MASTER_SHIFT_DIR_LSB_FM1
#pragma export_autodef_macro "FS_ETPU_SPI_MASTER_MAX_SLAVE_SELECT_CNT", SPI_MASTER_MAX_SLAVE_SELECT_CNT

/*********************************************************************
 *
 * ASH WARE has almost completely modifed and enhanced this eTPU function,
 * however a small amount of original code remains so the original
 * Freescale (NXP) copyright notice has been retained below.
 ********************************************************************/

/*********************************************************************
 *
 * Copyright:
 *  FREESCALE, INC. All Rights Reserved.
 *  You are hereby granted a copyright license to use, modify, and
 *  distribute the SOFTWARE so long as this entire notice is
 *  retained without alteration in any modified and/or redistributed
 *  versions, and that such modified versions are clearly identified
 *  as such. No licenses are granted by implication, estoppel or
 *  otherwise under any patents or trademarks of Motorola, Inc. This
 *  software is provided on an "AS IS" basis and without warranty.
 *
 *  To the maximum extent permitted by applicable law, FREESCALE
 *  DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED, INCLUDING
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
 *  PURPOSE AND ANY WARRANTY AGAINST INFRINGEMENT WITH REGARD TO THE
 *  SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF) AND ANY
 *  ACCOMPANYING WRITTEN MATERIALS.
 *
 *  To the maximum extent permitted by applicable law, IN NO EVENT
 *  SHALL FREESCALE BE LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING
 *  WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS
 *  INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY
 *  LOSS) ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
 *
 *  Freescale assumes no responsibility for the maintenance and support
 *  of this software
 ********************************************************************/
