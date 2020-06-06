/* 
 * gm-seed-key.c
 * seed to key generator for old GM units with 2 bytes keys.
 *
 * Based on: 
 * http://www.gearhead-efi.com/Fuel-Injection/showthread.php?7286-Gm-Seed-key-algorithms
 * https://github.com/SCResearch/GM-Seed-key-Tester
 *
 *
 */ 

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table_gmlan.h"
#include "table_class2.h"
#include "table_others.h"
#ifdef WITH_TESTS
#include "tests.h"
#endif

typedef unsigned char BYTE;
typedef unsigned short WORD;


/* flip bytes */
static WORD op_05(WORD val)
{
	return (val << 8) | (val >> 8);
}

/* addition */
static WORD op_14(WORD val, BYTE hh, BYTE ll)
{
	return val + (((WORD)hh << 8) | ll);
}

/* 1's (or 2's) complement */
static WORD op_2a(WORD val, BYTE hh, BYTE ll)
{
	val = ~val;
	if ( hh < ll )
		val++;
	return val;
}

/* and with flipped operand */
static WORD op_37(WORD val, BYTE hh, BYTE ll)
{
	return val & (((WORD)ll << 8) | hh);
}

/* ratate left by hh */
static WORD op_4c(WORD val, BYTE hh, BYTE ll)
{
	return ( val << hh) | (val >> ( 16 - hh ));
}

/* or with flipped operand */
static WORD op_52(WORD val, BYTE hh, BYTE ll)
{
	return val | hh | ((WORD)ll << 8);
}

/* rotate right by ll */
static WORD op_6b(WORD val, BYTE hh, BYTE ll)
{
	return (val >> ll) | (val << (16 - ll));
}

/* addition with flipped operand */
static WORD op_75(WORD val, BYTE hh, BYTE ll)
{
	return val + (((WORD)ll << 8) | hh);
}

/* filp bytes, and add (flipped) operand */
static WORD op_7e(WORD val, BYTE hh, BYTE ll)
{
	if ( hh >= ll )
		return op_14(op_05(val), hh, ll);
	return op_75(op_05(val), hh, ll);
}

/* substraction */
static WORD op_98(WORD val, BYTE hh, BYTE ll)
{
	return val - (((WORD)hh << 8) | ll);
}

/* substraction of flipped operand */
static WORD op_f8(WORD val, BYTE hh, BYTE ll)
{
	return val - (((WORD)ll << 8) | hh);
}

static WORD get_key(WORD seed, WORD algo, BYTE *table)
{
	int idx, i;
	BYTE hh, ll, code;

	if (algo == 0)
		return ~seed;

	idx = algo * 13;
	for (i = 0; i < 4; i++) {
		code = table[idx];
		hh = table[idx + 1];
		ll = table[idx + 2];
		switch (code) {
			case 0x05:
				seed = op_05(seed);
				break;
			case 0x14:
				seed = op_14(seed, hh, ll);
				break;
			case 0x2A:
				seed = op_2a(seed, hh, ll);
				break;
			case 0x37:
				seed = op_37(seed, hh, ll);
				break;
			case 0x4C:
				seed = op_4c(seed, hh, ll);
				break;
			case 0x52:
				seed = op_52(seed, hh, ll);
				break;
			case 0x6b:
				seed = op_6b(seed, hh, ll);
				break;
			case 0x75:
				seed = op_75(seed, hh, ll);
				break;
			case 0x7e:
				seed = op_7e(seed, hh, ll);
				break;
			case 0x98:
				seed = op_98(seed, hh, ll);
				break;
			case 0xf8:
				seed = op_f8(seed, hh, ll);
				break;

		}
//		printf("%02x %02x %02x:  %04x\n",code, hh, ll, seed);
		idx += 3;
	}
	return seed;
}

int main(int argc, char **argv)
{
	BYTE *table = table_gmlan;
	int algo = 0;
	int seed = 0;
	int algo_max = 0xff;
	int seed_max = 0xffff;
	int a, c;

	while ((c = getopt (argc, argv, "a:s:p:h")) != -1)
		switch (c) {
			case 'a':
				algo = algo_max = strtol(optarg, NULL, 16);
				if (algo > 255 || algo < 0) {
					fprintf (stderr, "Algo should by in [0x00:0xff] range.\n");
					return 1;
				}
				break;
			case 's':
				seed = seed_max = strtol(optarg, NULL, 16);
				if (seed > 0xffff || seed < 0) {
					fprintf (stderr, "Seed should by in [0x0000:0xffff] range.\n");
					return 1;
				}
				break;
			case 'p':
				if (!strcmp(optarg, "gmlan"))
					table = table_gmlan;
				else if (!strcmp(optarg, "class2"))
					table = table_class2;
				else if (!strcmp(optarg, "others"))
					table = table_others;
				else {
					fprintf (stderr, "Protocol should be one of: gmlan, class2, others.\n");
					return 1;
				}

				break;
			case 'h':
			default:
				printf("Usage: %s <-a algo> <-s seed> <-p protocol>\n", argv[0]);
				printf("Print key(s) for provided algo, seed and protocol.\n\n");
				printf(" -a <algo>     Algorithm to use. Hex value in the range [00:ff].\n");
				printf("               If not provided - will print the keys for all algos.\n");
				printf(" -s <seed>     Input seed. Hex value in the range [0000:ffff].\n");
				printf("               If not provided - will print the keys for all seeds.\n");
				printf(" -p <protocol> Protocol to use. Acceptable vlues \"gmlan\", \"class2\", \"others\".\n");
				printf("               Default protocol is \"gmlan\".\n");
				return 0;
		}

#ifdef WITH_TESTS
	int i;
	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
		if (tests[i][2] != get_key(tests[i][1], tests[i][0], table))
			printf("key(%2x, %4x) != %4x\n", tests[i][0],
				tests[i][1], tests[i][2]);
#endif

	for (; seed <= seed_max; seed++)
		for (a = algo; a <= algo_max; a++)
			printf("%04x %2x %04x\n",
				(WORD)seed, (BYTE)a,
				get_key ((WORD)seed, (BYTE)a, table));
	return 0;
}
