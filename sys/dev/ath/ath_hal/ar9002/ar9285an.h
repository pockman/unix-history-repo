/*
 * Copyright (c) 2008-2009 Sam Leffler, Errno Consulting
 * Copyright (c) 2008 Atheros Communications, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $FreeBSD$
 */

#ifndef	__AR9285_AN_H__
#define	__AR9285_AN_H__

/* AR9285 Analog register definitions */

#define	AR9285_AN_RF2G1		0x7820

#define	AR9285_AN_RF2G1_ENPACAL      0x00000800
#define	AR9285_AN_RF2G1_ENPACAL_S    11
#define	AR9285_AN_RF2G1_PDPADRV1     0x02000000
#define	AR9285_AN_RF2G1_PDPADRV1_S   25
#define	AR9285_AN_RF2G1_PDPADRV2     0x01000000
#define	AR9285_AN_RF2G1_PDPADRV2_S   24
#define	AR9285_AN_RF2G1_PDPAOUT      0x00800000
#define	AR9285_AN_RF2G1_PDPAOUT_S    23

#define	AR9285_AN_RF2G2		0x7824

#define	AR9285_AN_RF2G2_OFFCAL       0x00001000
#define	AR9285_AN_RF2G2_OFFCAL_S     12

#define	AR9285_AN_RF2G3		0x7828

#define	AR9285_AN_RF2G3_PDVCCOMP	0x02000000
#define	AR9285_AN_RF2G3_PDVCCOMP_S	25
#define	AR9285_AN_RF2G3_OB_0	0x00E00000
#define	AR9285_AN_RF2G3_OB_0_S	21
#define	AR9285_AN_RF2G3_OB_1	0x001C0000
#define	AR9285_AN_RF2G3_OB_1_S	18
#define	AR9285_AN_RF2G3_OB_2	0x00038000
#define	AR9285_AN_RF2G3_OB_2_S	15
#define	AR9285_AN_RF2G3_OB_3	0x00007000
#define	AR9285_AN_RF2G3_OB_3_S	12
#define	AR9285_AN_RF2G3_OB_4	0x00000E00
#define	AR9285_AN_RF2G3_OB_4_S	9
#define	AR9285_AN_RF2G3_DB1_0	0x000001C0
#define	AR9285_AN_RF2G3_DB1_0_S	6
#define	AR9285_AN_RF2G3_DB1_1	0x00000038
#define	AR9285_AN_RF2G3_DB1_1_S	3
#define	AR9285_AN_RF2G3_DB1_2	0x00000007
#define	AR9285_AN_RF2G3_DB1_2_S	0

#define	AR9285_AN_RF2G4		0x782C

#define	AR9285_AN_RF2G4_DB1_3	0xE0000000
#define	AR9285_AN_RF2G4_DB1_3_S	29
#define	AR9285_AN_RF2G4_DB1_4	0x1C000000
#define	AR9285_AN_RF2G4_DB1_4_S	26

#define	AR9285_AN_RF2G4_DB2_0	0x03800000
#define	AR9285_AN_RF2G4_DB2_0_S	23
#define	AR9285_AN_RF2G4_DB2_1	0x00700000
#define	AR9285_AN_RF2G4_DB2_1_S	20
#define	AR9285_AN_RF2G4_DB2_2	0x000E0000
#define	AR9285_AN_RF2G4_DB2_2_S	17
#define	AR9285_AN_RF2G4_DB2_3	0x0001C000
#define	AR9285_AN_RF2G4_DB2_3_S	14
#define	AR9285_AN_RF2G4_DB2_4	0x00003800
#define	AR9285_AN_RF2G4_DB2_4_S	11

#define	AR9285_AN_RF2G6		0x7834

#define	AR9285_AN_RF2G6_CCOMP	0x00007800
#define	AR9285_AN_RF2G6_CCOMP_S	11
#define	AR9285_AN_RF2G6_OFFS	0x03f00000
#define	AR9285_AN_RF2G6_OFFS_S	20

#define	AR9285_AN_RF2G7		0x7838

#define	AR9285_AN_RF2G7_PWDDB	0x00000002
#define	AR9285_AN_RF2G7_PWDDB_S	1
#define	AR9285_AN_RF2G7_PADRVGN2TAB0	0xE0000000
#define	AR9285_AN_RF2G7_PADRVGN2TAB0_S	29

#define	AR9285_AN_RF2G8		0x783C

#define	AR9285_AN_RF2G8_PADRVGN2TAB0	0x0001C000
#define	AR9285_AN_RF2G8_PADRVGN2TAB0_S	14

#define	AR9285_AN_RF2G9		0x7840

#define	AR9285_AN_RXTXBB1	0x7854

#define	AR9285_AN_RXTXBB1_PDRXTXBB1    0x00000020
#define	AR9285_AN_RXTXBB1_PDRXTXBB1_S  5
#define	AR9285_AN_RXTXBB1_PDV2I        0x00000080
#define	AR9285_AN_RXTXBB1_PDV2I_S      7
#define	AR9285_AN_RXTXBB1_PDDACIF      0x00000100
#define	AR9285_AN_RXTXBB1_PDDACIF_S    8
#define	AR9285_AN_RXTXBB1_SPARE9       0x00000001
#define	AR9285_AN_RXTXBB1_SPARE9_S     0

#define	AR9285_AN_TOP2		0x7868

#define	AR9285_AN_TOP3		0x786c

#define	AR9285_AN_TOP3_XPABIAS_LVL      0x0000000C
#define	AR9285_AN_TOP3_XPABIAS_LVL_S    2
#define	AR9285_AN_TOP3_PWDDAC		0x00800000
#define	AR9285_AN_TOP3_PWDDAC_S		23

#define	AR9285_AN_TOP4		0x7870
#define	AR9285_AN_TOP4_DEFAULT	0x10142c00

#endif	/* __AR9285_AN_H__ */
