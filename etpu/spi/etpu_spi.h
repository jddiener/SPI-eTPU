/**************************************************************************
 * FILE NAME: etpu_spi.h                                                  *
 * DESCRIPTION:                                                           *
 * This file contains the prototypes and defines for the ETPU Serial      *
 * Peripheral Interface (SPI) API (master and slave).                     *
 *========================================================================*/

#ifndef __ETPU_SPI_H
#define __ETPU_SPI_H

#include "etpu_util_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FS_ETPU_SPI_MASTER_MAX_SLAVE_SELECT_CNT 4

#define FS_ETPU_SPI_MSB_FIRST   0
#define FS_ETPU_SPI_LSB_FIRST   1

/**************************************************************************/
/*                            Definitions                                 */
/**************************************************************************/

/**************************************************************************/
/*                       Function Prototypes                              */
/**************************************************************************/

/* Channel assignment 
MISO = clock_channel - 1
SCLK = clock_channel
MOSI = clock_channel + 1
SS channel(s) [optional, same engine]
*/

/* New eTPU functions */
/* SPI initialization */

/*******************************************************************************
* Type Definitions
*******************************************************************************/

/** A structure to represent an instance of SPI_master
 *  It includes static SPI_master initialization items. */
struct spi_master_instance_t
{
    ETPU_MODULE   em;
    uint8_t       clock_chan_num;
    /* slave select channels should be set to 0xff to disable */
    uint8_t       slave_select_chan_list[FS_ETPU_SPI_MASTER_MAX_SLAVE_SELECT_CNT];
    uint8_t       priority;
    void          *cpba;        /* set during initialization */
    void          *cpba_pse;    /* set during initialization */
};
/** A structure to represent a configuration of SPI_master.
 *  It includes SPI_master configuration items which can be changed in run-time. */
struct spi_master_config_t
{
    uint8_t       timer; /* FS_ETPU_TCR1 or FS_ETPU_TCR2 */
    uint8_t       clock_polarity; /* standard CPOL 0 or 1 setting */
    uint8_t       clock_phase;    /* standard CPHA 0 ir 1 setting */
    uint8_t       shift_direction; /* 0 -> MSB first, 1 -> LSB first */
    uint8_t       transfer_size; /* 1 to 24 bits */
    uint32_t      baud_rate_hz; /* baud rate in Hz */
    uint32_t      slave_select_delay_us; /* if slave select (ss) is used, this is 
                    the time (us) between the ss output gettign set active (low) and the
                    first clock pulse - should be set to at least half a bit time. */
};

/** A structure to represent an instance of SPI_slave
 *  It includes static SPI_slave initialization items. */
struct spi_slave_instance_t
{
    ETPU_MODULE   em;
    uint8_t       clock_chan_num;
    uint8_t       ss_chan_num; /* set to 0xff if slave does not use a slave select input */
    uint8_t       priority;
    void          *cpba;        /* set during initialization */
    void          *cpba_pse;    /* set during initialization */
};
/** A structure to represent a configuration of SPI_slave.
 *  It includes SPI_slave configuration items which can be changed in run-time. */
struct spi_slave_config_t
{
    uint8_t       timer; /* FS_ETPU_TCR1 or FS_ETPU_TCR2 */
    uint8_t       clock_polarity; /* standard CPOL 0 or 1 setting */
    uint8_t       clock_phase;    /* standard CPHA 0 ir 1 setting */
    uint8_t       shift_direction; /* 0 -> MSB first, 1 -> LSB first */
    uint8_t       transfer_size; /* 1 to 24 bits */
    uint32_t      timeout_us; /* if a transfer starts, but doesn't complete, after
                    this amount of time (us), the slave SPI re-initializes itself to prepare
                    for another transfer. */
};

/* SPI master interfaces */

uint32_t fs_etpu_spi_master_init(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config);

uint32_t fs_etpu_spi_master_transmit_data(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config,
    uint32_t data,
    int8_t slave_select_index); /* -1 indicates no ss */

uint32_t fs_etpu_spi_master_get_data(
    struct spi_master_instance_t *p_spi_master_instance,
    struct spi_master_config_t   *p_spi_master_config,
    uint32_t *p_data);


/* SPI slave interfaces */

uint32_t fs_etpu_spi_slave_init(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config);

uint32_t fs_etpu_spi_slave_set_data(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config,
    uint32_t data);

uint32_t fs_etpu_spi_slave_get_data(
    struct spi_slave_instance_t *p_spi_slave_instance,
    struct spi_slave_config_t   *p_spi_slave_config,
    uint32_t *p_data);


#ifdef __cplusplus
}
#endif

#endif /* __ETPU_SPI_H */


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
