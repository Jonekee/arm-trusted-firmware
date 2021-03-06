/** @file

  Copyright (c) 2016, Intel Corporation. All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or other
  materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors may
  be used to endorse or promote products derived from this software without specific
  prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
  DAMAGE.

**/

//
// Include files
//
#include <arch.h>
#include <arch_helpers.h>
#include <arm_gic.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <errno.h>
#include <generic_delay_timer.h>
#include <mmio.h>
#include <platform.h>
#include <platform_def.h>
#include <platform_private.h>

#include <console_printf.h>
#include "Altera_Hps_Socal.h"
#include "ClockManager.h"
#include "Handoff.h"
//
// Global Variables
//

static const CLOCK_SOURCE_CONFIG  mClkSrc = {
	/* clk_freq_of_eosc1 */
	(uint32_t) 25000000,
	/* clk_freq_of_f2h_free */
	(uint32_t) 460000000,
	/* clk_freq_of_cb_intosc_ls */
	(uint32_t) 50000000,
	};

static const CLOCK_MANAGER_MAIN_PLL_CONFIG MainPllConfig = {
	/* main_pll_mpuclk 800MHz */
	(ALT_CLKMGR_MAINPLL_MPUCLK_SRC_SET(ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_MAIN) |
	 ALT_CLKMGR_MAINPLL_MPUCLK_CNT_SET(0)),
	/* main_pll_nocclk 400MHz */
	(ALT_CLKMGR_MAINPLL_NOCCLK_SRC_SET(ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_MAIN) |
	ALT_CLKMGR_MAINPLL_NOCCLK_CNT_SET(0)),
	/* main_pll_cntr2clk / emaca - 200MHz unused */
	999,
	/* main_pll_cntr3clk / emacb - 200MHz unused */
	999,
	/* main_pll_cntr4clk / emacptp - 200MHz unused */
	999,
	/* main_pll_cntr5clk / gpio_db - 200MHz*/
	1,
	/* main_pll_cntr6clk / sdmmc - 200MHz*/
	1,
	/* main_pll_cntr7clk / s2f_user0 - 400MHz unused */
	(ALT_CLKMGR_MAINPLL_CNTR7CLK_SRC_SET(ALT_CLKMGR_MAINPLL_CNTR7CLK_SRC_E_PERI) |
	 ALT_CLKMGR_MAINPLL_CNTR7CLK_SRC_SET(999)),
	/* main_pll_cntr8clk / s2f_user1 - 400MHz unused */
	999,
	/* main_pll_cntr9clk / psi - 400MHz unused*/
	999,
	/* main_pll_nocdiv */
	(ALT_CLKMGR_MAINPLL_NOCDIV_L4MAINCLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_L4MAINCLK_E_DIV1) |
	ALT_CLKMGR_MAINPLL_NOCDIV_L4MPCLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_L4MPCLK_E_DIV2) |
	ALT_CLKMGR_MAINPLL_NOCDIV_L4SPCLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_L4SPCLK_E_DIV4) |
	ALT_CLKMGR_MAINPLL_NOCDIV_CSATCLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_CSATCLK_E_DIV1) |
	ALT_CLKMGR_MAINPLL_NOCDIV_CSTRACECLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_CSTRACECLK_E_DIV1) |
	ALT_CLKMGR_MAINPLL_NOCDIV_CSPDBGCLK_SET(ALT_CLKMGR_MAINPLL_NOCDIV_CSPDBGCLK_E_DIV4)),
	/* main_pll_pllglob */
	(ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_SET(ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_E_EOSC1) |
	 ALT_CLKMGR_MAINPLL_PLLGLOB_REFCLKDIV_SET(1)),
	/* main_pll_fdbck - 1600MHz */
	ALT_CLKMGR_MAINPLL_FDBCK_MDIV_SET(58),
	/* main_pll_pllc0 - 800MHz */
	(ALT_CLKMGR_MAINPLL_PLLC0_EN_SET(1) | ALT_CLKMGR_MAINPLL_PLLC0_DIV_SET(2)),
	/* main_pll_pllc1 - 400MHz */
	(ALT_CLKMGR_MAINPLL_PLLC1_EN_SET(1) | ALT_CLKMGR_MAINPLL_PLLC1_DIV_SET(4)),

};
static const CLOCK_MANAGER_PER_PLL_CONFIG PerPllConfig = {
	/* per_pll_cntr2clk / emaca - 250MHz */
	(ALT_CLKMGR_PERPLL_CNTR2CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR2CLK_SRC_E_PERI) |
	ALT_CLKMGR_PERPLL_CNTR2CLK_CNT_SET(1)),
	/* per_pll_cntr3clk / emacb - 250MHz */
	(ALT_CLKMGR_PERPLL_CNTR3CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR3CLK_SRC_E_PERI) |
	ALT_CLKMGR_PERPLL_CNTR3CLK_CNT_SET(1)),
	/* per_pll_cntr4clk / emacptp - 100MHz */
	(ALT_CLKMGR_PERPLL_CNTR4CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR4CLK_SRC_E_PERI) |
	ALT_CLKMGR_PERPLL_CNTR4CLK_CNT_SET(4)),
	/* per_pll_cntr5clk / gpio_db - 250MHz unused */
	(ALT_CLKMGR_PERPLL_CNTR5CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR5CLK_SRC_E_MAIN) |
	ALT_CLKMGR_PERPLL_CNTR5CLK_CNT_SET(999)),
	/* per_pll_cntr6clk / sdmmc - 250MHz unused */
	(ALT_CLKMGR_PERPLL_CNTR6CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR6CLK_SRC_E_MAIN) |
	ALT_CLKMGR_PERPLL_CNTR6CLK_CNT_SET(999)),
	/* per_pll_cntr7clk / s2f_user0 - 500MHz */
	0,
	/* per_pll_cntr8clk / s2f_user1 - 500MHz */
	(ALT_CLKMGR_PERPLL_CNTR8CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR8CLK_SRC_E_PERI) |
	ALT_CLKMGR_PERPLL_CNTR8CLK_CNT_SET(0)),
	/* per_pll_cntr9clk / psi - 500MHz */
	(ALT_CLKMGR_PERPLL_CNTR9CLK_SRC_SET(ALT_CLKMGR_PERPLL_CNTR9CLK_SRC_E_PERI) |
	ALT_CLKMGR_PERPLL_CNTR9CLK_CNT_SET(0)),
	/* per_pll_emacctl */
	(ALT_CLKMGR_PERPLL_EMACCTL_EMAC0SEL_SET(0) |
	ALT_CLKMGR_PERPLL_EMACCTL_EMAC1SEL_SET(0) |
	ALT_CLKMGR_PERPLL_EMACCTL_EMAC2SEL_SET(0) ),
	/* per_pll_gpiodiv */
	0,
	/* per_pll_pllglob */
	(ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_SET(ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_E_EOSC1) |
	 ALT_CLKMGR_PERPLL_PLLGLOB_REFCLKDIV_SET(1)),
	/* per_pll_fdbck - 2000MHz */
	(ALT_CLKMGR_PERPLL_FDBCK_MDIV_SET(74)),
	/* per_pll_pllc0 - 1000MHz unused */
	(ALT_CLKMGR_PERPLL_PLLC0_EN_SET(1) | ALT_CLKMGR_PERPLL_PLLC0_DIV_SET(2)),
	/* per_pll_pllc1 - 500MHz*/
	(ALT_CLKMGR_PERPLL_PLLC1_EN_SET(1) | ALT_CLKMGR_PERPLL_PLLC1_DIV_SET(4)),
};


//
// Functions
void WaitFsm (void)
{
  uint32_t Data32;
  do {
    Data32 = mmio_read_32 (ALT_CLKMGR_OFST +
                         ALT_CLKMGR_STAT_OFST);
  } while (ALT_CLKMGR_STAT_BUSY_GET(Data32) == ALT_CLKMGR_STAT_BUSY_E_BUSY);
}

void ConfigureClockManager (void)
{
  uint32_t Mdiv;
  uint32_t Refclkdiv;
  uint32_t Mscnt;
  uint32_t Hscnt;
  CLOCK_MANAGER_CONFIG ClkCfg;
  console_printf("Configure Clock Manager\n");
  // init struct value
  // use default value
  ClkCfg.mainpll = MainPllConfig;
  ClkCfg.perpll = PerPllConfig;

  // 1. Disable mainpllgrp's software-managed clock
  /*mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_ENR_OFST,
               ALT_CLKMGR_MAINPLL_ENR_S2FUSER0CLKEN_SET_MSK);
  // Disable perpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_ENR_OFST,
               ALT_CLKMGR_PERPLL_ENR_RESET);
	*/
  // 2. Bypass Register has bypass, bypassS (set), bypassR (clear)
  // Bypassed all mainpllgrp's clocks
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_BYPASS_OFST,
               0x7);
  WaitFsm();
  // Bypassed all perpllgrp's clocks
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_BYPASS_OFST,
               0x7f);
  WaitFsm();
  // 3. Setup main PLL dividers
  // Calculate the vcocalib value
  Mdiv = ALT_CLKMGR_MAINPLL_FDBCK_MDIV_GET(ClkCfg.mainpll.fdbck);
  Refclkdiv = ALT_CLKMGR_MAINPLL_PLLGLOB_REFCLKDIV_GET(ClkCfg.mainpll.pllglob);
  Mscnt = 200 / (6 + Mdiv) / Refclkdiv;
  Hscnt = (Mdiv + 6) * Mscnt / Refclkdiv - 9;

  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLGLOB_OFST,
                 ClkCfg.mainpll.pllglob &
				 ALT_CLKMGR_MAINPLL_PLLGLOB_PD_CLR_MSK &
			     ALT_CLKMGR_MAINPLL_PLLGLOB_RST_CLR_MSK);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_FDBCK_OFST,
                 ClkCfg.mainpll.fdbck);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_VCOCALIB_OFST,
                 ALT_CLKMGR_MAINPLL_VCOCALIB_HSCNT_SET(Hscnt) |
				 ALT_CLKMGR_MAINPLL_VCOCALIB_MSCNT_SET(Mscnt));
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLC0_OFST,
                 ClkCfg.mainpll.pllc0);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLC1_OFST,
                 ClkCfg.mainpll.pllc1);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCDIV_OFST,
               ClkCfg.mainpll.nocdiv);

  // 4. Setup peripheral PLL dividers
  // Calculate the vcocalib value
  Mdiv = ALT_CLKMGR_PERPLL_FDBCK_MDIV_GET(ClkCfg.perpll.fdbck);
  Refclkdiv = ALT_CLKMGR_PERPLL_PLLGLOB_REFCLKDIV_GET(ClkCfg.perpll.pllglob);
  Mscnt = 200 / (6 + Mdiv) / Refclkdiv;
  Hscnt = (Mdiv + 6) * Mscnt / Refclkdiv - 9;

  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLGLOB_OFST,
                 ClkCfg.perpll.pllglob &
				 ALT_CLKMGR_PERPLL_PLLGLOB_PD_CLR_MSK &
			     ALT_CLKMGR_PERPLL_PLLGLOB_RST_CLR_MSK);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_FDBCK_OFST,
                 ClkCfg.perpll.fdbck);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_VCOCALIB_OFST,
                 ALT_CLKMGR_PERPLL_VCOCALIB_HSCNT_SET(Hscnt) |
				 ALT_CLKMGR_PERPLL_VCOCALIB_MSCNT_SET(Mscnt));
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLC0_OFST,
                 ClkCfg.perpll.pllc0);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLC1_OFST,
                 ClkCfg.perpll.pllc1);

  // Select EMAC clock source
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_EMACCTL_OFST,
               ClkCfg.perpll.emacctl);

  // Init GPIO De-bounce Clock Divider
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_GPIODIV_OFST,
               ALT_CLKMGR_PERPLL_GPIODIV_GPIODBCLK_SET(ClkCfg.perpll.gpiodiv));


  // 5. Take both PLL out of reset and power up
  mmio_setbits_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_PLLGLOB_OFST,
               ALT_CLKMGR_MAINPLL_PLLGLOB_PD_SET_MSK |
			   ALT_CLKMGR_MAINPLL_PLLGLOB_RST_SET_MSK);
  mmio_setbits_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_PLLGLOB_OFST,
               ALT_CLKMGR_PERPLL_PLLGLOB_PD_SET_MSK |
			   ALT_CLKMGR_PERPLL_PLLGLOB_RST_SET_MSK);

#ifndef EMULATOR
  console_printf("waiting for PLL to be locked\n");
  WaitPllLocked ();
  console_printf("PLL is locked\n");
  
#endif
  /* 7.1
   * Dividers for C2 to C9 only init after PLLs are lock. We will a large
   * dividers value then final value as requested by hardware behaviour
   */
  // reset value
  //mainpll.c0 - pll0_mpu_base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                  ALT_CLKMGR_MAINPLL_MPUCLK_OFST,
                  0xff);
  // mainpll.c1 - pll0_noc _base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCCLK_OFST,
               0xff);
  // mainpll.c2 - pll0_emaca_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR2CLK_OFST,
              0xff);
  // mainpll.c3 - pll0_emacb_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR3CLK_OFST,
               0xff);
  // mainpll.c4 - pll0_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR4CLK_OFST,
               0xff);
  // mainpll.c5 - pll0_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR5CLK_OFST,
              0xff);
  // mainpll.c6 - pll0_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST,
               0xff);
  // mainpll.c7 - pll0_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR7CLK_OFST,
              0xff);
  // mainpll.c8 - pll0_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR8CLK_OFST,
              0xff);
  // mainpll.c9 - pll0_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR9CLK_OFST,
              0xff);
  // reset value
  //------------------------------------------------
   // perpll.c2 - pll1_emaca_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR2CLK_OFST,
               0xff);
  // perpll.c3 - pll1_emacb_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR3CLK_OFST,
                0xff);
  // perpll.c4 - pll1_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR4CLK_OFST,
                0xff);
  // perpll.c5 - pll1_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR5CLK_OFST,
                0xff);
  // perpll.c6 - pll1_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR6CLK_OFST,
                0xff);
  // perpll.c7 - pll1_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR7CLK_OFST,
                0xff);
  // perpll.c8 - pll1_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR8CLK_OFST,
               0xff);
  // perpll.c9 - pll1_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR9CLK_OFST,
               0xff);
  // setting value
  //mainpll.c0 - pll0_mpu_base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_MPUCLK_OFST,
               ClkCfg.mainpll.mpuclk);
  // mainpll.c1 - pll0_noc _base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCCLK_OFST,
               ClkCfg.mainpll.nocclk);
  // mainpll.c2 - pll0_emaca_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR2CLK_OFST,
               ClkCfg.mainpll.cntr2clk);
  // mainpll.c3 - pll0_emacb_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR3CLK_OFST,
               ClkCfg.mainpll.cntr3clk);
  // mainpll.c4 - pll0_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR4CLK_OFST,
               ClkCfg.mainpll.cntr4clk);
  // mainpll.c5 - pll0_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR5CLK_OFST,
               ClkCfg.mainpll.cntr5clk);
  // mainpll.c6 - pll0_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST,
               ClkCfg.mainpll.cntr6clk);
  // mainpll.c7 - pll0_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR7CLK_OFST,
               ClkCfg.mainpll.cntr7clk);
  // mainpll.c8 - pll0_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR8CLK_OFST,
               ClkCfg.mainpll.cntr8clk);
  // mainpll.c9 - pll0_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR9CLK_OFST,
               ClkCfg.mainpll.cntr9clk);

 // 7.2 Peripheral PLL Clock Source and Counters/Divider

  // Setting value
  // perpll.c2 - pll1_emaca_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR2CLK_OFST,
               ClkCfg.perpll.cntr2clk);
  // perpll.c3 - pll1_emacb_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR3CLK_OFST,
               ClkCfg.perpll.cntr3clk);
  // perpll.c4 - pll1_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR4CLK_OFST,
               ClkCfg.perpll.cntr4clk);
  // perpll.c5 - pll1_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR5CLK_OFST,
               ClkCfg.perpll.cntr5clk);
  // perpll.c6 - pll1_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR6CLK_OFST,
               ClkCfg.perpll.cntr6clk);
  // perpll.c7 - pll1_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR7CLK_OFST,
               ClkCfg.perpll.cntr7clk);
  // perpll.c8 - pll1_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR8CLK_OFST,
               ClkCfg.perpll.cntr8clk);
  // perpll.c9 - pll1_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR9CLK_OFST,
              ClkCfg.perpll.cntr9clk);

  //==============================================================
#ifndef EMULATOR
  // 8. Bring PLL out of bypass
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_BYPASS_OFST,
               0x0);
  WaitFsm();
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_BYPASS_OFST,
               0x0);
  WaitFsm();
      // 9. Set safe mode/ out of boot mode
  mmio_clrbits_32 (ALT_CLKMGR_OFST +
               ALT_CLKMGR_CTRL_OFST,
               ALT_CLKMGR_CTRL_BOOTMODE_SET_MSK);
  WaitFsm();

#endif
  // 10 Enable mainpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_EN_OFST,
                 ALT_CLKMGR_MAINPLL_EN_RESET);
  // Enable perpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_EN_OFST,
                 ALT_CLKMGR_PERPLL_EN_RESET);

  // 11 Clear loss lock  interrupt status register that might be set during configuration
  mmio_write_32 (ALT_CLKMGR_OFST +
                 ALT_CLKMGR_INTRCLR_OFST,
                 ALT_CLKMGR_INTRCLR_MAINLOCKLOST_SET_MSK |
                 ALT_CLKMGR_INTRCLR_PERLOCKLOST_SET_MSK);
}

void ConfigureClockManagerHandoff (handoff *hoff_ptr)
{
  uint32_t Mdiv;
  uint32_t Refclkdiv;
  uint32_t Mscnt;
  uint32_t Hscnt;

  console_printf("Configure Clock Manager handoff\n");

  // 1. Disable mainpllgrp's software-managed clock
  /*mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_ENR_OFST,
               ALT_CLKMGR_MAINPLL_ENR_S2FUSER0CLKEN_SET_MSK);
  // Disable perpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_ENR_OFST,
               ALT_CLKMGR_PERPLL_ENR_RESET);
  */
  // 2. Bypass Register has bypass, bypassS (set), bypassR (clear)
  // Bypassed all mainpllgrp's clocks
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_BYPASS_OFST,
               0x7);
  WaitFsm();
  // Bypassed all perpllgrp's clocks
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_BYPASS_OFST,
               0x7f);
  WaitFsm();
  // 3. Setup main PLL dividers
  // Calculate the vcocalib value
  Mdiv = ALT_CLKMGR_MAINPLL_FDBCK_MDIV_GET(hoff_ptr->main_pll_fdbck);
  Refclkdiv = ALT_CLKMGR_MAINPLL_PLLGLOB_REFCLKDIV_GET(hoff_ptr->main_pll_pllglob);
  Mscnt = 200 / ((6 + Mdiv) / Refclkdiv);
  Hscnt = (Mdiv + 6) * Mscnt / Refclkdiv - 9;

  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLGLOB_OFST,
                 hoff_ptr->main_pll_pllglob);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_FDBCK_OFST,
                 hoff_ptr->main_pll_fdbck);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_VCOCALIB_OFST,
                 ALT_CLKMGR_MAINPLL_VCOCALIB_HSCNT_SET(Hscnt) |
				 ALT_CLKMGR_MAINPLL_VCOCALIB_MSCNT_SET(Mscnt));
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLC0_OFST,
                 hoff_ptr->main_pll_pllc0);
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_PLLC1_OFST,
                 hoff_ptr->main_pll_pllc1);

  // mainpll's NoC Clocks's divider
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCDIV_OFST,
               hoff_ptr->main_pll_nocdiv);

  // 4. Setup peripheral PLL dividers
  // Calculate the vcocalib value
  Mdiv = ALT_CLKMGR_PERPLL_FDBCK_MDIV_GET(hoff_ptr->per_pll_fdbck);
  Refclkdiv = ALT_CLKMGR_PERPLL_PLLGLOB_REFCLKDIV_GET(hoff_ptr->per_pll_pllglob);
  Mscnt = 200 / ((6 + Mdiv) / Refclkdiv);
  Hscnt = (Mdiv + 6) * Mscnt / Refclkdiv - 9;

  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLGLOB_OFST,
                 hoff_ptr->per_pll_pllglob);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_FDBCK_OFST,
                 hoff_ptr->per_pll_fdbck);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_VCOCALIB_OFST,
                 ALT_CLKMGR_PERPLL_VCOCALIB_HSCNT_SET(Hscnt) |
				 ALT_CLKMGR_PERPLL_VCOCALIB_MSCNT_SET(Mscnt));
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLC0_OFST,
                 0x8000002);//hoff_ptr->per_pll_pllc0);
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_PLLC1_OFST,
                 hoff_ptr->per_pll_pllc1);

  // Init GPIO De-bounce Clock Divider
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_GPIODIV_OFST,
               ALT_CLKMGR_PERPLL_GPIODIV_GPIODBCLK_SET(hoff_ptr->per_pll_gpiodiv));
  // Select EMAC clock source
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_EMACCTL_OFST,
               hoff_ptr->per_pll_emacctl);


  // 5. Take both PLL out of reset and power up
  mmio_setbits_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_PLLGLOB_OFST,
               ALT_CLKMGR_MAINPLL_PLLGLOB_PD_SET_MSK |
			   ALT_CLKMGR_MAINPLL_PLLGLOB_RST_SET_MSK);
  mmio_setbits_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_PLLGLOB_OFST,
               ALT_CLKMGR_PERPLL_PLLGLOB_PD_SET_MSK |
			   ALT_CLKMGR_PERPLL_PLLGLOB_RST_SET_MSK);

#ifndef EMULATOR
  WaitPllLocked ();
#endif
  /* 7.1
   * Dividers for C2 to C9 only init after PLLs are lock. We will a large
   * dividers value then final value as requested by hardware behaviour
   */
  // reset value
  //mainpll.c0 - pll0_mpu_base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                  ALT_CLKMGR_MAINPLL_MPUCLK_OFST,
                  0xff);
  // mainpll.c1 - pll0_noc _base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCCLK_OFST,
               0xff);
  // mainpll.c2 - pll0_emaca_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR2CLK_OFST,
              0xff);
  // mainpll.c3 - pll0_emacb_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR3CLK_OFST,
               0xff);
  // mainpll.c4 - pll0_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR4CLK_OFST,
               0xff);
  // mainpll.c5 - pll0_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR5CLK_OFST,
              0xff);
  // mainpll.c6 - pll0_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST,
               0xff);
  // mainpll.c7 - pll0_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR7CLK_OFST,
              0xff);
  // mainpll.c8 - pll0_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR8CLK_OFST,
              0xff);
  // mainpll.c9 - pll0_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR9CLK_OFST,
              0xff);
    // perpll.c2 - pll1_emaca_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR2CLK_OFST,
               0xff);
  // perpll.c3 - pll1_emacb_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR3CLK_OFST,
                0xff);
  // perpll.c4 - pll1_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR4CLK_OFST,
                0xff);
  // perpll.c5 - pll1_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR5CLK_OFST,
                0xff);
  // perpll.c6 - pll1_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR6CLK_OFST,
                0xff);
  // perpll.c7 - pll1_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR7CLK_OFST,
                0xff);
  // perpll.c8 - pll1_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR8CLK_OFST,
               0xff);
  
  // setting value
  //mainpll.c0 - pll0_mpu_base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_MPUCLK_OFST,
               hoff_ptr->main_pll_mpuclk);
  // mainpll.c1 - pll0_noc _base_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_NOCCLK_OFST,
               hoff_ptr->main_pll_nocclk);
  // mainpll.c2 - pll0_emaca_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR2CLK_OFST,
               hoff_ptr->main_pll_cntr2clk);
  // mainpll.c3 - pll0_emacb_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR3CLK_OFST,
               hoff_ptr->main_pll_cntr3clk);
  // mainpll.c4 - pll0_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR4CLK_OFST,
               hoff_ptr->main_pll_cntr4clk);
  // mainpll.c5 - pll0_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR5CLK_OFST,
               hoff_ptr->main_pll_cntr5clk);
  // mainpll.c6 - pll0_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST,
               hoff_ptr->main_pll_cntr6clk);
  // mainpll.c7 - pll0_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR7CLK_OFST,
               hoff_ptr->main_pll_cntr7clk);
  // mainpll.c8 - pll0_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR8CLK_OFST,
               hoff_ptr->main_pll_cntr8clk);
  // mainpll.c9 - pll0_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_CNTR9CLK_OFST,
               hoff_ptr->main_pll_cntr9clk);

 // 7.2 Peripheral PLL Clock Source and Counters/Divider
  //------------------------------------------------
   // Setting value
  // perpll.c2 - pll1_emaca_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR2CLK_OFST,
               hoff_ptr->per_pll_cntr2clk);
  // perpll.c3 - pll1_emacb_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR3CLK_OFST,
               hoff_ptr->per_pll_cntr3clk);
  // perpll.c4 - pll1_emac_ptp_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR4CLK_OFST,
               hoff_ptr->per_pll_cntr4clk);
  // perpll.c5 - pll1_gpio_db_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR5CLK_OFST,
               hoff_ptr->per_pll_cntr5clk);
  // perpll.c6 - pll1_sdmmc_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR6CLK_OFST,
               hoff_ptr->per_pll_cntr6clk);
  // perpll.c7 - pll1_h2f_user0_clk (A.K.A. s2f_user0_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR7CLK_OFST,
               hoff_ptr->per_pll_cntr7clk);
  // perpll.c8 - pll1_h2f_user1_clk (A.K.A. s2f_user1_clk)
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR8CLK_OFST,
               hoff_ptr->per_pll_cntr8clk);
  // perpll.c9 - pll1_hmc_pll_ref_clk
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_CNTR9CLK_OFST,
              hoff_ptr->per_pll_cntr9clk);

  //==============================================================
#ifndef EMULATOR
  // 8. Take all PLLs out of bypass
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
               ALT_CLKMGR_MAINPLL_BYPASS_OFST,
               0);
  WaitFsm();
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
               ALT_CLKMGR_PERPLL_BYPASS_OFST,
               0);
  WaitFsm();
  // 9. Set safe mode/ out of boot mode
  mmio_clrbits_32 (ALT_CLKMGR_OFST +
               ALT_CLKMGR_CTRL_OFST,
               ALT_CLKMGR_CTRL_BOOTMODE_SET_MSK);
  WaitFsm();
#endif
  // 10 Enable mainpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_MAINPLL_OFST +
                 ALT_CLKMGR_MAINPLL_EN_OFST,
                 ALT_CLKMGR_MAINPLL_EN_RESET);
  // Enable perpllgrp's software-managed clock
  mmio_write_32 (ALT_CLKMGR_PERPLL_OFST +
                 ALT_CLKMGR_PERPLL_EN_OFST,
                 ALT_CLKMGR_PERPLL_EN_RESET);

  // 11 Clear loss lock  interrupt status register that might be set during configuration
  mmio_write_32 (ALT_CLKMGR_OFST +
                 ALT_CLKMGR_INTRCLR_OFST,
                 ALT_CLKMGR_INTRCLR_MAINLOCKLOST_SET_MSK |
                 ALT_CLKMGR_INTRCLR_PERLOCKLOST_SET_MSK);
}


void WaitPllLocked (void)
{
  uint32_t Data32;
  do {
    Data32 = mmio_read_32 (ALT_CLKMGR_OFST +
                         ALT_CLKMGR_STAT_OFST);
  } while ((ALT_CLKMGR_STAT_MAINPLLLOCKED_GET(Data32) == 0) ||
           (ALT_CLKMGR_STAT_PERPLLLOCKED_GET(Data32) == 0));
}

void DisplayClockManagerInfo (void)
{
  unsigned int         i;

  console_printf("Clock Manager Register Info:\r\n");
  console_printf("mainpll.mpuclk   :\t0x%x\r\n"
    "\t mainpll.nocclk   :\t0x%x\r\n"
    "\t mainpll.nocdiv   :\t0x%x\r\n"
    "\t mainpll.pllc0   :\t0x%x\r\n",
    mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_MPUCLK_OFST),
    mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_NOCCLK_OFST),
	mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_NOCDIV_OFST),
    mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLC0_OFST));
  console_printf("\t mainpll.pllc1 is 0x%x\r\n", mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLC1_OFST));
  for (i = 0; i < 8; i++)
  {
    console_printf("\t mainpll.cntr%dclk :\t0x%x\r\n", (i + 2),
      mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_CNTR2CLK_OFST + (i * 4)));
  }

  console_printf("\t perpll.pllc0      :\t0x%x\r\n"
    "\t perpll.pllc1      :\t0x%x\r\n",
    mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLC0_OFST),
    mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLC1_OFST));

  for (i = 0; i < 8; i++)
  {
    console_printf("\t perpll.cntr%dclk  :\t0x%x\r\n", (i + 2),
      mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR2CLK_OFST + (i * 4)));
  }

  console_printf("\t perpll.emacctl   :\t0x%x\r\n"
    "\t perpll.gpiodiv   :\t0x%x\r\n",
     mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_EMACCTL_OFST),
     mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_GPIODIV_OFST));

  DisplayClockFrequencyInfo();
}

void DisplayClockFrequencyInfo (void)
{
  uint32_t  Data32;
  uint32_t  main_ref_clk;
  uint32_t  peri_ref_clk;
  // Boot Clock
  // ----------
  // The status of the hps_clk_f security fuse in the Security Manager determines if boot_clk should be secure:
  // If set, then boot_clk is cb_intosc_hs_clk divided by 2.
  // If clear, then boot_clk is EOSC1.
  console_printf(
    "\t List of possible Boot Clock Source:\r\n"
    "\t\t EOSC1            : %d MHz\r\n"
    "\t\t cb_intosc_hs_clk : 120-400 MHz \r\n",
     mClkSrc.clk_freq_of_eosc1/1000/1000);

  // If swctrlbtclken set, then Software will take control of the boot_clk mux select else it will follow fuse.
  Data32 = mmio_read_32 (ALT_CLKMGR_OFST + ALT_CLKMGR_CTRL_OFST);
  if (ALT_CLKMGR_CTRL_SWCTRLBTCLKEN_GET(Data32) == 1) {
      console_printf("\t\t Software took control of the boot_clk mux select\r\n");
    if (ALT_CLKMGR_CTRL_SWCTRLBTCLKSEL_GET(Data32) == 1)
      console_printf("\t\t The boot_clk is cb_intosc_hs_clk divided by 2\r\n");
    else
      console_printf("\t\t The boot_clk is EOSC1\r\n");
  } else {
    Data32 = mmio_read_32 (ALT_CLKMGR_OFST + ALT_CLKMGR_STAT_OFST);
    if (ALT_CLKMGR_STAT_BOOTCLKSRC_GET(Data32) == 1)
      console_printf("\t\t The boot_clk is secured cb_intosc_hs_clk divided by 2\r\n");
    else
      console_printf("\t\t The boot_clk is EOSC1\r\n");
  }

  console_printf("\t List of possible Clock Manager PLLs Clock Source:\r\n"
    "\t\t EOSC1                 : %d MHz\r\n"
    "\t\t cb_intosc_ls_clk      : 30-100 MHz\r\n"
    "\t\t f2h_free_clk          : %d MHz\r\n",
    mClkSrc.clk_freq_of_eosc1/1000000,
    mClkSrc.clk_freq_of_f2h_free/1000000);

  console_printf("Display VCO REF CLK\r\n");
  main_ref_clk = GetMainRefClock ();
  peri_ref_clk  = GetPeriRefClock ();
  console_printf("\t\tmain_ref_clk (Main VCO REF Clock : %d\r\n", main_ref_clk);
  console_printf("\t\tperi_ref_clk (Peripheral VCO REF Clock : %d\r\n", peri_ref_clk);
  DisplayMpuClockInfo (main_ref_clk, peri_ref_clk);
  DisplayNocClockInfo (main_ref_clk, peri_ref_clk);
  DisplayPllClockOuput (main_ref_clk, peri_ref_clk);
}

uint32_t GetPeriRefClock (void)
{
  uint32_t data32, refclkdiv, mdiv, ref_clk;

  console_printf("\t\t The peri_ref_clk (perivco) is ");
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLGLOB_OFST);
  switch (ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_GET(data32))
  {
    case ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_E_EOSC1:
      ref_clk = mClkSrc.clk_freq_of_eosc1 / 1000000;
      console_printf("EOSC1\r\n");
      break;
    case ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_E_INTOSC:
      ref_clk = mClkSrc.clk_freq_of_cb_intosc_ls / 1000000;
      console_printf("cb_intosc_ls_clk\r\n");
      break;
    case ALT_CLKMGR_PERPLL_PLLGLOB_PSRC_E_F2S:
      ref_clk = mClkSrc.clk_freq_of_f2h_free / 1000000;
      console_printf("f2h_free_clk\r\n");
      break;
    default:
      // We should have covered all supported conditions above
      ref_clk = 0;
      assert(0);
      break;
  }

  refclkdiv = ALT_CLKMGR_PERPLL_PLLGLOB_REFCLKDIV_GET(data32);
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_FDBCK_OFST);
  mdiv = ALT_CLKMGR_PERPLL_FDBCK_MDIV_GET(data32);

  ref_clk = (ref_clk / refclkdiv) * (6 + mdiv);
  return ref_clk;
}

uint32_t GetMainRefClock (void)
{
  uint32_t data32, refclkdiv, mdiv, ref_clk;

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLGLOB_OFST);
  switch (ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_GET(data32))
  {
    case ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_E_EOSC1:
      ref_clk = mClkSrc.clk_freq_of_eosc1 / 1000000;
      break;
    case ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_E_INTOSC:
      ref_clk = mClkSrc.clk_freq_of_cb_intosc_ls / 1000000;
      break;
    case ALT_CLKMGR_MAINPLL_PLLGLOB_PSRC_E_F2S:
      ref_clk = mClkSrc.clk_freq_of_f2h_free / 1000000;
      break;
    default:
      // We should have covered all possible conditions above
      ref_clk = 0;
      assert(0);
      break;
  }
  refclkdiv = ALT_CLKMGR_MAINPLL_PLLGLOB_REFCLKDIV_GET(data32);
  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_FDBCK_OFST);
  mdiv = ALT_CLKMGR_MAINPLL_FDBCK_MDIV_GET(data32);

  ref_clk = (ref_clk / refclkdiv) * (6 + mdiv);
  return ref_clk;
}

void DisplayPllClockOuput (uint32_t main_ref_clk, uint32_t peri_ref_clk)
{
  uint32_t data32;
  uint32_t  emaca_clk;
  uint32_t  emacb_clk;
  uint32_t  emac_ptp_clk;
  uint32_t  gpio_db_clk;
  uint32_t  sdmmc_clk;
  uint32_t  s2f_user0_clk;
  uint32_t  s2f_user1_clk;
  uint32_t  psi_ref_clk;
  uint32_t  peri_noc_base_clk;
  uint32_t  main_noc_base_clk;

  emaca_clk = 0;
  emacb_clk = 0;
  emac_ptp_clk = 0;
  gpio_db_clk = 0;
  sdmmc_clk = 0;
  s2f_user0_clk = 0;
  s2f_user1_clk = 0;
  psi_ref_clk = 0;

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLC1_OFST);
  main_noc_base_clk = main_ref_clk / ALT_CLKMGR_MAINPLL_PLLC1_DIV_GET(data32);
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLC1_OFST);
  peri_noc_base_clk = peri_ref_clk / ALT_CLKMGR_PERPLL_PLLC1_DIV_GET(data32);
  // Main PLL output clock
  // ----------
  console_printf("\t Main Output Clock Frequency: \r\n");
  // emaca_clk
  console_printf("\t\t main_emaca_clk       (C2): Unused\n");
  console_printf("\t\t main_emacb_clk       (C3): Unused\n");
  console_printf("\t\t main_emac_ptp_clk    (C4): Unused\n");

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_CNTR5CLK_OFST);
  gpio_db_clk = (main_noc_base_clk) / (ALT_CLKMGR_MAINPLL_CNTR6CLK_CNT_GET(data32) + 1);
  console_printf("\t\t main_gpio_db_clk     (C5):%d Mhz\n", gpio_db_clk);

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST);
  sdmmc_clk = (main_noc_base_clk) / (ALT_CLKMGR_MAINPLL_CNTR6CLK_CNT_GET(data32) + 1);
  console_printf("\t\t main_sdmmc_clk       (C6): %d MHz\n", sdmmc_clk);
  console_printf("\t\t main_s2f_user0_clk   (C7):  Unused\n");
  console_printf("\t\t main_h2f_user1_clk   (C8):  Unused\n");
  console_printf("\t\t main_psi_ref_clk     (C9):  Unused\n");

  // Peripheral output frequency
  console_printf("\t Peripheral Output Clock Frequency: \r\n");
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR2CLK_OFST);
  emaca_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR2CLK_CNT_GET(data32) + 1);
  console_printf("\t\t peri_emaca_clk       (C2): %d MHz\r\n", emaca_clk);

  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR3CLK_OFST);
  emacb_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR3CLK_CNT_GET(data32) + 1);
  console_printf("\t\t peri_emacb_clk       (C3): %d MHz\r\n", emacb_clk);

  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR4CLK_OFST);
  emac_ptp_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR4CLK_CNT_GET(data32) + 1);
  console_printf("\t\t peri_emacb_clk       (C4): %d MHz\r\n", emac_ptp_clk);
  console_printf( "\t\t peri_gpio_db_clk     (C5): Unused\r\n");
  console_printf( "\t\t peri_sdmmc_clk       (C6): Unused\r\n");

  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR7CLK_OFST);
  s2f_user0_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR7CLK_CNT_GET(data32) + 1);
  console_printf("\t\t peri_s2f_user0_clk       (C7): %d MHz\r\n", s2f_user0_clk);

  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR8CLK_OFST);
  s2f_user1_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR8CLK_CNT_GET(data32) + 1);
  console_printf("\t\t peri_s2f_user1_clk       (C8): %d MHz\r\n", s2f_user1_clk);

  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_CNTR9CLK_OFST);
  psi_ref_clk = (peri_noc_base_clk) / (ALT_CLKMGR_PERPLL_CNTR9CLK_CNT_GET(data32) + 1);
  console_printf("\t\tperi_psi_ref_clk (C9): %d MHz\r\n", psi_ref_clk);

}

void DisplayMpuClockInfo (uint32_t main_ref_clk, uint32_t peri_ref_clk)
{
  uint32_t  main_mpu_base_clk;
  uint32_t  peri_mpu_base_clk;
  uint32_t  mpu_free_clk;
  uint32_t  mpu_clk;
  uint32_t  mpu_ccu_clk;
  uint32_t  mpu_periph_clk;
  uint32_t data32;

  //======= MPU & NOC clock=======================================
  console_printf ("\t MPU and NOC Clock Group : \r\n");
  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLC0_OFST);
  main_mpu_base_clk = main_ref_clk / ALT_CLKMGR_MAINPLL_PLLC0_DIV_GET(data32);
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLC0_OFST);
  peri_mpu_base_clk = peri_ref_clk / ALT_CLKMGR_PERPLL_PLLC0_DIV_GET(data32);

  console_printf ("\t\t main_mpu_base_clk       : %d MHz\r\n"
            "\t\t peri_mpu_base_clk      : %d MHz\r\n",
	         main_mpu_base_clk,
			 peri_mpu_base_clk);

  console_printf ("\t\t mpu_free_clk clock source is ");
  // default value
  mpu_free_clk = main_mpu_base_clk;
  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_MPUCLK_OFST);
  switch (ALT_CLKMGR_MAINPLL_MPUCLK_SRC_GET(data32))
  {
    case ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_MAIN:
      console_printf ("main_mpu_base_clk\r\n");
	  mpu_free_clk = main_mpu_base_clk;
      break;
    case ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_PERI:
      console_printf ("peri_mpu_base_clk\r\n");
	  mpu_free_clk = peri_mpu_base_clk;;
      break;
    case ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_OSC1:
      console_printf ("EOSC1 pin\r\n");
      mpu_free_clk = mClkSrc.clk_freq_of_eosc1/1000/1000;
      break;
    case ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_INTOSC:
      console_printf ("cb_intosc_hs_div2_clk (60-200 MHz)\r\n");
      mpu_free_clk = 100;
      break;
    case ALT_CLKMGR_MAINPLL_MPUCLK_SRC_E_FPGA:
      console_printf ("h2f_free_clk\r\n");
      mpu_free_clk = mClkSrc.clk_freq_of_f2h_free/1000/1000;
      break;
    default:
      // We should have covered all supported conditions above
      assert(0);
      break;
  }
  mpu_clk = mpu_free_clk / (ALT_CLKMGR_MAINPLL_MPUCLK_CNT_GET(data32) + 1);
  mpu_ccu_clk = mpu_clk/2;
  mpu_periph_clk = mpu_clk/4;
  console_printf ("\t\t mpu_free_clk          : %d MHz\r\n"
		   "\t\t mpu_clk               : %d MHz\r\n"
           "\t\t mpu_ccu_clk           : %d MHz\r\n",
		    mpu_free_clk,
			mpu_clk,
            mpu_ccu_clk);
  console_printf ("\t\t mpu_periph_clk        : %d MHz\r\n",mpu_periph_clk);
}

void DisplayNocClockInfo(uint32_t main_ref_clk, uint32_t peri_ref_clk)
{
  uint32_t data32;
  uint32_t  peri_noc_base_clk;
  uint32_t  main_noc_base_clk;
  uint32_t  noc_free_clk;
  uint32_t  cs_at_clk;
  uint32_t  cs_pdbg_clk;
  uint32_t  cs_trace_clk;
  uint32_t  cs_timer_clk;
  uint32_t  l3_main_free_clk;
  uint32_t  l4_sys_free_clk;
  uint32_t  l4_main_clk;
  uint32_t  l4_mp_clk;
  uint32_t  l4_sp_clk;
  uint32_t sdmmc_clk;
  uint32_t  Div1248[4] = {1, 2, 4, 8};

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_PLLC1_OFST);
  main_noc_base_clk = main_ref_clk / ALT_CLKMGR_MAINPLL_PLLC1_DIV_GET(data32);
  data32 = mmio_read_32 (ALT_CLKMGR_PERPLL_OFST + ALT_CLKMGR_PERPLL_PLLC1_OFST);
  peri_noc_base_clk = peri_ref_clk / ALT_CLKMGR_PERPLL_PLLC1_DIV_GET(data32);

  console_printf ("\t\t main_noc_base_clk      : %d Mhz\r\n"
            "\t\t peri_noc_base_clk      : %d MHz\r\n",
			 main_noc_base_clk,
             peri_noc_base_clk);
  console_printf ("\t\t noc_free_clk clock source is ");
  // default value
  noc_free_clk = main_noc_base_clk;
  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_NOCCLK_OFST);
  switch (ALT_CLKMGR_MAINPLL_NOCCLK_SRC_GET(data32))
  {
    case ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_MAIN:
      console_printf ("main_noc_base_clk\r\n");
	  noc_free_clk = main_noc_base_clk;
      break;
    case ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_PERI:
      console_printf ("peri_noc_base_clk\r\n");
	  noc_free_clk = peri_noc_base_clk;;
      break;
    case ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_OSC1:
      console_printf ("EOSC1 pin\r\n");
      noc_free_clk = mClkSrc.clk_freq_of_eosc1/1000/1000;
      break;
    case ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_INTOSC:
      console_printf ("cb_intosc_hs_div2_clk (60-200 MHz)\r\n");
      noc_free_clk = 100;
      break;
    case ALT_CLKMGR_MAINPLL_NOCCLK_SRC_E_FPGA:
      console_printf ("h2f_free_clk\r\n");
      noc_free_clk = mClkSrc.clk_freq_of_f2h_free/1000/1000;
      break;
    default:
      // We should have covered all supported conditions above
      assert(0);
      break;
  }

  l3_main_free_clk = noc_free_clk / (ALT_CLKMGR_MAINPLL_NOCCLK_CNT_GET(data32) + 1);
  l4_sys_free_clk = l3_main_free_clk / 4;

  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_NOCDIV_OFST);
  l4_main_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_L4MAINCLK_GET(data32)];
  l4_mp_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_L4MPCLK_GET(data32)];
  l4_sp_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_L4SPCLK_GET(data32)];
  cs_at_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_CSATCLK_GET(data32)];
  cs_pdbg_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_CSPDBGCLK_GET(data32)];
  cs_trace_clk = l3_main_free_clk / Div1248[ALT_CLKMGR_MAINPLL_NOCDIV_CSTRACECLK_GET(data32)];
  cs_timer_clk = cs_at_clk;

  console_printf ("\t NOC Clock Group : \r\n"
             "\t\t noc_free_clk clock source is \r\n"
             "\t\t l3_main_free_clk      : %d MHz\r\n"
             "\t\t l4_sys_free_clk       : %d MHz\r\n"
             "\t\t l4_main_clk           : %d MHz\r\n"
             "\t\t l4_mp_clk             : %d MHz\r\n"
             "\t\t l4_sp_clk             : %d MHz\r\n"
             "\t\t cs_at_clk             : %d MHz\r\n"
             "\t\t cs_timer_clk          : %d MHz\r\n"
             "\t\t cs_pdbg_clk           : %d MHz\r\n"
             "\t\t cs_trace_clk          : %d MHz\r\n",
             l3_main_free_clk,
             l4_sys_free_clk,
             l4_main_clk,
             l4_mp_clk,
             l4_sp_clk,
             cs_at_clk,
             cs_timer_clk,
             cs_pdbg_clk,
             cs_trace_clk);

  console_printf ("\t HPS Managerial Blocks : \r\n"
             "\t\t Clock Manager         - l4_sys_free_clk    : %d MHz\r\n"
             "\t\t FPGA Manager          - l4_sys_free_clk    : %d MHz\r\n"
             "\t\t Reset Manager         - l4_sys_free_clk    : %d MHz\r\n"
             "\t\t System Manager        - l4_sys_free_clk    : %d MHz\r\n"
             "\t\t Security Manager      - cb_intosc_hs_clk   : 120-400 MHz\r\n",
             l4_sys_free_clk,
             l4_sys_free_clk,
             l4_sys_free_clk,
             l4_sys_free_clk);

  console_printf ("\t Memory related Blocks : \r\n"
             "\t\t Boot ROM              - l3_main_free_clk   : %d MHz\r\n"
             "\t\t On Chip RAM           - l3_main_free_clk   : %d MHz\r\n"
             "\t\t DMA Controller        - l4_main_clk        : %d MHz\r\n"
             "\t\t SDRAM L3 Interconnect - hmc_free_clk       : HMC's clock\r\n"
             "\t\t FPGA-to-SDRAM         - f2h_sdram_clk[2:0] : DDR's clock\r\n",
             l3_main_free_clk,
             l3_main_free_clk,
             l4_main_clk);

  console_printf ("\t Bridges Blocks : \r\n"
             "\t\t FPGA-to-HPS           - fpga2hps_clk \r\n"
             "\t\t HPS-to-FPGA           - hps2fpga_clk \r\n"
             "\t\t HPS-to-FPGA LW        - lwh2fpga_clk \r\n");

  console_printf ("\t Timer Peripherals : \r\n"
             "\t\t OSC Timer 0/1         - l4_sys_free_clk    : %d MHz\r\n"
             "\t\t SP Timer 0/1          - l4_sp_clk          : %d MHz\r\n"
             "\t\t Watchdog Timer 0/1    - l4_sys_free_clk    : %d MHz\r\n",
             l4_sys_free_clk,
             l4_sp_clk,
             l4_sys_free_clk);

  console_printf ("\t Interface Peripherals : \r\n"
             "\t\t Ethernet MAC 0/1/2    - l4_mp_clk          : %d MHz\r\n"
             "\t\t USB 2.0 On-The-Go 0/1 - l4_mp_clk          : %d MHz\r\n"
             "\t\t I2C Controllers 0-4   - l4_sp_clk          : %d MHz\r\n"
             "\t\t UART Controllers 0/1  - l4_sp_clk          : %d MHz\r\n"
             "\t\t SPI Master 0/1        - l4_main_clk        : %d MHz\r\n"
             "\t\t SPI Slave 0/1         - l4_main_clk        : %d MHz\r\n"
             "\t\t GPIO Controllers 0-2  - l4_sp_clk          : %d MHz\r\n",
             l4_mp_clk,
             l4_mp_clk,
             l4_sp_clk,
             l4_sp_clk,
             l4_main_clk,
             l4_main_clk,
             l4_sp_clk);

  console_printf ("\t NAND Controller : \r\n"
             "\t\t NAND Input Clock      - l4_mp_clk          : %d MHz\r\n"
             "\t\t NAND Output Clock     - nand_clk           : %d MHz\r\n"
             "\t\t NAND Output Clock     - nand_x_clk         : %d MHz\r\n",
             l4_mp_clk,
             l4_mp_clk / 4,
             l4_mp_clk);
  data32 = mmio_read_32 (ALT_CLKMGR_MAINPLL_OFST + ALT_CLKMGR_MAINPLL_CNTR6CLK_OFST);
  sdmmc_clk = (main_noc_base_clk) / (ALT_CLKMGR_MAINPLL_CNTR6CLK_CNT_GET(data32) + 1);
  console_printf ("\t SD/MMC Controller : \r\n"
             "\t\t SD/MMC Input Clock    - l4_mp_clk          : %d MHz\r\n"
             "\t\t SD/MMC Input Clock    - sdmmc_clk          : %d MHz\r\n"
             "\t\t SD/MMC CIU Clock      - cclk_in            : %d MHz\r\n"
             "\t\t SD/MMC Card Clock     - cclk_out           : cclk_in / (clk_divider0*2)\r\n",
             l4_mp_clk,
             sdmmc_clk,
             sdmmc_clk / 4);
}