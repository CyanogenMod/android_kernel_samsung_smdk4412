/*
 * midas-thermistor.c - thermistor of MIDAS Project
 *
 * Copyright (C) 2011 Samsung Electrnoics
 * SangYoung Son <hello.son@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <mach/midas-thermistor.h>
#ifdef CONFIG_SEC_THERMISTOR
#include <mach/sec_thermistor.h>
#endif

#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT) || \
	defined(CONFIG_MACH_C1_KOR_LGT)
extern int siopLevellimit;
#endif

#ifdef CONFIG_S3C_ADC
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_P4NOTE)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  204,	 800 },
	{  210,	 790 },
	{  216,	 780 },
	{  223,	 770 },
	{  230,	 760 },
	{  237,	 750 },
	{  244,	 740 },
	{  252,	 730 },
	{  260,	 720 },
	{  268,	 710 },
	{  276,	 700 },
	{  285,	 690 },
	{  294,	 680 },
	{  303,	 670 },
	{  312,	 660 },
	{  322,	 650 },
	{  332,	 640 },
	{  342,	 630 },
	{  353,	 620 },
	{  364,	 610 },
	{  375,	 600 },
	{  387,	 590 },
	{  399,	 580 },
	{  411,	 570 },
	{  423,	 560 },
	{  436,	 550 },
	{  450,	 540 },
	{  463,	 530 },
	{  477,	 520 },
	{  492,	 510 },
	{  507,	 500 },
	{  522,	 490 },
	{  537,	 480 },
	{  553,	 470 },
	{  569,	 460 },
	{  586,	 450 },
	{  603,	 440 },
	{  621,	 430 },
	{  638,	 420 },
	{  657,	 410 },
	{  675,	 400 },
	{  694,	 390 },
	{  713,	 380 },
	{  733,	 370 },
	{  753,	 360 },
	{  773,	 350 },
	{  794,	 340 },
	{  815,	 330 },
	{  836,	 320 },
	{  858,	 310 },
	{  880,	 300 },
	{  902,	 290 },
	{  924,	 280 },
	{  947,	 270 },
	{  969,	 260 },
	{  992,	 250 },
	{ 1015,	 240 },
	{ 1039,	 230 },
	{ 1062,	 220 },
	{ 1086,	 210 },
	{ 1109,	 200 },
	{ 1133,	 190 },
	{ 1156,	 180 },
	{ 1180,	 170 },
	{ 1204,	 160 },
	{ 1227,	 150 },
	{ 1250,	 140 },
	{ 1274,	 130 },
	{ 1297,	 120 },
	{ 1320,	 110 },
	{ 1343,	 100 },
	{ 1366,	  90 },
	{ 1388,	  80 },
	{ 1410,	  70 },
	{ 1432,	  60 },
	{ 1454,	  50 },
	{ 1475,	  40 },
	{ 1496,	  30 },
	{ 1516,	  20 },
	{ 1536,	  10 },
	{ 1556,	   0 },
	{ 1576,	 -10 },
	{ 1595,	 -20 },
	{ 1613,	 -30 },
	{ 1631,	 -40 },
	{ 1649,	 -50 },
	{ 1666,	 -60 },
	{ 1683,	 -70 },
	{ 1699,  -80 },
	{ 1714,  -90 },
	{ 1730, -100 },
	{ 1744, -110 },
	{ 1759, -120 },
	{ 1773, -130 },
	{ 1786, -140 },
	{ 1799, -150 },
	{ 1811, -160 },
	{ 1823, -170 },
	{ 1835, -180 },
	{ 1846, -190 },
	{ 1856, -200 },
};
#elif defined(CONFIG_MACH_C1)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  178,	 800 },
	{  186,	 790 },
	{  193,	 780 },
	{  198,	 770 },
	{  204,	 760 },
	{  210,	 750 },
	{  220,	 740 },
	{  226,	 730 },
	{  232,	 720 },
	{  247,	 710 },
	{  254,	 700 },
	{  261,	 690 },
	{  270,	 680 },
	{  278,	 670 },
	{  285,	 660 },
	{  292,	 650 },
	{  304,	 640 },
	{  319,	 630 },
	{  325,	 620 },
	{  331,	 610 },
	{  343,	 600 },
	{  354,	 590 },
	{  373,	 580 },
	{  387,	 570 },
	{  392,	 560 },
	{  408,	 550 },
	{  422,	 540 },
	{  433,	 530 },
	{  452,	 520 },
	{  466,	 510 },
	{  479,	 500 },
	{  497,	 490 },
	{  510,	 480 },
	{  529,	 470 },
	{  545,	 460 },
	{  562,	 450 },
	{  578,	 440 },
	{  594,	 430 },
	{  620,	 420 },
	{  632,	 410 },
	{  651,	 400 },
	{  663,	 390 },
	{  681,	 380 },
	{  705,	 370 },
	{  727,	 360 },
	{  736,	 350 },
	{  778,	 340 },
	{  793,	 330 },
	{  820,	 320 },
	{  834,	 310 },
	{  859,	 300 },
	{  872,	 290 },
	{  891,	 280 },
	{  914,	 270 },
	{  939,	 260 },
	{  951,	 250 },
	{  967,	 240 },
	{  999,	 230 },
	{ 1031,	 220 },
	{ 1049,	 210 },
	{ 1073,	 200 },
	{ 1097,	 190 },
	{ 1128,	 180 },
	{ 1140,	 170 },
	{ 1171,	 160 },
	{ 1188,	 150 },
	{ 1198,	 140 },
	{ 1223,	 130 },
	{ 1236,	 120 },
	{ 1274,	 110 },
	{ 1290,	 100 },
	{ 1312,	  90 },
	{ 1321,	  80 },
	{ 1353,	  70 },
	{ 1363,	  60 },
	{ 1404,	  50 },
	{ 1413,	  40 },
	{ 1444,	  30 },
	{ 1461,	  20 },
	{ 1470,	  10 },
	{ 1516,	   0 },
	{ 1522,	 -10 },
	{ 1533,	 -20 },
	{ 1540,	 -30 },
	{ 1558,	 -40 },
	{ 1581,	 -50 },
	{ 1595,	 -60 },
	{ 1607,	 -70 },
	{ 1614,  -80 },
	{ 1627,  -90 },
	{ 1655, -100 },
	{ 1664, -110 },
	{ 1670, -120 },
	{ 1676, -130 },
	{ 1692, -140 },
	{ 1713, -150 },
	{ 1734, -160 },
	{ 1746, -170 },
	{ 1789, -180 },
	{ 1805, -190 },
	{ 1824, -200 },
};
#elif defined(CONFIG_MACH_GC1)/*Sample # 3*/
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  250,	 700 },
	{  259,	 690 },
	{  270,	 680 },
	{  279,	 670 },
	{  289,	 660 },
	{  297,	 650 },
	{  312,	 640 },
	{  314,	 630 },
	{  324,	 620 },
	{  336,	 610 },
	{  344,	 600 },
	{  358,	 590 },
	{  369,	 580 },
	{  378,	 570 },
	{  390,	 560 },
	{  405,	 550 },
	{  419,	 540 },
	{  433,	 530 },
	{  447,	 520 },
	{  464,	 510 },
	{  471,	 500 },
	{  485,	 490 },
	{  510,	 480 },
	{  515,	 470 },
	{  537,	 460 },
	{  552,	 450 },
	{  577,	 440 },
	{  591,	 430 },
	{  606,	 420 },
	{  621,	 410 },
	{  636,	 400 },
	{  655,	 390 },
	{  677,	 380 },
	{  711,	 370 },
	{  727,	 360 },
	{  730,	 350 },
	{  755,	 340 },
	{  776,	 330 },
	{  795,	 320 },
	{  819,	 310 },
	{  832,	 300 },
	{  855,	 290 },
	{  883,	 280 },
	{  895,	 270 },
	{  939,	 260 },
	{  946,	 250 },
	{  958,	 240 },
	{  974,	 230 },
	{ 986,	 220 },
	{ 1023,	 210 },
	{ 1055,	 200 },
	{ 1065,	 190 },
	{ 1118,	 180 },
	{ 1147,	 170 },
	{ 1171,	 160 },
	{ 1190,	 150 },
	{ 1220,	 140 },
	{ 1224,	 130 },
	{ 1251,	 120 },
	{ 1271,	 110 },
	{ 1316,	 100 },
	{ 1325,	  90 },
	{ 1333,	  80 },
	{ 1365,	  70 },
	{ 1382,	  60 },
	{ 1404,	  50 },
	{ 1445,	  40 },
	{ 1461,	  30 },
	{ 1469,	  20 },
	{ 1492,	  10 },
	{ 1518,	   0 },
	{ 1552,	 -10 },
	{ 1560,	 -20 },
	{ 1588,	 -30 },
	{ 1592,	 -40 },
	{ 1613,	 -50 },
	{ 1632,	 -60 },
	{ 1647,	 -70 },
	{ 1661,	-80 },
	{ 1685,	-90 },
	{ 1692,	-100 },
};
#elif defined(CONFIG_MACH_M3)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  264,  700 },
	{  293,  670 },
	{  315,  650 },
	{  338,  630 },
	{  348,  620 },
	{  371,  600 },
	{  436,  550 },
	{  509,  500 },
	{  599,  450 },
	{  629,  430 },
	{  651,  420 },
	{  692,  400 },
	{  790,  350 },
	{  909,  300 },
	{ 1027,  250 },
	{ 1141,  200 },
	{ 1246,  150 },
	{ 1364,  100 },
	{ 1460,   50 },
	{ 1573,    0 },
	{ 1614,  -30 },
	{ 1650,  -50 },
	{ 1735, -100 },
	{ 1800, -150 },
	{ 1857, -200 },
	{ 1910, -250 },
	{ 1970, -300 },
};
#elif defined(CONFIG_MACH_T0)
#if defined(CONFIG_TARGET_LOCALE_KOR)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  200,	 800 },
	{  207,	 790 },
	{  214,	 780 },
	{  221,	 770 },
	{  228,	 760 },
	{  235,	 750 },
	{  248,	 740 },
	{  260,	 730 },
	{  273,	 720 },
	{  286,	 710 },
	{  299,	 700 },
	{  310,	 690 },
	{  321,	 680 },
	{  332,	 670 },
	{  345,	 660 },
	{  355,	 650 },
	{  370,	 640 },
	{  381,	 630 },
	{  393,	 620 },
	{  406,	 610 },
	{  423,	 600 },
	{  435,	 590 },
	{  448,	 580 },
	{  460,	 570 },
	{  473,	 560 },
	{  485,	 550 },
	{  498,	 540 },
	{  511,	 530 },
	{  524,	 520 },
	{  537,	 510 },
	{  550,	 500 },
	{  566,	 490 },
	{  582,	 480 },
	{  598,	 470 },
	{  614,	 460 },
	{  630,	 450 },
	{  649,	 440 },
	{  668,	 430 },
	{  687,	 420 },
	{  706,	 410 },
	{  725,	 400 },
	{  744,	 390 },
	{  763,	 380 },
	{  782,	 370 },
	{  801,	 360 },
	{  820,	 350 },
	{  841,	 340 },
	{  862,	 330 },
	{  883,	 320 },
	{  904,	 310 },
	{  925,	 300 },
	{  946,	 290 },
	{  967,	 280 },
	{  988,	 270 },
	{ 1009,	 260 },
	{ 1030,	 250 },
	{ 1050,	 240 },
	{ 1070,	 230 },
	{ 1090,	 220 },
	{ 1110,	 210 },
	{ 1130,	 200 },
	{ 1154,	 190 },
	{ 1178,	 180 },
	{ 1202,	 170 },
	{ 1226,	 160 },
	{ 1250,	 150 },
	{ 1272,	 140 },
	{ 1294,	 130 },
	{ 1315,	 120 },
	{ 1337,	 110 },
	{ 1359,	 100 },
	{ 1381,	  90 },
	{ 1403,	  80 },
	{ 1424,	  70 },
	{ 1446,	  60 },
	{ 1468,	  50 },
	{ 1489,	  40 },
	{ 1510,	  30 },
	{ 1532,	  20 },
	{ 1553,	  10 },
	{ 1574,	   0 },
	{ 1591,	 -10 },
	{ 1609,	 -20 },
	{ 1626,	 -30 },
	{ 1644,	 -40 },
	{ 1661,	 -50 },
	{ 1676,	 -60 },
	{ 1691,	 -70 },
	{ 1707,  -80 },
	{ 1722,  -90 },
	{ 1737, -100 },
	{ 1749, -110 },
	{ 1760, -120 },
	{ 1772, -130 },
	{ 1783, -140 },
	{ 1795, -150 },
	{ 1803, -160 },
	{ 1811, -170 },
	{ 1819, -180 },
	{ 1827, -190 },
	{ 1836, -200 },
};
#elif defined(CONFIG_MACH_T0_EUR_LTE) || defined(CONFIG_MACH_T0_USA_ATT)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{ 332,	 650 },
	{ 389,	 600 },
	{ 460,	 550 },
	{ 530,	 500 },
	{ 617,	 450 },
	{ 735,	 400 },
	{ 803,	 350 },
	{ 913,	 300 },
	{ 992,	 250 },
	{ 1126,	 200 },
	{ 1265,	 150 },
	{ 1370,	 100 },
	{ 1475,	  50 },
	{ 1530,	   0 },
	{ 1635,  -50 },
	{ 1724, -100 },
	{ 1803, -150 },
	{ 1855, -200 },
};
#elif defined(CONFIG_MACH_T0_USA_VZW) || defined(CONFIG_MACH_T0_USA_SPR) || \
	defined(CONFIG_MACH_T0_USA_USCC)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{ 193,	 800 },
	{ 222,	 750 },
	{ 272,	 700 },
	{ 320,	 650 },
	{ 379,	 600 },
	{ 439,	 550 },
	{ 519,	 500 },
	{ 555,	 480 },
	{ 605,	 450 },
	{ 698,	 400 },
	{ 802,	 350 },
	{ 906,	 300 },
	{ 1016,	 250 },
	{ 1127,	 200 },
	{ 1247,	 150 },
	{ 1360,	 100 },
	{ 1467,	  50 },
	{ 1567,	   0 },
	{ 1633,  -50 },
	{ 1725,	-100 },
	{ 1790,	-150 },
	{ 1839,	-200 },
	{ 1903,	-250 },
};
#elif defined(CONFIG_MACH_BAFFIN)
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{  178,	 800 },
	{  186,	 790 },
	{  193,	 780 },
	{  198,	 770 },
	{  204,	 760 },
	{  210,	 750 },
	{  220,	 740 },
	{  226,	 730 },
	{  232,	 720 },
	{  247,	 710 },
	{  254,	 700 },
	{  261,	 690 },
	{  270,	 680 },
	{  278,	 670 },
	{  285,	 660 },
	{  292,	 650 },
	{  304,	 640 },
	{  319,	 630 },
	{  325,	 620 },
	{  331,	 610 },
	{  343,	 600 },
	{  354,	 590 },
	{  373,	 580 },
	{  387,	 570 },
	{  392,	 560 },
	{  408,	 550 },
	{  422,	 540 },
	{  433,	 530 },
	{  452,	 520 },
	{  466,	 510 },
	{  479,	 500 },
	{  497,	 490 },
	{  510,	 480 },
	{  529,	 470 },
	{  545,	 460 },
	{  562,	 450 },
	{  578,	 440 },
	{  594,	 430 },
	{  620,	 420 },
	{  632,	 410 },
	{  651,	 400 },
	{  663,	 390 },
	{  681,	 380 },
	{  705,	 370 },
	{  727,	 360 },
	{  736,	 350 },
	{  778,	 340 },
	{  793,	 330 },
	{  820,	 320 },
	{  834,	 310 },
	{  859,	 300 },
	{  872,	 290 },
	{  891,	 280 },
	{  914,	 270 },
	{  939,	 260 },
	{  951,	 250 },
	{  967,	 240 },
	{  999,	 230 },
	{ 1031,	 220 },
	{ 1049,	 210 },
	{ 1073,	 200 },
	{ 1097,	 190 },
	{ 1128,	 180 },
	{ 1140,	 170 },
	{ 1171,	 160 },
	{ 1188,	 150 },
	{ 1198,	 140 },
	{ 1223,	 130 },
	{ 1236,	 120 },
	{ 1274,	 110 },
	{ 1290,	 100 },
	{ 1312,	  90 },
	{ 1321,	  80 },
	{ 1353,	  70 },
	{ 1363,	  60 },
	{ 1404,	  50 },
	{ 1413,	  40 },
	{ 1444,	  30 },
	{ 1461,	  20 },
	{ 1470,	  10 },
	{ 1516,	   0 },
	{ 1522,	 -10 },
	{ 1533,	 -20 },
	{ 1540,	 -30 },
	{ 1558,	 -40 },
	{ 1581,	 -50 },
	{ 1595,	 -60 },
	{ 1607,	 -70 },
	{ 1614,  -80 },
	{ 1627,  -90 },
	{ 1655, -100 },
	{ 1664, -110 },
	{ 1670, -120 },
	{ 1676, -130 },
	{ 1692, -140 },
	{ 1713, -150 },
	{ 1734, -160 },
	{ 1746, -170 },
	{ 1789, -180 },
	{ 1805, -190 },
	{ 1824, -200 },
};
#else	/* T0 3G(default) */
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{ 358,   600 },
	{ 500,   500 },
	{ 698,   400 },
	{ 898,   300 },
	{ 1132,  200 },
	{ 1363,  100 },
	{ 1574,    0 },
	{ 1732, -100 },
	{ 1860, -200 },
};
#endif
#else	/* sample */
static struct adc_table_data ap_adc_temper_table_battery[] = {
	{ 305,  650 },
	{ 566,  430 },
	{ 1494,   0 },
	{ 1571, -50 },
};
#endif

int convert_adc(int adc_data, int channel)
{
	int adc_value;
	int low, mid, high;
	struct adc_table_data *temper_table = NULL;
	pr_debug("%s\n", __func__);

	low = mid = high = 0;
	switch (channel) {
	case 1:
		temper_table = ap_adc_temper_table_battery;
		high = ARRAY_SIZE(ap_adc_temper_table_battery) - 1;
		break;
	case 2:
		temper_table = ap_adc_temper_table_battery;
		high = ARRAY_SIZE(ap_adc_temper_table_battery) - 1;
		break;
	default:
		pr_info("%s: not exist temper table for ch(%d)\n", __func__,
							channel);
		return -EINVAL;
		break;
	}

	/* Out of table range */
	if (adc_data <= temper_table[low].adc) {
		adc_value = temper_table[low].value;
		return adc_value;
	} else if (adc_data >= temper_table[high].adc) {
		adc_value = temper_table[high].value;
		return adc_value;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		if (temper_table[mid].adc > adc_data)
			high = mid - 1;
		else if (temper_table[mid].adc < adc_data)
			low = mid + 1;
		else
			break;
	}
	adc_value = temper_table[mid].value;

	/* high resolution */
	if (adc_data < temper_table[mid].adc)
		adc_value = temper_table[mid].value +
			((temper_table[mid-1].value - temper_table[mid].value) *
			(temper_table[mid].adc - adc_data) /
			(temper_table[mid].adc - temper_table[mid-1].adc));
	else
		adc_value = temper_table[mid].value -
			((temper_table[mid].value - temper_table[mid+1].value) *
			(adc_data - temper_table[mid].adc) /
			(temper_table[mid+1].adc - temper_table[mid].adc));

	pr_debug("%s: adc data(%d), adc value(%d)\n", __func__,
					adc_data, adc_value);
	return adc_value;

}
#endif

#ifdef CONFIG_SEC_THERMISTOR
#if defined(CONFIG_MACH_GC1)
static struct sec_therm_adc_table temper_table_ap[] = {
	{241,	700},
	{260,	650},
	{301,	600},
	{351,	550},
	{367,	540},
	{383,	530},
	{399,	520},
	{415,	510},
	{431,	500},
	{442,	490},
	{454,	480},
	{465,	470},
	{477,	460},
	{488,	450},
	{508,	440},
	{528,	430},
	{548,	420},
	{568,	410},
	{587,	400},
	{604,	390},
	{622,	380},
	{639,	370},
	{657,	360},
	{674,	350},
	{696,	340},
	{718,	330},
	{740,	320},
	{762,	310},
	{784,	300},
};
#else
static struct sec_therm_adc_table temper_table_ap[] = {
	{196,	700},
	{211,	690},
	{242,	685},
	{249,	680},
	{262,	670},
	{275,	660},
	{288,	650},
	{301,	640},
	{314,	630},
	{328,	620},
	{341,	610},
	{354,	600},
	{366,	590},
	{377,	580},
	{389,	570},
	{404,	560},
	{419,	550},
	{434,	540},
	{452,	530},
	{469,	520},
	{487,	510},
	{498,	500},
	{509,	490},
	{520,	480},
	{529,	470},
	{538,	460},
	{547,	450},
	{556,	440},
	{564,	430},
	{573,	420},
	{581,	410},
	{590,	400},
	{615,	390},
	{640,	380},
	{665,	370},
	{690,	360},
	{715,	350},
	{736,	340},
	{758,	330},
	{779,	320},
	{801,	310},
	{822,	300},
};
#endif

/* when the next level is same as prev, returns -1 */
static int get_midas_siop_level(int temp)
{
	static int prev_temp = 400;
	static int prev_level = 0;
	int level = -1;

#if defined(CONFIG_MACH_C1_KOR_SKT) || defined(CONFIG_MACH_C1_KOR_KT) || \
	defined(CONFIG_MACH_C1_KOR_LGT)
	if (temp > prev_temp) {
		if (temp >= 540)
			level = 4;
		else if (temp >= 530)
			level = 3;
		else if (temp >= 480)
			level = 2;
		else if (temp >= 440)
			level = 1;
		else
			level = 0;
	} else {
		if (temp < 410)
			level = 0;
		else if (temp < 440)
			level = 1;
		else if (temp < 480)
			level = 2;
		else if (temp < 530)
			level = 3;
		else
			level = 4;

		if (level > prev_level)
			level = prev_level;
	}

	if (siopLevellimit != 0 && level > siopLevellimit)
		level = siopLevellimit;

#elif defined(CONFIG_MACH_P4NOTE)
	if (temp > prev_temp) {
		if (temp >= 620)
			level = 4;
		else if (temp >= 610)
			level = 3;
		else if (temp >= 580)
			level = 2;
		else if (temp >= 550)
			level = 1;
		else
			level = 0;
	} else {
		if (temp < 520)
			level = 0;
		else if (temp < 550)
			level = 1;
		else if (temp < 580)
			level = 2;
		else if (temp < 610)
			level = 3;
		else
			level = 4;

		if (level > prev_level)
			level = prev_level;
	}
#elif defined(CONFIG_MACH_T0)
	if (temp > prev_temp) {
		if (temp >= 620)
			level = 4;
		else if (temp >= 610)
			level = 3;
		else if (temp >= 570)
			level = 2;
		else if (temp >= 540)
			level = 1;
		else
			level = 0;
	} else {
		if (temp < 510)
			level = 0;
		else if (temp < 540)
			level = 1;
		else if (temp < 570)
			level = 2;
		else if (temp < 610)
			level = 3;
		else
			level = 4;

		if (level > prev_level)
			level = prev_level;
	}
#elif defined(CONFIG_MACH_GC1)
	if (temp > prev_temp) {
		if (temp >= 590)
			level = 4;
		else if (temp >= 580)
			level = 3;
		else if (temp >= 550)
			level = 2;
		else if (temp >= 520)
			level = 1;
		else
			level = 0;
	} else {
		if (temp < 480)
			level = 0;
		else if (temp < 520)
			level = 1;
		else if (temp < 550)
			level = 2;
		else if (temp < 580)
			level = 3;
		else
			level = 4;

		if (level > prev_level)
			level = prev_level;
	}
#else
	if (temp > prev_temp) {
		if (temp >= 540)
			level = 4;
		else if (temp >= 530)
			level = 3;
		else if (temp >= 480)
			level = 2;
		else if (temp >= 440)
			level = 1;
		else
			level = 0;
	} else {
		if (temp < 410)
			level = 0;
		else if (temp < 440)
			level = 1;
		else if (temp < 480)
			level = 2;
		else if (temp < 530)
			level = 3;
		else
			level = 4;

		if (level > prev_level)
			level = prev_level;
	}
#endif

	prev_temp = temp;
	if (prev_level == level)
		return -1;

	prev_level = level;

	return level;
}

static struct sec_therm_platform_data sec_therm_pdata = {
	.adc_channel	= 1,
	.adc_arr_size	= ARRAY_SIZE(temper_table_ap),
	.adc_table	= temper_table_ap,
	.polling_interval = 30 * 1000, /* msecs */
	.get_siop_level = get_midas_siop_level,
};

struct platform_device sec_device_thermistor = {
	.name = "sec-thermistor",
	.id = -1,
	.dev.platform_data = &sec_therm_pdata,
};
#endif

#ifdef CONFIG_STMPE811_ADC
/* temperature table for ADC ch7 */
static struct adc_table_data temper_table_battery[] = {
	{	1856, -20	},
	{	1846, -19	},
	{	1835, -18	},
	{	1823, -17	},
	{	1811, -16	},
	{	1799, -15	},
	{	1786, -14	},
	{	1773, -13	},
	{	1759, -12	},
	{	1744, -11	},
	{	1730, -10	},
	{	1714, -9	},
	{	1699, -8	},
	{	1683, -7	},
	{	1666, -6	},
	{	1649, -5	},
	{	1631, -4	},
	{	1613, -3	},
	{	1595, -2	},
	{	1576, -1	},
	{	1556, 0		},
	{	1536, 1		},
	{	1516, 2		},
	{	1496, 3		},
	{	1475, 4		},
	{	1454, 5		},
	{	1432, 6		},
	{	1410, 7		},
	{	1388, 8		},
	{	1366, 9		},
	{	1343, 10	},
	{	1320, 11	},
	{	1297, 12	},
	{	1274, 13	},
	{	1250, 14	},
	{	1227, 15	},
	{	1204, 16	},
	{	1180, 17	},
	{	1156, 18	},
	{	1133, 19	},
	{	1109, 20	},
	{	1086, 21	},
	{	1062, 22	},
	{	1039, 23	},
	{	1015, 24	},
	{	992,  25	},
	{	969,  26	},
	{	947,  27	},
	{	924,  28	},
	{	902,  29	},
	{	880,  30	},
	{	858,  31	},
	{	836,  32	},
	{	815,  33	},
	{	794,  34	},
	{	773,  35	},
	{	753,  36	},
	{	733,  37	},
	{	713,  38	},
	{	694,  39	},
	{	675,  40	},
	{	657,  41	},
	{	638,  42	},
	{	621,  43	},
	{	603,  44	},
	{	586,  45	},
	{	569,  46	},
	{	553,  47	},
	{	537,  48	},
	{	522,  49	},
	{	507,  50	},
	{	492,  51	},
	{	477,  52	},
	{	463,  53	},
	{	450,  54	},
	{	436,  55	},
	{	423,  56	},
	{	411,  57	},
	{	399,  58	},
	{	387,  59	},
	{	375,  60	},
	{	364,  61	},
	{	353,  62	},
	{	342,  63	},
	{	332,  64	},
	{	322,  65	},
	{	312,  66	},
	{	303,  67	},
	{	294,  68	},
	{	285,  69	},
	{	276,  70	},
	{	268,  71	},
	{	260,  72	},
	{	252,  73	},
	{	244,  74	},
	{	237,  75	},
	{	230,  76	},
	{	223,  77	},
	{	216,  78	},
	{	210,  79	},
	{	204,  80	},
};

struct stmpe811_platform_data stmpe811_pdata = {
	.adc_table_ch4 = temper_table_battery,
	.table_size_ch4 = ARRAY_SIZE(temper_table_battery),
	.adc_table_ch7 = temper_table_battery,
	.table_size_ch7 = ARRAY_SIZE(temper_table_battery),

	.irq_gpio = GPIO_ADC_INT,
};
#endif

