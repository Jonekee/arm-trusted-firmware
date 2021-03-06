/*
 * Copyright (c) 2013-2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <arch.h>
#include <asm_macros.S>
#include <platform_def.h>

	.globl	plat_secondary_cold_boot_setup
	.globl	platform_is_primary_cpu
	.globl	plat_is_my_cpu_primary
	.globl	plat_my_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	platform_mem_init

	.globl plat_get_my_entrypoint
	.globl stratix10_sec_entry
	.globl cpuid_release

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	/* Wait until the it gets reset signal from rstmgr gets populated */
poll_mailbox:
	 wfi

	adr	x0, stratix10_sec_entry
	ldr	x1, [x0]
	adr	x2, cpuid_release
	ldr	x3, [x2]
	mrs	x4, mpidr_el1
	and	x4, x4, #0xff
	cmp	x3, x4
	b.ne	poll_mailbox
	br	x1
endfunc plat_secondary_cold_boot_setup

func platform_is_primary_cpu
	and	x0, x0, #(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
	cmp	x0, #PLAT_PRIMARY_CPU
	cset	x0, eq
	ret
endfunc platform_is_primary_cpu

func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	b   platform_is_primary_cpu
endfunc plat_is_my_cpu_primary

func plat_my_core_pos
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_my_core_pos

func plat_get_my_entrypoint
	adr	x1,stratix10_sec_entry
	ldr	x0, [x1]
	ret
endfunc plat_get_my_entrypoint

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, PLAT_UART0_BASE
	mov_imm	x1, PLAT_UART_CLOCK
	mov_imm	x2, PLAT_BAUDRATE
	b	console_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(void)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm x1, PLAT_UART0_BASE
	b	console_core_putc
endfunc plat_crash_console_putc


	/* --------------------------------------------------------
	 * void platform_mem_init (void);
	 *
	 * Any memory init, relocation to be done before the
	 * platform boots. Called very early in the boot process.
	 * --------------------------------------------------------
	 */
func platform_mem_init
	mov	x0, #0
	ret
endfunc platform_mem_init


	.data
	.align 3

stratix10_sec_entry:
	.quad 0

cpuid_release:
	.quad 0

