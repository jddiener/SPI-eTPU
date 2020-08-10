/* main.c
 *
 * This is the entry point for the system.  System init
 * is performed, then the main app is kicked off.
 */

/* This code initializes the eTPU and a SPI master and slave that communicate with each
   other via connected pins.  A number of data transfers are done in all the different
   modes to perform a basic operational test. */

/* for sim environment */
#include "isrLib.h"
#include "ScriptLib.h"

/* for eTPU/SPI */
#include "etpu_util_ext.h"
#include "etpu_gct.h"
#include "etpu_spi.h"


uint32_t g_complete_flag = 0;


uint32_t test_spi_word_transfer(uint32_t master_tx_word, uint32_t slave_tx_word, int8_t ss_index, uint32_t finish_time)
{
    uint32_t err_code;
    uint32_t master_data, slave_data;

    /* always re-initialize to ensure new config in place */
    err_code = fs_etpu_spi_master_init(&spi_master_1_instance, &spi_master_1_config);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    err_code = fs_etpu_spi_slave_init(&spi_slave_1_instance, &spi_slave_1_config);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;

    /* do test */

    /* send a byte on SPI1 */
    fs_etpu_spi_slave_set_data(&spi_slave_1_instance, &spi_slave_1_config, slave_tx_word);
    fs_etpu_spi_master_transmit_data(&spi_master_1_instance, &spi_master_1_config, master_tx_word, ss_index);

    at_time(finish_time);
    err_code = fs_etpu_spi_master_get_data(&spi_master_1_instance, &spi_master_1_config, &master_data);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    err_code = fs_etpu_spi_slave_get_data(&spi_slave_1_instance, &spi_slave_1_config, &slave_data);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    if (master_data != slave_tx_word) return 1;
    if (slave_data != master_tx_word) return 1;
        
    return 0;
}


/* main application entry point */
/* w/ GNU, if we name this main, it requires linking with the libgcc.a
   run-time support.  This may be useful with C++ because this extra
   code initializes static C++ objects.  However, this C demo will
   skip it */
int user_main()
{
    uint32_t err_code;
    uint32_t master_data, slave_data;
    uint32_t cisr;
    uint32_t master_sclk_cisr_mask = 1 << (ETPU_SPI_MASTER1_SCLK_CHAN & 0x1f);
    uint32_t slave_sclk_cisr_mask = 1 << (ETPU_SPI_SLAVE1_SCLK_CHAN & 0x1f);
    uint32_t slave_ss_cisr_mask = 1 << (ETPU_SPI_SLAVE1_SS_CHAN & 0x1f);
    uint32_t transfer_done_isr_cnt;

	/* initialize interrupt support */
	isrLibInit();
	
    /* initialize eTPU */	
    if (my_system_etpu_init())
        return 1;

    /* start eTPU */
    my_system_etpu_start();
    
    at_time(1000);
    
    /* re-initialize with CPOL=0, CPHA=0 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0xa3, 0x5a, -1, 1100)) return 1;

    /* re-initialize with CPOL=1, CPHA=0 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0x3a, 0xa5, -1, 1200)) return 1;

    /* re-initialize with CPOL=0, CPHA=1 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0xcc, 0x33, -1, 1300)) return 1;

    /* re-initialize with CPOL=1, CPHA=1 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0x70, 0x67, -1, 1400)) return 1;
    
    
    /******************************************/
    /* switch to MSB first, re-run same tests */
    /******************************************/

    at_time(2000);
    spi_master_1_config.shift_direction = 0;
    spi_slave_1_config.shift_direction = 0;

    /* re-initialize with CPOL=0, CPHA=0 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0xa3, 0x5a, -1, 2100)) return 1;

    /* re-initialize with CPOL=1, CPHA=0 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0x3a, 0xa5, -1, 2200)) return 1;

    /* re-initialize with CPOL=0, CPHA=1 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0xcc, 0x33, -1, 2300)) return 1;

    /* re-initialize with CPOL=1, CPHA=1 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0x70, 0x67, -1, 2400)) return 1;


    /******************************************/
    /* turn on slave select capability and test */
    /******************************************/

    at_time(3000);
    spi_master_1_instance.slave_select_chan_list[0] = ETPU_SPI_MASTER1_SS_CHAN;
    spi_slave_1_instance.ss_chan_num = ETPU_SPI_SLAVE1_SS_CHAN;
    /* also, LSB first */
    spi_master_1_config.shift_direction = 1;
    spi_slave_1_config.shift_direction = 1;

    /* re-initialize with CPOL=0, CPHA=0 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0xa3, 0x5a, 0, 3200)) return 1;

    /* re-initialize with CPOL=1, CPHA=0 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 0;
    if (test_spi_word_transfer(0x3a, 0xa5, 0, 3400)) return 1;

    /* re-initialize with CPOL=0, CPHA=1 */

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0xcc, 0x33, 0, 3600)) return 1;

    /* re-initialize with CPOL=1, CPHA=1 */

    spi_master_1_config.clock_polarity = 1;
    spi_master_1_config.clock_phase = 1;
    spi_slave_1_config.clock_polarity = 1;
    spi_slave_1_config.clock_phase = 1;
    if (test_spi_word_transfer(0x70, 0x67, 0, 3800)) return 1;


    /******************************************/
    /* test interrupts */
    /******************************************/

    /* 3 different interrupts :
         - master SCLK interrupt -> transfer done
         - slave SCLK interrupt -> transfer done
         - slave SS interrupt -> slave has been selected */

    /* clear all interrupts */
    eTPU_AB->CISR_A.R = 0xffffffff;

    spi_master_1_config.clock_polarity = 0;
    spi_master_1_config.clock_phase = 0;
    spi_slave_1_config.clock_polarity = 0;
    spi_slave_1_config.clock_phase = 0;
    err_code = fs_etpu_spi_master_init(&spi_master_1_instance, &spi_master_1_config);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    err_code = fs_etpu_spi_slave_init(&spi_slave_1_instance, &spi_slave_1_config);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;

    /* start transmission, and poll for interrupts */
    fs_etpu_spi_master_transmit_data(&spi_master_1_instance, &spi_master_1_config, 0x6e, 0);
    transfer_done_isr_cnt = 0;
	while (1)
	{
		cisr = eTPU_AB->CISR_A.R;
		if (cisr & master_sclk_cisr_mask)
		{
			/* clear handled interrupt */
			eTPU_AB->CISR_A.R = master_sclk_cisr_mask;
			transfer_done_isr_cnt += 1;
			cisr &= ~master_sclk_cisr_mask;
		}
		if (cisr & slave_sclk_cisr_mask)
		{
			/* clear handled interrupt */
			eTPU_AB->CISR_A.R = slave_sclk_cisr_mask;
			transfer_done_isr_cnt += 1;
			cisr &= ~slave_sclk_cisr_mask;
		}
		if (cisr & slave_ss_cisr_mask)
		{
			/* clear handled interrupt */
			eTPU_AB->CISR_A.R = slave_ss_cisr_mask;
			/* put slave data out */
            fs_etpu_spi_slave_set_data(&spi_slave_1_instance, &spi_slave_1_config, 0x44);
			cisr &= ~slave_ss_cisr_mask;
		}
		if (cisr != 0) return 1; /* unknown interrupt */
		
		if (transfer_done_isr_cnt == 2) break;
	}
	/* read and verify transfer */
    err_code = fs_etpu_spi_master_get_data(&spi_master_1_instance, &spi_master_1_config, &master_data);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    err_code = fs_etpu_spi_slave_get_data(&spi_slave_1_instance, &spi_slave_1_config, &slave_data);
    if (err_code != FS_ETPU_ERROR_NONE) return 1;
    if (master_data != 0x44) return 1;
    if (slave_data != 0x6e) return 1;
    


	/* TESTING DONE */
	
	at_time(4000);

	g_complete_flag = 1;

	while (1)
		;

	return 0;
}
