/**************************************************************************
 * FILE NAME: etpu_spi.c                                                  *
 * DESCRIPTION:                                                           *
 * This file contains the ETPU Serial Peripheral Interface (SPI) API.     *
 **************************************************************************/

#include "etpu_util_ext.h"      /* Utility routines for working eTPU */
#include "etpu_auto_api.h"      /* auto-generated eTPU interface data */
#include "etpu_spi.h"           /* eTPU SPI API header */


uint32_t fs_etpu_spi_master_init(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config)
{
    volatile struct eTPU_struct * eTPU;
    uint32_t timer_freq;
    uint32_t half_period;
    int32_t i;
    uint32_t timer_freq_mhz, timer_freq_khz, timer_freq_remainder, delay_calc_temp;
    uint32_t mode;

    if (p_spi_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
        if (p_spi_master_config->timer == FS_ETPU_TCR1)
        {
            if (p_spi_master_instance->clock_chan_num < 32)
            {
                timer_freq = etpu_a_tcr1_freq;
            }
            else
            {
                timer_freq = etpu_b_tcr1_freq;
            }
        }
        else
        {
            if (p_spi_master_instance->clock_chan_num < 32)
            {
                timer_freq = etpu_a_tcr2_freq;
            }
            else
            {
                timer_freq = etpu_b_tcr2_freq;
            }
        }
    }
    else
    {
        eTPU = eTPU_C;
        if (p_spi_master_config->timer == FS_ETPU_TCR1)
        {
            timer_freq = etpu_c_tcr1_freq;
        }
        else
        {
            timer_freq = etpu_c_tcr2_freq;
        }
    }

    /* first disable channels */
    fs_etpu_disable_ext(p_spi_master_instance->em, p_spi_master_instance->clock_chan_num - 1);
    fs_etpu_disable_ext(p_spi_master_instance->em, p_spi_master_instance->clock_chan_num);
    fs_etpu_disable_ext(p_spi_master_instance->em, p_spi_master_instance->clock_chan_num + 1);

    /* get channel frame memory configured */
    if (eTPU->CHAN[p_spi_master_instance->clock_chan_num].CR.B.CPBA == 0)
    {
        /* get parameter RAM for channel frame */
        p_spi_master_instance->cpba = fs_etpu_malloc_ext(p_spi_master_instance->em, _FRAME_SIZE_SPI_master_);
        p_spi_master_instance->cpba_pse = (void*)((uint32_t)p_spi_master_instance->cpba + (fs_etpu_data_ram_ext - fs_etpu_data_ram_start));

        if (p_spi_master_instance->cpba  == 0)
        {
            return (FS_ETPU_ERROR_MALLOC);
        }
    }
    else  /* set cpba to what is in the CR register */
    {
        p_spi_master_instance->cpba = fs_etpu_get_cpba_ext(p_spi_master_instance->em, p_spi_master_instance->clock_chan_num);
        p_spi_master_instance->cpba_pse = fs_etpu_get_cpba_pse_ext(p_spi_master_instance->em, p_spi_master_instance->clock_chan_num);
    }

    /* intialize channel frame */
    ((etpu_if_SPI_master_CHANNEL_FRAME*)p_spi_master_instance->cpba)->_BF_UNIT_0000._BF._CPOL = p_spi_master_config->clock_polarity;
    ((etpu_if_SPI_master_CHANNEL_FRAME*)p_spi_master_instance->cpba)->_bit_count = p_spi_master_config->transfer_size;
    for (i = 0; i < FS_ETPU_SPI_MASTER_MAX_SLAVE_SELECT_CNT; i++)
    {
        ((etpu_if_SPI_master_CHANNEL_FRAME*)p_spi_master_instance->cpba)->_slave_select_chan_list[i] = p_spi_master_instance->slave_select_chan_list[i];
    }
    half_period = timer_freq / (p_spi_master_config->baud_rate_hz * 2);
    ((etpu_if_SPI_master_CHANNEL_FRAME_PSE*)p_spi_master_instance->cpba_pse)->_half_period = half_period;
    timer_freq_mhz = timer_freq / 1000000;
    timer_freq_remainder = timer_freq - (timer_freq_mhz * 1000000);
    delay_calc_temp = timer_freq_mhz * p_spi_master_config->slave_select_delay_us;
    timer_freq_khz = timer_freq_remainder / 1000; //666
    timer_freq_remainder = timer_freq_remainder - (timer_freq_khz * 1000);
    delay_calc_temp += (timer_freq_khz * p_spi_master_config->slave_select_delay_us) / 1000;
    delay_calc_temp += (timer_freq_remainder * p_spi_master_config->slave_select_delay_us) / 1000000;
    ((etpu_if_SPI_master_CHANNEL_FRAME_PSE*)p_spi_master_instance->cpba_pse)->_slave_select_delay = delay_calc_temp;

    /* function mode */
    if (p_spi_master_config->clock_phase == 1)
    {
        mode = FS_ETPU_SPI_MASTER_CPHA_1_FM0;
    }
    else
    {
        mode = FS_ETPU_SPI_MASTER_CPHA_0_FM0;
    }
    if (p_spi_master_config->shift_direction == FS_ETPU_SPI_LSB_FIRST)
    {
        mode |= (FS_ETPU_SPI_MASTER_SHIFT_DIR_LSB_FM1 << 1);
    }
    else
    {
        mode |= (FS_ETPU_SPI_MASTER_SHIFT_DIR_MSB_FM1 << 1);
    }
    eTPU->CHAN[p_spi_master_instance->clock_chan_num].SCR.R = mode;

    /* write hsr */
    if (p_spi_master_config->timer == FS_ETPU_TCR1)
    {
        eTPU->CHAN[p_spi_master_instance->clock_chan_num].HSRR.R = FS_ETPU_SPI_MASTER_INIT_TCR1_HSR;
    }
    else
    {
        eTPU->CHAN[p_spi_master_instance->clock_chan_num].HSRR.R = FS_ETPU_SPI_MASTER_INIT_TCR2_HSR;
    }


    /* final channel configuration */
    /* MISO and MOSI channels have same base address as SCLK */
    eTPU->CHAN[p_spi_master_instance->clock_chan_num - 1].CR.R =
        (uint32_t) (((uint32_t)p_spi_master_instance->cpba & 0x3fff) >> 3);
    eTPU->CHAN[p_spi_master_instance->clock_chan_num + 1].CR.R =
        (uint32_t) (((uint32_t)p_spi_master_instance->cpba & 0x3fff) >> 3);

    eTPU->CHAN[p_spi_master_instance->clock_chan_num].CR.R =
        (p_spi_master_instance->priority << 28) + 
        (_ENTRY_TABLE_TYPE_SPI_master_ << 24) +
        (_FUNCTION_NUM_SPI_master_ << 16) + 
        (uint32_t) (((uint32_t)p_spi_master_instance->cpba & 0x3fff) >> 3);

    return 0;
}

uint32_t fs_etpu_spi_master_transmit_data(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config,
    uint32_t data,
    int8_t slave_select_index)
{
    volatile struct eTPU_struct * eTPU;

    if (p_spi_master_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

    /* pre-shift the data if necessary */
    if (p_spi_master_config->shift_direction == FS_ETPU_SPI_MSB_FIRST)
    {
        /* MSB first, need to shift to the top */
        data <<= (24 - p_spi_master_config->transfer_size);
    }
    ((etpu_if_SPI_master_CHANNEL_FRAME_PSE*)p_spi_master_instance->cpba_pse)->_data_out_reg = data;
    if (slave_select_index == -1)
    {
        ((etpu_if_SPI_master_CHANNEL_FRAME*)p_spi_master_instance->cpba)->_slave_select_chan = 0xff;
    }
    else
    {
        ((etpu_if_SPI_master_CHANNEL_FRAME*)p_spi_master_instance->cpba)->_slave_select_chan = 
            p_spi_master_instance->slave_select_chan_list[slave_select_index];
    }
    eTPU->CHAN[p_spi_master_instance->clock_chan_num].HSRR.R = FS_ETPU_SPI_MASTER_RUN_HSR;

    return 0;
}

uint32_t fs_etpu_spi_master_get_data(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config,
    uint32_t *p_data)
{
    uint32_t data;
    uint32_t mask = (1 << p_spi_master_config->transfer_size) - 1;

    data = ((etpu_if_SPI_master_CHANNEL_FRAME_PSE*)p_spi_master_instance->cpba_pse)->_data_in_reg;
    /* shift data to correct bits if necessary */
    if (p_spi_master_config->shift_direction == FS_ETPU_SPI_LSB_FIRST)
    {
        /* LSB first, need to shift down into position */
        data >>= (24 - p_spi_master_config->transfer_size);
    }
    *p_data = data & mask;

    return 0;
}


uint32_t fs_etpu_spi_slave_init(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config)
{
    volatile struct eTPU_struct * eTPU;
    uint32_t timer_freq;
    uint32_t timer_freq_mhz, timer_freq_khz, timer_freq_remainder, timeout_calc_temp;
    uint32_t mode;

    if (p_spi_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
        if (p_spi_slave_config->timer == FS_ETPU_TCR1)
        {
            if (p_spi_slave_instance->clock_chan_num < 32)
            {
                timer_freq = etpu_a_tcr1_freq;
            }
            else
            {
                timer_freq = etpu_b_tcr1_freq;
            }
        }
        else
        {
            if (p_spi_slave_instance->clock_chan_num < 32)
            {
                timer_freq = etpu_a_tcr2_freq;
            }
            else
            {
                timer_freq = etpu_b_tcr2_freq;
            }
        }
    }
    else
    {
        eTPU = eTPU_C;
        if (p_spi_slave_config->timer == FS_ETPU_TCR1)
        {
            timer_freq = etpu_c_tcr1_freq;
        }
        else
        {
            timer_freq = etpu_c_tcr2_freq;
        }
    }

    /*first disable channels*/
    fs_etpu_disable_ext(p_spi_slave_instance->em, p_spi_slave_instance->clock_chan_num - 1);
    fs_etpu_disable_ext(p_spi_slave_instance->em, p_spi_slave_instance->clock_chan_num);
    fs_etpu_disable_ext(p_spi_slave_instance->em, p_spi_slave_instance->clock_chan_num + 1);
    if (p_spi_slave_instance->ss_chan_num != 0xff)
    {
        fs_etpu_disable_ext(p_spi_slave_instance->em, p_spi_slave_instance->ss_chan_num);
    }

    /* get channel frame memory configured */
    if (eTPU->CHAN[p_spi_slave_instance->clock_chan_num].CR.B.CPBA == 0)
    {
        /* get parameter RAM for channel frame */
        p_spi_slave_instance->cpba = fs_etpu_malloc_ext(p_spi_slave_instance->em, _FRAME_SIZE_SPI_slave_);
        p_spi_slave_instance->cpba_pse = (void*)((uint32_t)p_spi_slave_instance->cpba + (fs_etpu_data_ram_ext - fs_etpu_data_ram_start));

        if (p_spi_slave_instance->cpba  == 0)
        {
            return (FS_ETPU_ERROR_MALLOC);
        }
    }
    else                        /*set pba to what is in the CR register */
    {
        p_spi_slave_instance->cpba = fs_etpu_get_cpba_ext(p_spi_slave_instance->em, p_spi_slave_instance->clock_chan_num);
        p_spi_slave_instance->cpba_pse = fs_etpu_get_cpba_pse_ext(p_spi_slave_instance->em, p_spi_slave_instance->clock_chan_num);
    }

    /* intialize channel frame */
    ((etpu_if_SPI_slave_CHANNEL_FRAME*)p_spi_slave_instance->cpba)->_BF_UNIT_0000._BF._use_TCR1 = (p_spi_slave_config->timer == FS_ETPU_TCR1);
    ((etpu_if_SPI_slave_CHANNEL_FRAME*)p_spi_slave_instance->cpba)->_BF_UNIT_0000._BF._CPOL = p_spi_slave_config->clock_polarity;
    ((etpu_if_SPI_slave_CHANNEL_FRAME*)p_spi_slave_instance->cpba)->_bit_count = p_spi_slave_config->transfer_size;
    ((etpu_if_SPI_slave_CHANNEL_FRAME*)p_spi_slave_instance->cpba)->_MISO_chan = p_spi_slave_instance->clock_chan_num - 1;;
    /* if there is no slve select channel, then selected flag must be initialized on (always on) */
    ((etpu_if_SPI_slave_CHANNEL_FRAME*)p_spi_slave_instance->cpba)->_selected_flag = (p_spi_slave_instance->ss_chan_num == 0xff ? 1 : 0);
    /* calculate the timeout carefully to avoid numerical overflow and avoid floating point use */
    timer_freq_mhz = timer_freq / 1000000;
    timer_freq_remainder = timer_freq - (timer_freq_mhz * 1000000);
    timeout_calc_temp = timer_freq_mhz * p_spi_slave_config->timeout_us;
    timer_freq_khz = timer_freq_remainder / 1000; //666
    timer_freq_remainder = timer_freq_remainder - (timer_freq_khz * 1000);
    timeout_calc_temp += (timer_freq_khz * p_spi_slave_config->timeout_us) / 1000;
    timeout_calc_temp += (timer_freq_remainder * p_spi_slave_config->timeout_us) / 1000000;
    ((etpu_if_SPI_slave_CHANNEL_FRAME_PSE*)p_spi_slave_instance->cpba_pse)->_timeout = timeout_calc_temp;
    
    /* function mode */
    if (p_spi_slave_config->clock_phase == 1)
    {
        mode = FS_ETPU_SPI_SLAVE_CPHA_1_FM0;
    }
    else
    {
        mode = FS_ETPU_SPI_SLAVE_CPHA_0_FM0;
    }
    if (p_spi_slave_config->shift_direction == FS_ETPU_SPI_LSB_FIRST)
    {
        mode |= (FS_ETPU_SPI_SLAVE_SHIFT_DIR_LSB_FM1 << 1);
    }
    else
    {
        mode |= (FS_ETPU_SPI_SLAVE_SHIFT_DIR_MSB_FM1 << 1);
    }
    eTPU->CHAN[p_spi_slave_instance->clock_chan_num].SCR.R = mode;
    if (p_spi_slave_instance->ss_chan_num != 0xff)
    {
        eTPU->CHAN[p_spi_slave_instance->ss_chan_num].SCR.R = mode;
    }
    
    /* hsr */
    eTPU->CHAN[p_spi_slave_instance->clock_chan_num].HSRR.R = FS_ETPU_SPI_SLAVE_INIT_HSR;
    if (p_spi_slave_instance->ss_chan_num != 0xff)
    {
        eTPU->CHAN[p_spi_slave_instance->ss_chan_num].HSRR.R = FS_ETPU_SPI_SLAVE_INIT_SS_HSR;
    }
    
    /* final channel configuration */
    eTPU->CHAN[p_spi_slave_instance->clock_chan_num - 1].CR.R =
        (uint32_t) (((uint32_t)p_spi_slave_instance->cpba & 0x3fff) >> 3);
    eTPU->CHAN[p_spi_slave_instance->clock_chan_num + 1].CR.R =
        (uint32_t) (((uint32_t)p_spi_slave_instance->cpba & 0x3fff) >> 3);

    eTPU->CHAN[p_spi_slave_instance->clock_chan_num].CR.R =
        (p_spi_slave_instance->priority << 28) + 
        (_ENTRY_TABLE_TYPE_SPI_slave_ << 24) +
        (_FUNCTION_NUM_SPI_slave_ << 16) + 
        (uint32_t) (((uint32_t)p_spi_slave_instance->cpba & 0x3fff) >> 3);
        
    if (p_spi_slave_instance->ss_chan_num != 0xff)
    {
        eTPU->CHAN[p_spi_slave_instance->ss_chan_num].CR.R =
            (p_spi_slave_instance->priority << 28) + 
            (_ENTRY_TABLE_TYPE_SPI_slave_ << 24) +
            (_FUNCTION_NUM_SPI_slave_ << 16) + 
            (uint32_t) (((uint32_t)p_spi_slave_instance->cpba & 0x3fff) >> 3);
    }

    return 0;
}

uint32_t fs_etpu_spi_slave_set_data(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config,
    uint32_t data)
{
    volatile struct eTPU_struct * eTPU;

    if (p_spi_slave_instance->em == EM_AB)
    {
        eTPU = eTPU_AB;
    }
    else
    {
        eTPU = eTPU_C;
    }

    /* pre-shift the data if necessary */
    if (p_spi_slave_config->shift_direction == FS_ETPU_SPI_MSB_FIRST)
    {
        /* MSB first, need to shift to the top */
        data <<= (24 - p_spi_slave_config->transfer_size);
    }
    ((etpu_if_SPI_slave_CHANNEL_FRAME_PSE*)p_spi_slave_instance->cpba_pse)->_data_out_reg = data;
    eTPU->CHAN[p_spi_slave_instance->clock_chan_num].HSRR.R = FS_ETPU_SPI_SLAVE_SET_DATA_HSR;

    return 0;
}

uint32_t fs_etpu_spi_slave_get_data(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config,
    uint32_t *p_data)
{
    uint32_t data;
    uint32_t mask = (1 << p_spi_slave_config->transfer_size) - 1;

    data = ((etpu_if_SPI_slave_CHANNEL_FRAME_PSE*)p_spi_slave_instance->cpba_pse)->_data_in_reg;
    /* shift data to correct bits if necessary */
    if (p_spi_slave_config->shift_direction == FS_ETPU_SPI_LSB_FIRST)
    {
        /* LSB first, need to shift down into position */
        data >>= (24 - p_spi_slave_config->transfer_size);
    }
    *p_data = data & mask;

    return 0;
}


/*********************************************************************
 *
 * ASH WARE has almost completely modifed and enhanced this eTPU function,
 * however a small amount of original code remains so the original
 * Freescale (NXP) copyright notice has been retained below.
 ********************************************************************/

/*********************************************************************
 *
 * Copyright:
 *	Freescale Semiconductor, INC. All Rights Reserved.
 *  You are hereby granted a copyright license to use, modify, and
 *  distribute the SOFTWARE so long as this entire notice is
 *  retained without alteration in any modified and/or redistributed
 *  versions, and that such modified versions are clearly identified
 *  as such. No licenses are granted by implication, estoppel or
 *  otherwise under any patents or trademarks of Freescale
 *  Semiconductor, Inc. This software is provided on an "AS IS"
 *  basis and without warranty.
 *
 *  To the maximum extent permitted by applicable law, Freescale
 *  Semiconductor DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
 *  INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
 *  PARTICULAR PURPOSE AND ANY WARRANTY AGAINST INFRINGEMENT WITH
 *  REGARD TO THE SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
 *  AND ANY ACCOMPANYING WRITTEN MATERIALS.
 *
 *  To the maximum extent permitted by applicable law, IN NO EVENT
 *  SHALL Freescale Semiconductor BE LIABLE FOR ANY DAMAGES WHATSOEVER
 *  (INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
 *  BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR OTHER
 *  PECUNIARY LOSS) ARISING OF THE USE OR INABILITY TO USE THE SOFTWARE.
 *
 *  Freescale Semiconductor assumes no responsibility for the
 *  maintenance and support of this software
 ********************************************************************/
