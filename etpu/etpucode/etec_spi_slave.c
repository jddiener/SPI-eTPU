/****************************************************************************
 * Copyright (C) 2020 ASH WARE, Inc.
 ****************************************************************************/
/****************************************************************************
 * FILE NAME: etec_spi_slave.c                                              *
 * DESCRIPTION:                                                             *
 * This function uses 3 channels, and optionally a slave select channel     *
 * channels/outputs, to form a uni or bi directional synchronous serial     *
 * port.  This function provides the slave-side capability of SPI.          *
 ****************************************************************************/

#include <ETpu_Std.h>

/* SPI Slave Definitions */
/* Host Service Requests */
#define  SPI_SLAVE_INIT_HSR         1
#define  SPI_SLAVE_INIT_SS_HSR      2
#define  SPI_SLAVE_SET_DATA_HSR     7
/* Function Modes */
#define  SPI_SLAVE_CPHA_0_FM0         0
#define  SPI_SLAVE_CPHA_1_FM0         1
#define  SPI_SLAVE_SHIFT_DIR_MSB_FM1  0
#define  SPI_SLAVE_SHIFT_DIR_LSB_FM1  1

/***********************************/
/* Verify performance requirements */
/***********************************/
#pragma verify_wctl  SPI_slave                 35  steps  10 rams
#pragma exclude_wctl SPI_slave::Init
#pragma exclude_wctl SPI_slave::InitSSActive
#pragma exclude_wctl SPI_slave::InitSSInactive

/* provide hint that channel frame base addr same on all chans touched by func */
#pragma same_channel_frame_base SPI_slave

/*
chan flag0 : 0==SCLK, 1==SS
chan flag1 : 0==SCLK leading next, 1==SCLK trailing next
*/

/*
MISO     - channel - 1
SCLK     - channel
MOSI     - channel + 1
SS       - <config> [optional]

interrupts
- from SS chan when SS goes low (if SS exists)
- from SCLK chan when word completes
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


_eTPU_class SPI_slave
{
    /* channel frame */
public:
    _Bool       _use_TCR1;
    _Bool       _CPOL;
    /*_Bool       _CPHA; */         /* held in FM0 */
    /*_Bool       _LSB_first; */    /* held in FM1 */
    int8_t      _bit_count;
    uint24_t    _data_out_reg;
    uint24_t    _data_in_reg;
    int24_t     _timeout;
    int8_t      _MISO_chan;
    int8_t      _selected_flag;

private:
    int8_t      _bit_count_current;
    uint24_t    _data_out_shift_reg;
    uint24_t    _data_in_shift_reg;

    /* threads */
    
    /* initialization */
    _eTPU_thread Init(_eTPU_matches_disabled);
    _eTPU_thread InitSSActive(_eTPU_matches_disabled);
    _eTPU_thread InitSSInactive(_eTPU_matches_disabled);

    /* Slave select channel threads */
    _eTPU_thread SelectTransDetected(_eTPU_matches_enabled);
    _eTPU_thread SelectLevelCheck(_eTPU_matches_enabled);
    
    /* SCLK channel threads */
    _eTPU_thread SetData(_eTPU_matches_enabled);
    _eTPU_thread ClockLeading(_eTPU_matches_enabled);
    _eTPU_thread ClockTrailing(_eTPU_matches_enabled);
    _eTPU_thread ClockTimeout(_eTPU_matches_enabled);

    /* fragments */
    _eTPU_fragment CommonInitSS();
    _eTPU_fragment ReadData();
    _eTPU_fragment WriteData();
    
    /* methods */
    /* none */

    /* entry table(s) */
    _eTPU_entry_table SPI_slave;
};

DEFINE_ENTRY_TABLE(SPI_slave, SPI_slave, alternate, inputpin, autocfsr)
{
	/*           HSR    LSR M1 M2 PIN F0 F1 vector */
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  0, x, InitSSActive),
	ETPU_VECTOR2(2,3,   x,  x, x, 0,  1, x, InitSSActive),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  0, x, InitSSInactive),
	ETPU_VECTOR2(2,3,   x,  x, x, 1,  1, x, InitSSInactive),
	ETPU_VECTOR3(1,4,5, x,  x, x, x,  x, x, Init),
	ETPU_VECTOR2(6,7,   x,  x, x, x,  x, x, SetData),
	
	/* Clock leading transition detected */
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 0, ClockLeading),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 0, ClockLeading),
	/* prioritize edge over timeout */
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 0, ClockLeading),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 0, ClockLeading),

	/* Clock trailing transition detected */
	ETPU_VECTOR1(0,     x,  0, 1, 0,  0, 1, ClockTrailing),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  0, 1, ClockTrailing),
	/* prioritize edge over timeout */
	ETPU_VECTOR1(0,     x,  1, 1, 0,  0, 1, ClockTrailing),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  0, 1, ClockTrailing),
	
	/* Clock timeout detected */
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 0, ClockTimeout),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  0, 1, ClockTimeout),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 0, ClockTimeout),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  0, 1, ClockTimeout),
	
	/* Select line transition detected */
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 0, SelectTransDetected),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 0, SelectTransDetected),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 0, SelectTransDetected),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 0, SelectTransDetected),
	
	/* Select line level check */
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 0, SelectLevelCheck),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 0, SelectLevelCheck),

    /* invalid entries */	
	ETPU_VECTOR1(0,     1,  0, 0, 0,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     1,  0, 0, 1,  x, x, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 0, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  0, 1, 1,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 0,  1, 1, _Error_handler_entry),
	ETPU_VECTOR1(0,     x,  1, 1, 1,  1, 1, _Error_handler_entry),
};


_eTPU_thread SPI_slave::Init(_eTPU_matches_disabled)
{
    if (_use_TCR1 == TRUE)
    {
        channel.TBSA = TBS_M1C1GE;
    }
    else
    {
        channel.TBSA = TBS_M2C2GE;
    }

    channel.PDCM = PDCM_SM_ST;
    channel.TBSA = TBSA_CLR_OBE;
    channel.IPACA = IPAC_EITHER;
    /* clear all latches */
    channel.LSR = LSR_CLEAR;
    channel.MRLA = MRL_CLEAR;
    channel.MRLB = MRL_CLEAR;
    channel.TDL = TDL_CLEAR;
    channel.MTD = MTD_ENABLE;
    /* set flag state, awaiting clock leading edge */
    channel.FLAG0 = 0;
    channel.FLAG1 = 0;
    _bit_count_current = 0;
    
    /* configure data channels */
    chan += 1;
    channel.TBSA = TBSA_CLR_OBE;
   
    chan -= 2;
    if (_selected_flag == 1)
    {
        channel.TBSA = TBSA_SET_OBE;
    }
    else
    {
        channel.TBSA = TBSA_CLR_OBE;
    }
}

_eTPU_thread SPI_slave::InitSSActive(_eTPU_matches_disabled)
{
    _selected_flag = 1;
    CommonInitSS();
    
}

_eTPU_thread SPI_slave::InitSSInactive(_eTPU_matches_disabled)
{
    _selected_flag = 0;
    CommonInitSS();
}

_eTPU_fragment SPI_slave::CommonInitSS()
{
    channel.TBSA = TBSA_CLR_OBE;
    channel.PDCM = PDCM_SM_ST;
    channel.TBSA = TBS_M1C1GE;
    channel.FLAG0 = 1;
    channel.IPACA = IPAC_EITHER; /* detect all transitions */
    channel.LSR = LSR_CLEAR;
    channel.MRLA = MRL_CLEAR;
    channel.MRLB = MRL_CLEAR;
    channel.TDL = TDL_CLEAR;
    
    /* start pin level polling */
    erta = tcr1 + _timeout;
    channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
    channel.MTD = MTD_ENABLE;
}


/* Slave select channel threads */
_eTPU_thread SPI_slave::SelectTransDetected(_eTPU_matches_enabled)
{
    channel.TDL = TDL_CLEAR;
    channel.CIRC = CIRC_INT_FROM_SERVICED;
    if (channel.PSTI == 0)
    {
        _selected_flag = 1;
        _bit_count_current = 0;
        chan = _MISO_chan;
        channel.TBSA = TBSA_SET_OBE;
        channel.CIRC = CIRC_INT_FROM_SERVICED;
        /* need to put first output bit on pin depending upon clock phase */
        if (channel.FM0 == 0)
        {
            WriteData();
        }
    }
    else
    {
        _selected_flag = 0;
        chan = _MISO_chan;
        channel.TBSA = TBSA_CLR_OBE;
    }
}

_eTPU_thread SPI_slave::SelectLevelCheck(_eTPU_matches_enabled)
{
    /* continue polling */
    erta += _timeout;
    channel.MRLA = MRL_CLEAR;
    channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
    
    if (channel.PSTI == 0)
    {
        if (_selected_flag == 0)
        {
            _selected_flag = 1;
            _bit_count_current = 0;
            chan = _MISO_chan;
            channel.TBSA = TBSA_SET_OBE;
            channel.CIRC = CIRC_INT_FROM_SERVICED;
            /* need to put first output bit on pin depending upon clock phase */
            if (channel.FM0 == 0)
            {
                WriteData();
            }
        }
    }
    else
    {
        if (_selected_flag == 1)
        {
            _selected_flag = 0;
            chan = _MISO_chan;
            channel.TBSA = TBSA_CLR_OBE;
            channel.CIRC = CIRC_INT_FROM_SERVICED;
        }
    }
}

/* SCLK channel threads */

_eTPU_thread SPI_slave::SetData(_eTPU_matches_enabled)
{
    /* when there is no slave select, and CPHA is 0, need to put the first bit out
       on pin BEFORE any clocking action */
    if (_selected_flag == 1 && channel.FM0 == 0)
    {
        chan -= 1;
        WriteData();
    }
}

_eTPU_thread SPI_slave::ClockLeading(_eTPU_matches_enabled)
{
    channel.TDL = TDL_CLEAR;
    if (!_selected_flag)
    {
        /* ignore this, slave not selected */
        return;
    }
    channel.FLAG1 = 1;
    
    if (channel.FM0 == 0)
    {
        chan += 1;
        ReadData();
    }
    else
    {
        chan -= 1;
        WriteData();
    }
}

_eTPU_thread SPI_slave::ClockTrailing(_eTPU_matches_enabled)
{
    channel.TDL = TDL_CLEAR;
    if (!_selected_flag)
    {
        /* ignore this, slave not selected */
        return;
    }
    channel.FLAG1 = 0;

    if (channel.FM0 == 0)
    {
        chan -= 1;
        WriteData();
    }
    else
    {
        chan += 1;
        ReadData();
    }
}

_eTPU_thread SPI_slave::ClockTimeout(_eTPU_matches_enabled)
{
    channel.MRLA = MRL_CLEAR;
    /* reset to awaiting new transmission */
    channel.FLAG1 = 0;
    _bit_count_current = 0;
    chan -= 1;
    channel.TBSA = TBSA_CLR_OBE;
}


_eTPU_fragment SPI_slave::ReadData()
{
    if (channel.FM1 == SPI_SLAVE_SHIFT_DIR_LSB_FM1)
    {
        _data_in_shift_reg >>= 1;
        if (channel.PSTI == 1)
            _data_in_shift_reg |= 0x800000;
    }
    else
    {
        _data_in_shift_reg <<= 1;
        if (channel.PSTI == 1)
            _data_in_shift_reg += 1;
    }

    if (++_bit_count_current == _bit_count)
    {
        /* this word is done */
        _data_in_reg = _data_in_shift_reg;
        channel.CIRC = CIRC_INT_FROM_SERVICED;
        /* prepare for next word */
        _bit_count_current = 0;
        /* end timeout check */
        channel.MRLE = MRLE_DISABLE;
    }
    else
    {
        /* set up timeout check */
        chan -= 1;
        channel.MRLE = MRLE_DISABLE;
        erta += _timeout;
        channel.ERWA = ERW_WRITE_ERT_TO_MATCH;
    }
}

_eTPU_fragment SPI_slave::WriteData()
{
    /* note that if CPHA == 0, this is entered at the end of the LAST clock, but
       sampling the data out register and outputing a bit is a don't care, so
       to streamline code, just let it happen */

    if (_bit_count_current == 0)
    {
        /* sample data out register into data out shift register */
        _data_out_shift_reg = _data_out_reg;
    } 

    if (channel.FM1 == SPI_SLAVE_SHIFT_DIR_LSB_FM1)
    {
        _data_out_shift_reg >>= 1;
    }
    else
    {
        _data_out_shift_reg <<= 1;
    }
    if (CC.C == 1)
    {
        channel.PIN = PIN_SET_HIGH;
    }
    else
    {
        channel.PIN = PIN_SET_LOW;
    }
}


#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_INIT_HSR", SPI_SLAVE_INIT_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_INIT_SS_HSR", SPI_SLAVE_INIT_SS_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_SET_DATA_HSR", SPI_SLAVE_SET_DATA_HSR
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_CPHA_0_FM0", SPI_SLAVE_CPHA_0_FM0
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_CPHA_1_FM0", SPI_SLAVE_CPHA_1_FM0
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_SHIFT_DIR_MSB_FM1", SPI_SLAVE_SHIFT_DIR_MSB_FM1
#pragma export_autodef_macro "FS_ETPU_SPI_SLAVE_SHIFT_DIR_LSB_FM1", SPI_SLAVE_SHIFT_DIR_LSB_FM1
