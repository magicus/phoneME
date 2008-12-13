/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

/*
 * Base address and offsets for the RPS peripherals (specifically:
 * timer 1) of the ARMulator.
 */

#define IntBase         0x0A000000  /* Interrupt Controller Base */
#define TimerBase       0x0A800000  /* Counter/Timer Base */

#define IRQStatus      ((volatile unsigned *)IntBase)
#define IRQRawStatus   ((volatile unsigned *)(IntBase + 0x04))
#define IRQEnable      ((volatile unsigned *)(IntBase + 0x08))
#define IRQEnableSet   ((volatile unsigned *)(IntBase + 0x08))
#define IRQEnableClear ((volatile unsigned *)(IntBase + 0x0c))
#define IRQSoft        ((volatile unsigned *)(IntBase + 0x10))

#define IRQTimer1       0x0010

#define Timer1Load       ((volatile unsigned *)TimerBase)
#define Timer1Value      ((volatile unsigned *)(TimerBase + 0x04))
#define Timer1Control    ((volatile unsigned *)(TimerBase + 0x08))
#define Timer1Clear      ((volatile unsigned *)(TimerBase + 0x0C))

#define TimerEnable      0x80
#define TimerPeriodic    0x40
#define TimerPrescale1   (0x00
#define TimerPrescale16  (0x01 << 2)
#define TimerPrescale256 (0x02 << 2)
#define TimerDisable     0
#define TimerCyclic      0x00

/*
 * Profiler control registers, as implemented by Sun's ADS-based
 * profiler. See internal_misc/ads_memprof/src/memprof.c.
 */
#define PROFILER_ATTACHED_REG        ((volatile int*)0x0d100000)
#define PROFILER_CYCLES64_MSW_REG    ((volatile int*)0x0d100004)
#define PROFILER_CYCLES64_LSW_REG    ((volatile int*)0x0d100008)
#define PROFILER_FREQUENCY_REG       ((volatile int*)0x0d10000c)
#define PROFILER_SUSPEND_REG         ((volatile int*)0x0d100010)
#define PROFILER_RESUME_REG          ((volatile int*)0x0d100014)
#define PROFILER_BEGIN_SEND_NAME_REG ((volatile int*)0x0d100018)
#define PROFILER_END_SEND_NAME_REG   ((volatile int*)0x0d10001c)
#define PROFILER_CPU_SPEED_REG       ((volatile int*)0x0d100020)
#define PROFILER_CACHE_EXISTS_REG    ((volatile int*)0x0d100024)

extern "C" {

void send_name_profiler(char *name, address a, int size);
void armulator_simulate_memory_transfer(juint size, int repeat);
void armulator_simulate_memory_change(juint size, int repeat);

}
