#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "egdb.h"


#define DB1_DIR "e:/kr_english_wld"
#define DB1_MEM 300

/* If you have 2 databases you can set VERIFY to 1 and then
 * the program will verify some lookups of one against the other.
 * Set DB2_DIR and DB2_MEM for the second db.
 */
#define VERIFY 0

#define DB2_DIR "c:\\db\\cake"
#define DB2_MEM 300

/* Set BENCHMARK to 1 to run timings on some lookups. */
#define BENCHMARK 1

#define SAMPLES 500		/* positions to verify in each slice */
#define MAX_PIECES 8
#define MAX_PIECE 4
#define REPEAT_COUNT 100

typedef struct {
	int npieces;
	EGDB_NORMAL_BITMAP pos;
	int color;
	int value;
} DBVALUE;

int show_driver_msgs = 1;

extern DBVALUE test_table[];
char *db_description(EGDB_TYPE db_type);
void __cdecl msg_fn(char *msg);
void verify(EGDB_BITMAP_TYPE db1_bitmap_type, EGDB_BITMAP_TYPE db2_bitmap_type);
void benchmark();
void lookup_test_positions(EGDB_DRIVER * db, int max_pieces, EGDB_BITMAP_TYPE bitmap_type);


/*
 * This program demonstrates the use of the egdb library, and does
 * some testing of the API functions.
 */
int __cdecl main(int argc, char *argv[])
{
	int max_pieces;
	EGDB_DRIVER *db_handle;
	EGDB_TYPE db_type;

	/* See if there's a database in the directory given, get its attributes. */
	if (egdb_identify(DB1_DIR, &db_type, &max_pieces)) {
		printf("No database found in %s\n", DB1_DIR);
		return(1);
	}
	else
		printf("Found %s %d piece database\n", db_description(db_type), max_pieces);

	/* Open the database. */
	printf("Wait for egdb_open ...\n");
	max_pieces = min(max_pieces, MAX_PIECES);
	db_handle = egdb_open(EGDB_NORMAL, max_pieces, DB1_MEM, DB1_DIR, msg_fn);
	if (!db_handle) {
		printf("Cannot open database\n");
		return(1);
	}

	/* Lookup some known positions. */
	lookup_test_positions(db_handle, max_pieces, EGDB_NORMAL);

	/* Close the database. */
	if ((*db_handle->close)(db_handle)) {
		printf("Close failed\n");
		return(1);
	}
	else
		printf("Close ok\n");

	/* Open it again, this time for reversed row bitmaps, and
	 * give it 0mb of memory.  The driver will use the minimum amount.
	 */
	printf("Checking reversed row lookups ...\n");
	db_handle = egdb_open(EGDB_ROW_REVERSED, max_pieces, 0, DB1_DIR, msg_fn);
	if (!db_handle) {
		printf("Cannot open database\n");
		return(1);
	}

	/* Lookup known positions again. */
	lookup_test_positions(db_handle, max_pieces, EGDB_ROW_REVERSED);

	/* Verify the database with crc checks. */
	printf("Verifying CRCs...\n");
	if ((*db_handle->verify)(db_handle)) {
		printf("Verify failed\n");
		exit(1);
	}
	else
		printf("Verify ok\n");

	/* Close the driver. */
	if ((*db_handle->close)(db_handle)) {
		printf("Close failed\n");
		return(1);
	}

#if BENCHMARK
	/* Measure the lookup speed. */
	benchmark();
#endif

#if VERIFY
	/* Verify one db against another using both bitmap types.
	 * Need to set the macros DB2_DIR and DB2_MEM for the 2nd database
	 * to use this.
	 */
	verify(EGDB_NORMAL, EGDB_ROW_REVERSED);
	verify(EGDB_ROW_REVERSED, EGDB_NORMAL);
#endif

	return(0);
}


/*
 * Exchange bit n with bit (3 - n) in each nibble (checker row) of a bitmap.
 */
unsigned int reverse_rows(unsigned int bitmap)
{
	int i;
	unsigned int row_reversed;
	static char xlat[] = {
		0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
	};

	row_reversed = 0;
	for (i = 0; i < sizeof(bitmap) * 2; ++i)
		row_reversed |= (xlat[(bitmap >> (4 * i)) & 0x0f] << (4 * i));

	return(row_reversed);
}


/*
 * Convert an EGDB_NORMAL_BITMAP board to an EGDB_REVERSED_BITMAP board.
 */
void normal_to_row_reversed(EGDB_ROW_REVERSED_BITMAP *dest, EGDB_NORMAL_BITMAP *src)
{
	unsigned int black, white, king;

	black = reverse_rows(src->black);
	white = reverse_rows(src->white);
	king = reverse_rows(src->king);
	dest->black_man = black & ~king;
	dest->black_king = black & king;
	dest->white_man = white & ~king;
	dest->white_king = white & king;
}


void lookup_test_positions(EGDB_DRIVER *db, int max_pieces, EGDB_BITMAP_TYPE bitmap_type)
{
	int i;
	int value;
	DBVALUE *p;
	EGDB_BITMAP pos;

	printf("Checking test positions\n");
	for (i = 0; test_table[i].npieces <= max_pieces; ++i) {
		p = test_table + i;

		/* Convert the bitmaps if we opened the driver for cake-style bitmaps. */
		if (bitmap_type == EGDB_NORMAL)
			pos.normal = p->pos;
		else
			normal_to_row_reversed(&pos.row_reversed, &p->pos);

		value = (*db->lookup)(db, &pos, p->color, 0);
		if (value != p->value) {
			printf("lookup value doesn't agree with test table: expected %d, got %d\n",
						p->value, value);
			printf("pos (%08x %08x %08x), color %d\n", 
						p->pos.black, p->pos.white, p->pos.king, p->color);
		}
	}
}


/*
 * Verify one slice by doing some lookups on 2 different dbs and comparing
 * the values.
 */
void verify_slice(EGDB_DRIVER *db1, EGDB_BITMAP_TYPE db1_bitmap_type, EGDB_DRIVER *db2, 
				  EGDB_BITMAP_TYPE db2_bitmap_type, int nbm, int nbk, int nwm, int nwk)
{
	int value1, value2;
	__int64 index, step, range;
	EGDB_BITMAP pos;
	EGDB_BITMAP rev;
	EGDB_BITMAP *pos1, *pos2;

	printf("verifying db%d%d%d%d ...\n", nbm, nbk, nwm, nwk);
	range = egdb_index_range(nbm, nbk, nwm, nwk);

	step = range / SAMPLES;
	if (step < 1)
		step = 1;

	for (index = 0; index < range; index += step) {

		/* Get the position. */
		egdb_indextoposition(index, &pos.normal, nbm, nbk, nwm, nwk);

		/* Get the position in row reversed format. */
		normal_to_row_reversed(&rev.row_reversed, &pos.normal);

		/* Dont lookup capture positions. */
		if (egdb_iscapture(&pos.normal, EGDB_BLACK) || egdb_iscapture(&pos.normal, EGDB_WHITE))
			continue;

		if (db1_bitmap_type == EGDB_NORMAL)
			pos1 = &pos;
		else
			pos1 = &rev;
		if (db2_bitmap_type == EGDB_NORMAL)
			pos2 = &pos;
		else
			pos2 = &rev;

		/* Verify the position with black to move. */
		value1 = (*db1->lookup)(db1, pos1, EGDB_BLACK, 0);
		value2 = (*db2->lookup)(db2, pos2, EGDB_BLACK, 0);
		if (value1 != value2) {
			printf("values dont match %d %d\n", value1, value2);
			printf("index %I64d (%08x  %08x  %08x)\n", 
					index, pos.normal.black, pos.normal.white, pos.normal.king);
		}
		if (value1 != EGDB_WIN && value1 != EGDB_DRAW && value1 != EGDB_LOSS)
			printf("unknown value %d\n", value1);

		/* Verify the position with white to move. */
		value1 = (*db1->lookup)(db1, pos1, EGDB_WHITE, 0);
		value2 = (*db2->lookup)(db2, pos2, EGDB_WHITE, 0);
		if (value1 != value2) {
			printf("values dont match %d %d\n", value1, value2);
			printf("index %I64d (%08x  %08x  %08x)\n", 
					index, pos.normal.black, pos.normal.white, pos.normal.king);
		}
		if (value1 != EGDB_WIN && value1 != EGDB_DRAW && value1 != EGDB_LOSS)
			printf("unknown value %d\n", value1);
	}
}


/*
 * Verify that values read from one db agree with values from a 2nd db.
 */
void verify(EGDB_BITMAP_TYPE db1_bitmap_type, EGDB_BITMAP_TYPE db2_bitmap_type)
{
	int pieces, nblack, nwhite, nbm, nbk, nwm, nwk;
	EGDB_DRIVER *db1, *db2;

	db1 = egdb_open(db1_bitmap_type, MAX_PIECES, DB1_MEM, DB1_DIR, msg_fn);
	if (!db1) {
		printf("Cannot open db1\n");
		exit(1);
	}
	db2 = egdb_open(db2_bitmap_type, MAX_PIECES, DB2_MEM, DB2_DIR, msg_fn);
	if (!db2) {
		printf("Cannot open db2\n");
		exit(1);
	}

	/* Break the db up into slices. */
	for (pieces = 2; pieces <= MAX_PIECES; ++pieces) {
		for (nblack = 0; nblack <= pieces; ++nblack) {
			nwhite = pieces - nblack;
			if (nblack > MAX_PIECE || nwhite > MAX_PIECE)
				continue;
			for (nbm = 0; nbm <= nblack; ++nbm) {
				nbk = nblack - nbm;
				for (nwm = 0; nwm <= nwhite; ++nwm) {
					nwk = nwhite - nwm;
					verify_slice(db1, db1_bitmap_type, db2, db2_bitmap_type, nbm, nbk, nwm, nwk);
				}
			}
		}
	}

	(*db1->close)(db1);
	(*db2->close)(db2);
}


/*
 * Return a somewhat random integer such that minval <= n <= maxval.
 */
int get_rand(int minval, int maxval)
{
	int n;

	n = rand();
	n %= (1 + maxval - minval);
	n += minval;
	return(n);
}


/*
 * Return a somewhat random 48-bit integer such that minval <= n <= maxval.
 */
__int64 get_rand64(__int64 minval, __int64 maxval)
{
	__int64 n;

	n = rand();
	n ^= (n << 19);
	n %= (1 + maxval - minval);
	n += minval;
	return(n);
}


void print_stats(EGDB_DRIVER *db)
{
	EGDB_STATS *stats;

	stats = (*db->get_stats)(db);
	printf("hits %d, loads %d, alhits %d, reqs %d, rtns %d, notpres %d\n",
			stats->lru_cache_hits,
			stats->lru_cache_loads, 
			stats->autoload_hits, 
			stats->db_requests, 
			stats->db_returns, 
			stats->db_not_present_requests);
}


void benchmark_pieces(int npieces, int dbmem, EGDB_BITMAP_TYPE bitmap_type)
{
	int i, test_pos_count, count;
	int nblack, nwhite, nbm, nbk, nwm, nwk;
	__int64 index, range;
	unsigned int t0;
	char *dbdir = DB1_DIR;
	EGDB_BITMAP pos;
	EGDB_DRIVER *db;
	EGDB_STATS *stats;
	EGDB_BITMAP testpos[10000];

	if (npieces > MAX_PIECES)
		return;

	/* Create a random set of test positions wth npieces. */
	test_pos_count = sizeof(testpos) / sizeof(testpos[0]);
	for (count = 0; count < test_pos_count; ) {
		nblack = get_rand(max(npieces - MAX_PIECE, 0), min(npieces, MAX_PIECE));
		nwhite = npieces - nblack;
		nbm = get_rand(0, nblack);
		nbk = nblack - nbm;
		nwm = get_rand(0, nwhite);
		nwk = nwhite - nwm;
		range = egdb_index_range(nbm, nbk, nwm, nwk);
		index = get_rand64((__int64)0, range - 1);

		/* Get the position. */
		egdb_indextoposition(index, &pos.normal, nbm, nbk, nwm, nwk);

		/* Dont lookup capture positions. */
		if (egdb_iscapture(&pos.normal, EGDB_BLACK) || egdb_iscapture(&pos.normal, EGDB_WHITE))
			continue;

		if (bitmap_type == EGDB_ROW_REVERSED)
			normal_to_row_reversed(&pos.row_reversed, &pos.normal);

		testpos[count] = pos;
		++count;
	}

	show_driver_msgs = 0;
	db = egdb_open(bitmap_type, npieces, dbmem, dbdir, msg_fn);
	if (!db) {
		printf("Cannot open db for benchmarking\n");
		exit(1);
	}

	/* Lookup each position once to preload the lru cache buffers.
	 * This way there should be no disk i/o during the benchmark.
	 */
	show_driver_msgs = 1;
	(*db->reset_stats)(db);
	for (i = 0; i < test_pos_count; ++i)
		(*db->lookup)(db, testpos + i, i & 1, 0);

	/* Check some statistics. */
	stats = (*db->get_stats)(db);
	if (stats->db_requests != (unsigned int)test_pos_count)
		printf("db_requests incorrect, expected %d, got %d\n", test_pos_count, stats->db_requests);
	if (stats->db_returns != (unsigned int)test_pos_count)
		printf("db_returns incorrect, expected %d, got %d\n", test_pos_count, stats->db_returns);

	(*db->reset_stats)(db);
	t0 = GetTickCount();
	for (count = 0; count < REPEAT_COUNT; ++count) {
		for (i = 0; i < test_pos_count; ++i)
			(*db->lookup)(db, testpos + i, i & 1, 0);
	}

	/* Make sure there were no cache loads during the benchmarks.  We should
	 * have opened the driver with enough memory so that there are more
	 * cache buffers than test positions.
	 */
	stats = (*db->get_stats)(db);
	if (stats->lru_cache_loads > 0) {
		printf("Unexpected lru cache loads during benchmarks\n");
		print_stats(db);
	}

	printf("%.2fsec for %d %s %d piece lookups on db in %s\n", 
			(float)(GetTickCount() - t0) / 1000.0,
			REPEAT_COUNT * test_pos_count,
			bitmap_type == EGDB_NORMAL ? "normal" : "row_reversed",
			npieces,
			dbdir);
	(*db->close)(db);
}


void benchmark()
{
	printf("lookup benchmarks\n");
	benchmark_pieces(4, 100, EGDB_NORMAL);
	benchmark_pieces(4, 100, EGDB_ROW_REVERSED);
	benchmark_pieces(5, 100, EGDB_NORMAL);
	benchmark_pieces(5, 100, EGDB_ROW_REVERSED);
	benchmark_pieces(6, 100, EGDB_NORMAL);
	benchmark_pieces(6, 100, EGDB_ROW_REVERSED);
	benchmark_pieces(7, 100, EGDB_NORMAL);
	benchmark_pieces(7, 100, EGDB_ROW_REVERSED);
	benchmark_pieces(8, 100, EGDB_NORMAL);
	benchmark_pieces(8, 100, EGDB_ROW_REVERSED);
	benchmark_pieces(9, 100, EGDB_NORMAL);
	benchmark_pieces(9, 100, EGDB_ROW_REVERSED);
}


/*
 * Driver message function, writes the message to stdout.
 */
void __cdecl msg_fn(char *msg)
{
	if (show_driver_msgs)
	    printf(msg);
}


/*
 * Return a string description of a database type.
 */
char *db_description(EGDB_TYPE db_type)
{
	switch (db_type) {
	case EGDB_KINGSROW_WLD:
		return("KingsRow WLD");
	case EGDB_KINGSROW_MTC:
		return("KingsRow MTC");
	case EGDB_CAKE_WLD:
		return("Cake WLD");
	case EGDB_CHINOOK_WLD:
		return("Chinook WLD");
	}
	return("unknown");
}

/* A table of a few known values for a quick test of lookup. */
DBVALUE test_table[] = {
	{4, {0x00000010, 0x00008600, 0x00008000}, EGDB_BLACK, EGDB_LOSS},
	{4, {0x01800880, 0x00000000, 0x00000000}, EGDB_WHITE, EGDB_LOSS},
	{4, {0x00000202, 0x80001000, 0x00000000}, EGDB_BLACK, EGDB_DRAW},
	{4, {0x00000000, 0x02801040, 0x02001000}, EGDB_WHITE, EGDB_WIN},
	{4, {0x00009200, 0x01000000, 0x00000000}, EGDB_BLACK, EGDB_WIN},
	{4, {0x00000002, 0x00080108, 0x0008000a}, EGDB_WHITE, EGDB_WIN},
	{4, {0x08002000, 0x04100000, 0x00102000}, EGDB_BLACK, EGDB_DRAW},
	{4, {0x00500800, 0x00000004, 0x00000004}, EGDB_WHITE, EGDB_LOSS},
	{4, {0x10000208, 0x00400000, 0x10000208}, EGDB_BLACK, EGDB_WIN},
	{4, {0x40002000, 0x80000020, 0xc0002020}, EGDB_WHITE, EGDB_DRAW},
	{5, {0x00012400, 0xa0000000, 0x80012000}, EGDB_BLACK, EGDB_WIN},
	{5, {0x00080000, 0x00214200, 0x00000000}, EGDB_WHITE, EGDB_WIN},
	{5, {0x00804006, 0x00400000, 0x00804006}, EGDB_BLACK, EGDB_WIN},
	{5, {0x00008000, 0x01200180, 0x00000080}, EGDB_WHITE, EGDB_WIN},
	{5, {0x004004a0, 0x10000000, 0x10000000}, EGDB_BLACK, EGDB_WIN},
	{5, {0x0c001000, 0x00000030, 0x00001000}, EGDB_WHITE, EGDB_LOSS},
	{5, {0x10042400, 0x00000200, 0x10040000}, EGDB_BLACK, EGDB_WIN},
	{5, {0x00801100, 0x40008000, 0x00009100}, EGDB_WHITE, EGDB_LOSS},
	{5, {0x84000001, 0x20020000, 0x84000001}, EGDB_BLACK, EGDB_WIN},
	{5, {0x00000208, 0x82100000, 0x02100000}, EGDB_WHITE, EGDB_WIN},
	{6, {0x00022000, 0x0c000042, 0x04000042}, EGDB_BLACK, EGDB_LOSS},
	{6, {0x44050000, 0x00000900, 0x40010000}, EGDB_WHITE, EGDB_LOSS},
	{6, {0x04820200, 0x40000800, 0x00020000}, EGDB_BLACK, EGDB_WIN},
	{6, {0x00008814, 0x04200000, 0x00008014}, EGDB_WHITE, EGDB_LOSS},
	{6, {0x00070000, 0x22000020, 0x00050000}, EGDB_BLACK, EGDB_WIN},
	{6, {0x00000003, 0x08040410, 0x00000000}, EGDB_WHITE, EGDB_WIN},
	{6, {0x00601002, 0x10040000, 0x00000000}, EGDB_BLACK, EGDB_WIN},
	{6, {0x00040002, 0x14020400, 0x14060400}, EGDB_WHITE, EGDB_WIN},
	{6, {0x20080000, 0x08200140, 0x20000000}, EGDB_BLACK, EGDB_LOSS},
	{6, {0x00009804, 0x08200000, 0x08200800}, EGDB_WHITE, EGDB_LOSS},
	{7, {0x01080022, 0x00118000, 0x00008000}, EGDB_BLACK, EGDB_WIN},
	{7, {0x04000510, 0x008a0000, 0x04020400}, EGDB_WHITE, EGDB_LOSS},
	{7, {0x20204000, 0x80401040, 0xa0605040}, EGDB_BLACK, EGDB_LOSS},
	{7, {0x010d0000, 0x00000920, 0x01090920}, EGDB_WHITE, EGDB_LOSS},
	{7, {0x44000080, 0x20008220, 0x64008000}, EGDB_BLACK, EGDB_LOSS},
	{7, {0x00600020, 0x0800008c, 0x0860000c}, EGDB_WHITE, EGDB_WIN},
	{7, {0x00010110, 0x30800080, 0x30800110}, EGDB_BLACK, EGDB_LOSS},
	{7, {0x10484000, 0x00200022, 0x10480002}, EGDB_WHITE, EGDB_LOSS},
	{7, {0x04150000, 0x10000101, 0x14140101}, EGDB_BLACK, EGDB_WIN},
	{7, {0x02102010, 0x04088000, 0x02002000}, EGDB_WHITE, EGDB_LOSS},
	{8, {0x04000444, 0x60820000, 0x00000000}, EGDB_BLACK, EGDB_WIN},
	{8, {0x00090810, 0x20044200, 0x00000010}, EGDB_WHITE, EGDB_DRAW},
	{8, {0x80010030, 0x42082000, 0x80000000}, EGDB_BLACK, EGDB_DRAW},
	{8, {0x00000382, 0x30800010, 0x30800000}, EGDB_WHITE, EGDB_WIN},
	{8, {0x01004480, 0x24180000, 0x24080000}, EGDB_BLACK, EGDB_LOSS},
	{8, {0x86000020, 0x0010c080, 0x80004000}, EGDB_WHITE, EGDB_WIN},
	{8, {0x00201480, 0x40800030, 0x40800010}, EGDB_BLACK, EGDB_LOSS},
	{8, {0x00088048, 0x50030000, 0x00088048}, EGDB_WHITE, EGDB_LOSS},
	{8, {0x80000089, 0x01900010, 0x80100089}, EGDB_BLACK, EGDB_WIN},
	{8, {0x0010c004, 0xa0012000, 0x00000004}, EGDB_WHITE, EGDB_DRAW},
	{9, {0x00800c14, 0x81010020, 0x00000020}, EGDB_BLACK, EGDB_WIN},
	{9, {0x04000906, 0x28000088, 0x0c00018e}, EGDB_WHITE, EGDB_LOSS},
	{9, {0x0010c400, 0x00e00011, 0x0010c011}, EGDB_BLACK, EGDB_DRAW},
	{9, {0x08201800, 0x30050200, 0x00000000}, EGDB_WHITE, EGDB_WIN},
	{9, {0x1001040c, 0x20040a00, 0x30050a04}, EGDB_BLACK, EGDB_DRAW},
	{9, {0x30810008, 0x80000406, 0xb081040e}, EGDB_WHITE, EGDB_LOSS},
	{9, {0x00800701, 0x1000080c, 0x10800f0d}, EGDB_BLACK, EGDB_WIN},
	{9, {0x00108900, 0x82004030, 0x00000000}, EGDB_WHITE, EGDB_WIN},
	{9, {0x04020805, 0x20040018, 0x2406081d}, EGDB_BLACK, EGDB_WIN},
	{9, {0x40040101, 0xa10000c0, 0x41040180}, EGDB_WHITE, EGDB_WIN},
	10	/* Marks the end of the table (until I finish db10). */
};

