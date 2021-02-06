// Taken and adapted from https://github.com/d0k3/GodMode9

#include "disadiff.h"

#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define getbe16(d) \
    (((d)[0]<<8) | (d)[1])
#define getbe32(d) \
    ((((u32) getbe16(d))<<16) | ((u32) getbe16(d+2)))
#define getbe64(d) \
    ((((u64) getbe32(d))<<32) | ((u64) getbe32(d+4)))
#define getle16(d) \
    (((d)[1]<<8) | (d)[0])
#define getle32(d) \
    ((((u32) getle16(d+2))<<16) | ((u32) getle16(d)))
#define getle64(d) \
((((u64) getle32(d+4))<<32) | ((u64) getle32(d)))

// info taken from here:
// http://3dbrew.org/wiki/DISA_and_DIFF
// https://github.com/wwylele/3ds-save-tool

#define DISA_MAGIC  'D', 'I', 'S', 'A', 0x00, 0x00, 0x04, 0x00
#define DIFF_MAGIC  'D', 'I', 'F', 'F', 0x00, 0x00, 0x03, 0x00
#define IVFC_MAGIC  'I', 'V', 'F', 'C', 0x00, 0x00, 0x02, 0x00
#define DPFS_MAGIC  'D', 'P', 'F', 'S', 0x00, 0x00, 0x01, 0x00
#define DIFI_MAGIC  'D', 'I', 'F', 'I', 0x00, 0x00, 0x01, 0x00


typedef struct {
    u8  magic[8]; // "DISA" 0x00040000
    u32 n_partitions;
    u8  padding0[4];
    u64 offset_table1;
    u64 offset_table0;
    u64 size_table;
    u64 offset_descA;
    u64 size_descA;
    u64 offset_descB;
    u64 size_descB;
    u64 offset_partitionA;
    u64 size_partitionA;
    u64 offset_partitionB;
    u64 size_partitionB;
    u8  active_table; // 0 or 1
    u8  padding1[3];
    u8  hash_table[0x20]; // for the active table
    u8  unused[0x74];
} __attribute__((packed)) DisaHeader;

typedef struct {
    u8  magic[8]; // "DIFF" 0x00030000
    u64 offset_table1; // also desc offset
    u64 offset_table0; // also desc offset
    u64 size_table; // includes desc size
    u64 offset_partition;
    u64 size_partition;
    u32 active_table; // 0 or 1
    u8  hash_table[0x20]; // for the active table
    u64 unique_id; // see: http://3dbrew.org/wiki/Extdata
    u8  unused[0xA4];
} __attribute__((packed)) DiffHeader;

typedef struct {
    u8  magic[8]; // "DIFI" 0x00010000
    u64 offset_ivfc; // always 0x44
    u64 size_ivfc; // always 0x78
    u64 offset_dpfs; // always 0xBC
    u64 size_dpfs; // always 0x50
    u64 offset_hash; // always 0x10C
    u64 size_hash; // may include padding
    u8  ivfc_use_extlvl4;
    u8  dpfs_lvl1_selector;
    u8  padding[2];
    u64 ivfc_offset_extlvl4;
} __attribute__((packed)) DifiHeader;

typedef struct {
    u8  magic[8]; // "IVFC" 0x00020000
    u64 size_hash; // same as the one in DIFI, may include padding
    u64 offset_lvl1;
    u64 size_lvl1;
    u32 log_lvl1;
    u8  padding0[4];
    u64 offset_lvl2;
    u64 size_lvl2;
    u32 log_lvl2;
    u8  padding1[4];
    u64 offset_lvl3;
    u64 size_lvl3;
    u32 log_lvl3;
    u8  padding2[4];
    u64 offset_lvl4;
    u64 size_lvl4;
    u64 log_lvl4;
    u64 size_ivfc; // 0x78
} __attribute__((packed)) IvfcDescriptor;

typedef struct {
    u8  magic[8]; // "DPFS" 0x00010000
    u64 offset_lvl1;
    u64 size_lvl1;
    u32 log_lvl1;
    u8  padding0[4];
    u64 offset_lvl2;
    u64 size_lvl2;
    u32 log_lvl2;
    u8  padding1[4];
    u64 offset_lvl3;
    u64 size_lvl3;
    u32 log_lvl3;
    u8  padding2[4];
} __attribute__((packed)) DpfsDescriptor;

typedef struct {
    DifiHeader difi;
    IvfcDescriptor ivfc;
    DpfsDescriptor dpfs;
    u8 hash[0x20];
    u8 padding[4]; // all zeroes when encrypted
} __attribute__((packed)) DifiStruct;

// condensed info to enable reading IVFC lvl4
typedef struct {
    u32 offset_dpfs_lvl1; // relative to start of file
    u32 offset_dpfs_lvl2; // relative to start of file
    u32 offset_dpfs_lvl3; // relative to start of file
    u32 size_dpfs_lvl1;
    u32 size_dpfs_lvl2;
    u32 size_dpfs_lvl3;
    u32 log_dpfs_lvl2;
    u32 log_dpfs_lvl3;
    u32 offset_ivfc_lvl4; // relative to DPFS lvl3 if not external
    u32 size_ivfc_lvl4;
    u8  dpfs_lvl1_selector;
    u8  ivfc_use_extlvl4;
    u8* dpfs_lvl2_cache; // optional, NULL when unused
} __attribute__((packed)) DisaDiffReaderInfo;

#define TICKDB_AREA_OFFSET  0xA1C00 // offset inside the decoded DIFF partition
#define TICKDB_AREA_SIZE 0x00500000 

// from: https://github.com/profi200/Project_CTR/blob/02159e17ee225de3f7c46ca195ff0f9ba3b3d3e4/ctrtool/tik.h#L15-L39
// all numbers in big endian
typedef struct {
    u8 sig_type[4];
    u8 signature[0x100];
    u8 padding1[0x3C];
    u8 issuer[0x40];
    u8 ecdsa[0x3C];
    u8 version;
    u8 ca_crl_version;
    u8 signer_crl_version;
    u8 titlekey[0x10];
    u8 reserved0;
    u8 ticket_id[8];
    u8 console_id[4];
    u8 title_id[8];
    u8 sys_access[2];
    u8 ticket_version[2];
    u8 time_mask[4];
    u8 permit_mask[4];
    u8 title_export;
    u8 commonkey_idx;
    u8 reserved1[0x2A];
    u8 eshop_id[4];
    u8 reserved2;
    u8 audit;
    u8 content_permissions[0x40];
    u8 reserved3[2];
    u8 timelimits[0x40];
    u8 content_index[0xAC];
} __attribute__((packed)) Ticket;

typedef struct {
    u32 commonkey_idx;
    u8  reserved[4];
    u8  title_id[8];
    u8  titlekey[16];
} __attribute__((packed)) TitleKeyEntry;

typedef struct {
    u32 n_entries;
    u8  reserved[12];
} __attribute__((packed)) TitleKeysInfoHeader;

typedef struct {
    TitleKeysInfoHeader header;
    TitleKeyEntry entries[300*2]; // title limit * 2 because people are crazy
} __attribute__((packed)) TitleKeysInfo;

#define GET_DPFS_BIT(b, lvl) (((((u32*) (void*) lvl)[b >> 5]) >> (31 - (b % 32))) & 1)

inline static u64 DisaDiffSize(Handle handle)
{
    u64 size = 0;
    FSFILE_GetSize(handle, &size);
    return size;
}

inline static Result DisaDiffRead(Handle handle, void* buf, u32 btr, u64 ofs)
{
    u32 br;
    Result res = FSFILE_Read(handle, &br, ofs, buf, btr);
    if((res == 0) && (br != btr)) res = -1;
    return res;
}

inline static Result DisaDiffClose(Handle handle) 
{
    return FSFILE_Close(handle);
}

inline static Result DisaDiffQRead(Handle handle, void* buf, u64 ofs, u32 btr) {
    return FSFILE_Read(handle, NULL, ofs, buf, btr);
}

static u32 GetDisaDiffReaderInfo(Handle handle, DisaDiffReaderInfo* info, bool partitionB) {
    const u8 disa_magic[] = { DISA_MAGIC };
    const u8 diff_magic[] = { DIFF_MAGIC };
    const u8 ivfc_magic[] = { IVFC_MAGIC };
    const u8 dpfs_magic[] = { DPFS_MAGIC };
    const u8 difi_magic[] = { DIFI_MAGIC };

    // reset reader info
    memset(info, 0x00, sizeof(DisaDiffReaderInfo));

    // get file size, header at header offset
    u64 file_size = DisaDiffSize(handle);
    u8 header[0x100];
    if(R_FAILED(DisaDiffQRead(handle, header, 0x100, 0x100))) return 1;

    // DISA/DIFF header: find partition offset & size and DIFI descriptor
    u32 offset_partition = 0;
    u32 size_partition = 0;
    u32 offset_difi = 0;
    if(memcmp(header, disa_magic, 8) == 0) { // DISA file
	   DisaHeader* disa = (DisaHeader*) (void*) header;
	   offset_difi = (disa->active_table) ? disa->offset_table1 : disa->offset_table0;
	   if(!partitionB) {
		  offset_partition = (u32) disa->offset_partitionA;
		  size_partition = (u32) disa->size_partitionA;
		  offset_difi += (u32) disa->offset_descA;
	   } else {
		  if(disa->n_partitions != 2) return 1;
		  offset_partition = (u32) disa->offset_partitionB;
		  size_partition = (u32) disa->size_partitionB;
		  offset_difi += (u32) disa->offset_descB;
	   }
    } else if(memcmp(header, diff_magic, 8) == 0) { // DIFF file
	   if(partitionB) return 1;
	   DiffHeader* diff = (DiffHeader*) (void*) header;
	   offset_partition = (u32) diff->offset_partition;
	   size_partition = (u32) diff->size_partition;
	   offset_difi = (diff->active_table) ? diff->offset_table1 : diff->offset_table0;
    }

    // check the output so far
    if(!offset_difi || (offset_difi + sizeof(DifiStruct) > file_size) ||
	   (offset_partition + size_partition > file_size))
	   return 1;

    // read DIFI struct from filr
    DifiStruct difis;
    if(R_FAILED(DisaDiffQRead(handle, &difis, offset_difi, sizeof(DifiStruct))))
	   return 1;
    if((memcmp(difis.difi.magic, difi_magic, 8) != 0) ||
	   (memcmp(difis.ivfc.magic, ivfc_magic, 8) != 0) ||
	   (memcmp(difis.dpfs.magic, dpfs_magic, 8) != 0))
	   return 1;

    // check & get data from DIFI header
    DifiHeader* difi = &(difis.difi);
    if((difi->offset_ivfc != sizeof(DifiHeader)) ||
	   (difi->size_ivfc != sizeof(IvfcDescriptor)) ||
	   (difi->offset_dpfs != difi->offset_ivfc + difi->size_ivfc) ||
	   (difi->size_dpfs != sizeof(DpfsDescriptor)) ||
	   (difi->offset_hash != difi->offset_dpfs + difi->size_dpfs) ||
	   (difi->size_hash < 0x20))
	   return 1;

    info->dpfs_lvl1_selector = difi->dpfs_lvl1_selector;
    info->ivfc_use_extlvl4 = difi->ivfc_use_extlvl4;
    info->offset_ivfc_lvl4 = (u32) (offset_partition + difi->ivfc_offset_extlvl4);

    // check & get data from DPFS descriptor
    DpfsDescriptor* dpfs = &(difis.dpfs);
    if((dpfs->offset_lvl1 + dpfs->size_lvl1 > dpfs->offset_lvl2) ||
	   (dpfs->offset_lvl2 + dpfs->size_lvl2 > dpfs->offset_lvl3) ||
	   (dpfs->offset_lvl3 + dpfs->size_lvl3 > size_partition) ||
	   (2 > dpfs->log_lvl2) || (dpfs->log_lvl2 > dpfs->log_lvl3) ||
	   !dpfs->size_lvl1 || !dpfs->size_lvl2 || !dpfs->size_lvl3)
	   return 1;

    info->offset_dpfs_lvl1 = (u32) (offset_partition + dpfs->offset_lvl1);
    info->offset_dpfs_lvl2 = (u32) (offset_partition + dpfs->offset_lvl2);
    info->offset_dpfs_lvl3 = (u32) (offset_partition + dpfs->offset_lvl3);
    info->size_dpfs_lvl1 = (u32) dpfs->size_lvl1;
    info->size_dpfs_lvl2 = (u32) dpfs->size_lvl2;
    info->size_dpfs_lvl3 = (u32) dpfs->size_lvl3;
    info->log_dpfs_lvl2 = (u32) dpfs->log_lvl2;
    info->log_dpfs_lvl3 = (u32) dpfs->log_lvl3;

    // check & get data from IVFC descriptor
    IvfcDescriptor* ivfc = &(difis.ivfc);
    if((ivfc->size_hash != difi->size_hash) ||
	   (ivfc->size_ivfc != sizeof(IvfcDescriptor)) ||
	   (ivfc->offset_lvl1 + ivfc->size_lvl1 > ivfc->offset_lvl2) ||
	   (ivfc->offset_lvl2 + ivfc->size_lvl2 > ivfc->offset_lvl3) ||
	   (ivfc->offset_lvl3 + ivfc->size_lvl3 > dpfs->size_lvl3))
	   return 1;
    if(!info->ivfc_use_extlvl4) {
	   if((ivfc->offset_lvl3 + ivfc->size_lvl3 > ivfc->offset_lvl4) ||
		  (ivfc->offset_lvl4 + ivfc->size_lvl4 > dpfs->size_lvl3))
		  return 1;
	   info->offset_ivfc_lvl4 = (u32) ivfc->offset_lvl4;
    } else if(info->offset_ivfc_lvl4 + ivfc->size_lvl4 > offset_partition + size_partition) {
	   return 1;
    }
    info->size_ivfc_lvl4 = (u32) ivfc->size_lvl4;

    return 0;
}

static u32 BuildDisaDiffDpfsLvl2Cache(Handle handle, DisaDiffReaderInfo* info, u8* cache, u32 cache_size) {
    const u32 min_cache_bits = (info->size_dpfs_lvl3 + (1 << info->log_dpfs_lvl3) - 1) >> info->log_dpfs_lvl3;
    const u32 min_cache_size = ((min_cache_bits + 31) >> (3 + 2)) << 2;
    const u32 offset_lvl1 = info->offset_dpfs_lvl1 + ((info->dpfs_lvl1_selector) ? info->size_dpfs_lvl1 : 0);

    // safety (this still assumes all the checks from GetDisaDiffReaderInfo())
    if(info->ivfc_use_extlvl4 ||
	   (cache_size < min_cache_size) ||
	   (min_cache_size > info->size_dpfs_lvl2) ||
	   (min_cache_size > (info->size_dpfs_lvl1 << (3 + info->log_dpfs_lvl2))))
	   return 1;

    // allocate memory
    u8* lvl1 = (u8*) malloc(info->size_dpfs_lvl1);
    if(!lvl1) return 1; // this is never more than 8 byte in reality -___-

    // read lvl1
    u32 ret = 0;
    if((ret != 0) || DisaDiffRead(handle, lvl1, info->size_dpfs_lvl1, offset_lvl1)) ret = 1;

    // read full lvl2_0 to cache
    if((ret != 0) || DisaDiffRead(handle, cache, info->size_dpfs_lvl2, info->offset_dpfs_lvl2)) ret = 1;

    // cherry-pick lvl2_1
    u32 log_lvl2 = info->log_dpfs_lvl2;
    u32 offset_lvl2_1 = info->offset_dpfs_lvl2 + info->size_dpfs_lvl2;
    for (u32 i = 0; (ret == 0) && ((i << (3 + log_lvl2)) < min_cache_size); i += 4) {
	   u32 dword = *(u32*) (void*) (lvl1 + i);
	   for (u32 b = 0; b < 32; b++) {
		  if((dword >> (31 - b)) & 1) {
			 u32 offset = ((i << 3) + b) << log_lvl2;
			 if(R_FAILED(DisaDiffRead(handle, (u8*) cache + offset, 1 << log_lvl2, offset_lvl2_1 + offset))) ret = 1;
		  }
	   }
    }

    info->dpfs_lvl2_cache = cache;
    free(lvl1);
    return ret;
}

static u32 ReadDisaDiffIvfcLvl4(Handle handle, DisaDiffReaderInfo* info, u32 offset, u32 size, void* buffer) {
    // data: full DISA/DIFF file in memory
    // offset: offset inside IVFC lvl4

    // DisaDiffReaderInfo not provided?
    DisaDiffReaderInfo info_l;
    u8* cache = NULL;
    if(!info)
    {
	   info = &info_l;
	   if(GetDisaDiffReaderInfo(handle, info, false) != 0) return 0;
	   cache = malloc(info->size_dpfs_lvl2);
	   if(!cache) return 0;
	   if(BuildDisaDiffDpfsLvl2Cache(handle, info, cache, info->size_dpfs_lvl2) != 0)
	   {
		  free(cache);
		  return 0;
	   }
    }
   
    // sanity checks - offset & size
    if(offset > info->size_ivfc_lvl4)
	   return 0;
    else if(offset + size > info->size_ivfc_lvl4)
	   size = info->size_ivfc_lvl4 - offset;

    if(info->ivfc_use_extlvl4)
    {
	   // quick reading in case of external IVFC lvl4
	   if(R_FAILED(DisaDiffQRead(handle, buffer, info->offset_ivfc_lvl4 + offset, size)))
		  size = 0;
    }
    else
    {
	   // full reading in standard case
	   const u32 offset_start = info->offset_ivfc_lvl4 + offset;
	   const u32 offset_end = offset_start + size;
	   const u8* lvl2 = info->dpfs_lvl2_cache;
	   const u32 offset_lvl3_0 = info->offset_dpfs_lvl3;
	   const u32 offset_lvl3_1 = offset_lvl3_0 + info->size_dpfs_lvl3;
	   const u32 log_lvl3 = info->log_dpfs_lvl3;

	   u32 read_start = offset_start;
	   u32 read_end = read_start;
	   u32 bit_state = 0;

	   // full reading below
	   while (size && (read_start < offset_end)) {
		  // read bits until bit_state does not match
		  // idx_lvl2 is a bit offset
		  u32 idx_lvl2 = read_end >> log_lvl3;
		  if(GET_DPFS_BIT(idx_lvl2, lvl2) == bit_state) {
			 read_end = (idx_lvl2+1) << log_lvl3;
			 if(read_end >= offset_end) read_end = offset_end;
			 else continue;
		  }
		  // read data if there is any
		  if(read_start < read_end) {
			 const u32 pos_f = (bit_state ? offset_lvl3_1 : offset_lvl3_0) + read_start;
			 const u32 pos_b = read_start - offset_start;
			 const u32 btr = read_end - read_start;
			 if(R_FAILED(DisaDiffRead(handle, (u8*) buffer + pos_b, btr, pos_f))) size = 0;
			 read_start = read_end;
		  }
		  // flip the bit_state
		  bit_state = ~bit_state & 0x1;
	   }
    }

    if(cache) free(cache);
    return size;
}

void extractTitleKeysTo(const char * outpath)
{
    FILE * fh = fopen(outpath, "wb");
    if (fh)
    {
	   Handle ticketDB;
	   if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&ticketDB, ARCHIVE_NAND_CTR_FS, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/dbs/ticket.db"), FS_OPEN_READ, 0)))
	   {
		  u8 * data = (u8*)malloc(TICKDB_AREA_SIZE);
		  if (data && ReadDisaDiffIvfcLvl4(ticketDB, NULL, TICKDB_AREA_OFFSET, TICKDB_AREA_SIZE, data) == TICKDB_AREA_SIZE)
		  {
			 TitleKeysInfo tik_info = {0};

			 for (u32 i = 0; i < TICKDB_AREA_SIZE + 0x400; i += 0x200)
			 {
				Ticket* ticket = (Ticket*)((data + i) + 0x18);
				if (getle64(ticket->title_id) == 0 || ticket->commonkey_idx >= 2) continue;
				TitleKeyEntry tik = {0};
				memcpy(tik.title_id, ticket->title_id, 8);
				memcpy(tik.titlekey, ticket->titlekey, 16);
				tik.commonkey_idx = ticket->commonkey_idx;
				memcpy(&tik_info.entries[tik_info.header.n_entries], &tik, sizeof(TitleKeyEntry));
				tik_info.header.n_entries++;
			 }

			 fwrite(&tik_info.header, sizeof(tik_info.header), 1, fh);
			 fwrite(tik_info.entries, sizeof(TitleKeyEntry), tik_info.header.n_entries, fh);
		  }
		  free(data);
		  FSFILE_Close(ticketDB);
	   }
	   fclose(fh);
    }
}

