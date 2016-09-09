/*
 * LDPC code constants/functions
 * Copyright 2016 Adam Greig
 *
 * Codes from CCSDS documents 231.1-O-1 and 131.0-B-2.
 * https://public.ccsds.org/default.aspx
 */

#include <string.h>
#include "ldpc_codes.h"

static void get_code_params(enum ldpc_code code, int* n, int* k, int* m);
static void init_parity_tc(enum ldpc_code code, uint32_t* h);
static void init_parity_tm(enum ldpc_code code, uint32_t* h);
static void init_parity_tm_sub(int m, int col0, int dwidth, int hcols,
                               uint8_t const * design, uint32_t* h);

/*
 * Generator matrix constants for the three CCSDS 231.1-O-1 codes,
 * in compact circulant form. Rows 1, M+1, 2M+1, 3M+1 are given, with
 * intermediate rows found by right circular shifts of these rows.
 * Note that the (128, 64) and (512, 256) codes are (at the time of writing )
 * in review in CCSDS 231.0-P-2.1 to become part of the recommendations,
 * but the (256, 128) code is removed. It's kept here because it's really
 * quite useful though.
 */
static const uint32_t g_n128_k64[4*2] = {
    0x0E69166B, 0xEF4C0BC2, 0x7766137E, 0xBB248418,
    0xC480FEB9, 0xCD53A713, 0x4EAA22FA, 0x465EEA11,
};

static const uint32_t g_n256_k128[4*4] = {
    0x73F5E839, 0x0220CE51, 0x36ED68E9, 0xF39EB162,
    0xBAC812C0, 0xBCD24379, 0x4786D928, 0x5A09095C,
    0x7DF83F76, 0xA5FF4C38, 0x8E6C0D4E, 0x025EB712,
    0xBAA37B32, 0x60CB31C5, 0xD0F66A31, 0xFAF511BC,
};

static const uint32_t g_n512_k256[4*8] = {
    0x1D21794A, 0x22761FAE, 0x59945014, 0x257E130D,
    0x74D60540, 0x03794014, 0x2DADEB9C, 0xA25EF12E,
    0x60E0B662, 0x3C5CE512, 0x4D2C81EC, 0xC7F469AB,
    0x20678DBF, 0xB7523ECE, 0x2B54B906, 0xA9DBE98C,
    0xF6739BCF, 0x54273E77, 0x167BDA12, 0x0C6C4774,
    0x4C071EFF, 0x5E32A759, 0x3138670C, 0x095C39B5,
    0x28706BD0, 0x45300258, 0x2DAB85F0, 0x5B9201D0,
    0x8DFDEE2D, 0x9D84CA88, 0xB371FAE6, 0x3A4EB07E,
};


/*
 * Parity check matrices corresponding to the above codes.
 * This representation mirrors the definition in CCSDS 231.1-O-1,
 * and can be expanded at runtime to create the actual matrix in memory.
 * Each macro represents a single MxM sub-matrix, where M=n/8.
 */
#define HZ          (0 << 6)                /* All-zero sub-matrix           */
#define HI          (1 << 6)                /* Identity matrix sub-matrix    */
#define HP          (2 << 6)                /* nth right circular shift of I */
#define HS          (HI | HP)               /* HI + HP(n)                    */

static const uint8_t h_n128_k64[4][8] = {
    {HS| 7, HP| 2, HP|14, HP| 6, HZ   , HP| 0, HP|13, HI   },
    {HP| 6, HS|15, HP| 0, HP| 1, HI   , HZ   , HP| 0, HP| 7},
    {HP| 4, HP| 1, HS|15, HP|14, HP|11, HI   , HZ   , HP| 3},
    {HP| 0, HP| 1, HP| 9, HS|13, HP|14, HP| 1, HI   , HZ   },
};

static const uint8_t h_n256_k128[4][8] = {
    {HS|31, HP|15, HP|25, HP| 0, HZ   , HP|20, HP|12, HI   },
    {HP|28, HS|30, HP|29, HP|24, HI   , HZ   , HP| 1, HP|20},
    {HP| 8, HP| 0, HS|28, HI| 1, HP|29, HI   , HZ   , HP|21},
    {HP|18, HP|30, HP| 0, HS|30, HP|25, HP|26, HI   , HZ   },
};

static const uint8_t h_n512_k256[4][8] = {
    {HS|63, HP|30, HP|50, HP|25, HZ   , HP|43, HP|62, HI   },
    {HP|56, HS|61, HP|50, HP|23, HI   , HZ   , HP|37, HP|26},
    {HP|16, HP| 0, HS|55, HP|27, HP|56, HI   , HZ   , HP|43},
    {HP|35, HP|56, HP|62, HS|11, HP|58, HP| 3, HI   , HZ   },
};


/*
 * Generator matrix constants for the CCSDS 131.0-B-2 LDPC codes,
 * in compact circulant form. Each row corresponds to one row in
 * the parity block, and intermediate rows are found by right circular
 * shifts of the specified rows.
 * See scripts/ccsds131_parity_to_generator.py for generation.
 * Only the k=1024 codes are included as they're all we plan to use and it's
 * 4kB of code space for each of the k=4096 codes, to say nothing of the
 * k=16384 codes! The constants for the k=4096 are available, the constants for
 * the largest code require either more RAM or a more efficient script.
 * In theory you should be able to add new codes by basically adding the
 * constants here, extending the ldpc_codes enum, and adding them in to each
 * switch(code) in this file. Right now phi_m_j_k only has the M=(128,256,512)
 * options, but you can also easily add the remaining numbers for other block
 * sizes (available in the python script). You'll probably need to change
 * the type to uint16_t, though.
 */
static const uint32_t g_n2048_k1024[8*32] = {
    0xCFA794F4, 0x9FA5A0D8, 0x8BB31D8F, 0xCA7EA8BB, 0xA7AE7EE8, 0xA68580E3,
    0xE922F9E1, 0x3359B284, 0x91F72AE8, 0xF2D6BF78, 0x30A1F83B, 0x3CDBD463,
    0xCE95C0EC, 0x1F609370, 0xD7E791C8, 0x70229C1E, 0x71EF3FDF, 0x60E28784,
    0x78934DB2, 0x85DEC9DC, 0x0E95C103, 0x008B6BCD, 0xD2DAF85C, 0xAE732210,
    0x8326EE83, 0xC1FBA56F, 0xDD15B2DD, 0xB31FE7F2, 0x3BA0BB43, 0xF83C67BD,
    0xA1F6AEE4, 0x6AEF4E62, 0x56508378, 0x0CA89ACA, 0xA70CCFB4, 0xA888AE35,
    0x1210FAD0, 0xEC9602CC, 0x8C96B0A8, 0x6D3996A3, 0xC0B07FDD, 0xA73454C2,
    0x5295F72B, 0xD5004E80, 0xACCF973F, 0xC30261C9, 0x90525AA0, 0xCBA006BD,
    0x9F079F09, 0xA405F7F8, 0x7AD98429, 0x096F2A7E, 0xEB8C9B13, 0xB84C06E4,
    0x2843A476, 0x89A9C528, 0xDAAA1A17, 0x5F598DCF, 0xDBAD426C, 0xA43AD479,
    0x1BA78326, 0xE75F38EB, 0x6ED09A45, 0x303A6425, 0x48F42033, 0xB7B9A051,
    0x49DC839C, 0x90291E98, 0x9B2CEBE5, 0x0A7C2C26, 0x4FC6E7D6, 0x74063589,
    0xF5B6DEAE, 0xBF72106B, 0xA9E66765, 0x64C17134, 0x6D595455, 0x8D235191,
    0x50AAF88D, 0x7008E634, 0x1FA962FB, 0xAB864A5F, 0x867C9D6C, 0xF4E087AA,
    0x5D7AA674, 0xBA4B1D8C, 0xD7AE9186, 0xF1D3B23B, 0x047F1127, 0x91EE97B6,
    0x3FB7B58F, 0xF3B94E95, 0x93BE39A6, 0x365C66B8, 0x77AD3169, 0x65A72F5B,
    0x1B58F88E, 0x49C00DC6, 0xB35855BF, 0xF228A088, 0x5C8ED47B, 0x61EEC66B,
    0x5004FB6E, 0x65CBECF3, 0x77789998, 0xFE80925E, 0x0237F570, 0xE04C5F5B,
    0xED677661, 0xEB7FC382, 0x5AB5D5D9, 0x68C0808C, 0x2BDB828B, 0x19593F41,
    0x671B8D0D, 0x41DF136C, 0xCB47553C, 0x9B3F0EA0, 0x16CC1554, 0xC35E6A7D,
    0x97587FEA, 0x91D2098E, 0x126EA73C, 0xC78658A6, 0xADE19711, 0x208186CA,
    0x95C7417A, 0x15690C45, 0xBE9C169D, 0x889339D9, 0x654C976A, 0x85CFD9F7,
    0x47C4148E, 0x3B4712DA, 0xA3BAD1AD, 0x71873D3A, 0x1CD630C3, 0x42C5EBB9,
    0x183ADE9B, 0xEF294E8E, 0x7014C077, 0xA5F96F75, 0xBE566C86, 0x6964D01C,
    0xE72AC43A, 0x35AD2166, 0x72EBB325, 0x9B77F9BB, 0x18DA8B09, 0x194FA1F0,
    0xE876A080, 0xC9D6A39F, 0x809B168A, 0x3D88E8E9, 0x3D995CE5, 0x232C2DC2,
    0xC7CFA44A, 0x363F628A, 0x668D46C3, 0x98CAF96F, 0xD57DBB24, 0xAE27ACA1,
    0x716F8EA1, 0xB8AA1086, 0x7B7796F4, 0xA86F1FD5, 0x4C7576AD, 0x01C68953,
    0xE75BE799, 0x02448236, 0x8F069658, 0xF7AAAFB0, 0x975F3AF7, 0x95E78D25,
    0x5871C71B, 0x4F4B77F6, 0x65CD9C35, 0x9BB2A82D, 0x5353E007, 0x166BDD41,
    0x2C544731, 0x4DB027B1, 0x0B130071, 0xAD0398D1, 0xDE19BC7A, 0x6BBCF6A0,
    0xFF021AAB, 0xF12920A5, 0x58BAED48, 0x4AF89E29, 0xD4DBC170, 0xCEF1D369,
    0x4C330B2D, 0x11E15B5C, 0xB3815E09, 0x605338A6, 0x75E3D1A3, 0x541E0E28,
    0x4F6556D6, 0x8D3C8A9E, 0xE5BB3B29, 0x7DB62CD2, 0x907F0999, 0x6967A0F4,
    0xFF33AEEE, 0x2C8A4A52, 0xFCCF5C39, 0xD355C39C, 0x5FE5F09A, 0xBA6BCCE0,
    0x2A73401E, 0x5F87EAC2, 0xD75702F4, 0xF57670DF, 0xA70B1C00, 0x2F523EEA,
    0x6CE1CE2E, 0x05D420CB, 0x867EC016, 0x6B8E53A9, 0x9DF9801A, 0x1C33058D,
    0xD116A0AE, 0x7278BBB9, 0x4CF0B0C7, 0x92DD8FDB, 0x3ECEAE6F, 0x2B7F663D,
    0x106A1C29, 0x6E47C14C, 0x1498B045, 0xD57DEFB5, 0x968F6D8C, 0x790263C3,
    0x53CF307E, 0xF90C1F21, 0x66E6B632, 0xF6614E58, 0x267EF096, 0xC37718A3,
    0x3D46E5D1, 0x0E993EB6, 0xDF81518F, 0x885EDA1B, 0x6FF518FD, 0x48BB8E9D,
    0xDBED4AC0, 0xF4F5EB89, 0xBCC64D21, 0xA65DB379, 0xABE2E4DC, 0x21F109FF,
    0x2EC0CE7B, 0x5D40973D, 0x13ECF713, 0xB01C6F10
};

static const uint32_t g_n1536_k1024[16*16] = {
    0x51236781, 0x781D416A, 0xB0C8419F, 0xA21559A8, 0x5F14E1E4, 0xD88726F1,
    0x762F6ED6, 0xCF32F06D, 0x8ABFD971, 0xE17A0BE9, 0xA5D14774, 0x1B698D14,
    0x2A58AB30, 0xE2BC32D3, 0x9F251FBC, 0x5DB8C768, 0xD73C205B, 0xBEB231CB,
    0xCAB5EFF5, 0xB2C76C71, 0xFA70FAD4, 0x8828355F, 0x68C6138F, 0xA5524A61,
    0xBB20031D, 0x7AA8FE69, 0x432ADE44, 0x6F49CE27, 0x5E5DB9CC, 0xCEBD1326,
    0xE8782B1B, 0x01F2ABA2, 0x4748E951, 0x3B41147A, 0x17B1FBB7, 0x8B4F914C,
    0x281F5680, 0xBA56DE50, 0x74B0FB08, 0x17E33E2B, 0xDD166CFB, 0x774B5959,
    0xAC7FDCEA, 0x4FECB5BE, 0xED747C81, 0xB540D66A, 0xB2A6A203, 0x9A87967F,
    0x4780DCB2, 0xDC5CBFAE, 0x55BC8FF8, 0x4EC89440, 0xE5D41122, 0x3F09979F,
    0xDDDE9D94, 0x0A15A801, 0x19406463, 0x9D254969, 0x1BE32DDC, 0x829B0032,
    0x1326515A, 0x22EE88A2, 0x0EC664DD, 0x2D701891, 0x69748DFE, 0x6372F2EF,
    0x15F3B0D4, 0x00ACD68A, 0xCF4144CE, 0x1FE2581C, 0x79B1A55B, 0xA59E54AE,
    0x65A2B47E, 0xEBAB0CF3, 0x24DD8757, 0x2CB0F71D, 0xF24ABF15, 0x590F4DA6,
    0x9C3BAE51, 0x969C6502, 0xD3A714B6, 0x0B22789B, 0x3DF5504D, 0x80F54C5A,
    0x9D75CF14, 0x65031211, 0x09834A0C, 0x9F659C99, 0xB9241BDF, 0x76EB3788,
    0x6F927251, 0xC86DECF1, 0x390BE9F5, 0xBBB93D05, 0xC6F435BF, 0xA1FF96B6,
    0x222461B6, 0x58DC3E91, 0xB01DF2A2, 0xEAD2DAA6, 0x5572EE62, 0x78F6F63A,
    0x17B63CB2, 0xFDA3B97F, 0xB233BB25, 0x9F3D83F7, 0xF64760C7, 0x74989384,
    0x46F57E03, 0xF55B1C0B, 0x5AC8A6CE, 0xA05466C1, 0xAE882552, 0x1F85CA31,
    0x37BEED74, 0xB5303407, 0x751FC9A1, 0x5FCEE486, 0x93F0F69B, 0xD04E72A4,
    0xC0EBFA3F, 0x49DF4DBB, 0x03E52D81, 0x5DC99A1D, 0x98FE8BF0, 0x1BB2CD6D,
    0x009C5290, 0xD81A18F6, 0x4FFBAD88, 0x545CAA95, 0x0C74659F, 0xA4828CA3,
    0x60CE56E3, 0x2DA28B2E, 0x299D4BF8, 0x2FE54B81, 0x51047BE3, 0xB3AE4F4B,
    0xF3AC9578, 0xB9477A4C, 0x3730F81F, 0x92767E11, 0x04E84EC3, 0xA3AD1F19,
    0x2D0E0CAB, 0x8EDD2185, 0xCEFBE8F2, 0xF538522A, 0x92DAEDC2, 0x2C441893,
    0xBCB99915, 0x7B35619D, 0x069951BF, 0xB90A08E1, 0x54C7E270, 0xCBA1656E,
    0x7FBBB806, 0xB6A06FB3, 0x7224943B, 0x1C3A5723, 0x1BAA1475, 0x2EFCEBC0,
    0xCFF08949, 0x75557623, 0xFA95908D, 0xC3F34D48, 0xFECA6509, 0x99A26E91,
    0x245433EB, 0xBE9CDA13, 0x5771EAFF, 0x9B02D8FC, 0xBCEBCA57, 0x3D3775C8,
    0x1E46F2B9, 0x51D0EAAB, 0x32942F7F, 0x4743DDF4, 0x8FA2F60A, 0xD62095EF,
    0x80E4A736, 0xB5E1A3A3, 0x01190628, 0x72DAEDF4, 0xE7800695, 0x8CD99F95,
    0xD2062505, 0x7C99C7A3, 0xB569736D, 0xE2167610, 0x0E1C6183, 0xADF09FD0,
    0xE5C492DB, 0xB48B319A, 0xE2D83ADE, 0xFEBBDEFE, 0xAA944EEA, 0x53C77DB3,
    0x0FAA85D9, 0xC13B1F73, 0x8ACED57F, 0x3BE4E807, 0x33CB7262, 0x7624F426,
    0xA0C6E669, 0xB5C74980, 0xABBAEFEA, 0x2D3B69AA, 0xF8366DDA, 0xE56A6DDC,
    0xFDED5582, 0xF4EA6525, 0x4C962827, 0x8ED17036, 0x6E711B6D, 0x20A67966,
    0x3B28BDF0, 0x04C21B93, 0x1BC37B73, 0x0FFC1786, 0x5D20C81D, 0x345FE4B9,
    0x1D14A566, 0x3D369A93, 0x5EBD4BD3, 0x9B2217D0, 0x56833BE1, 0xCDDBA6BC,
    0xB288169B, 0x4E3BB726, 0xC2ED28FB, 0xFC395D1F, 0x035B30C6, 0x8F9A6B6F,
    0x539836A6, 0xE56A7B16, 0xCEB1525C, 0x6ADB65A5, 0x5F71754A, 0xA458B11A,
    0x0DB9D180, 0xB21C0B13, 0x417D86C5, 0x9DF33E49, 0x183A8F6C, 0x44DAFA24,
    0x4E224C18, 0x0C1F0B45, 0xC93CD9CA, 0x23658555, 0x7DDEC5E9, 0x451AD519,
    0xB122C72A, 0x6177EE99, 0x1290B4C6, 0xB007D973
};

static const uint32_t g_n1280_k1024[32*8] = {
    0x678ECB51, 0xFE821D5C, 0xFA5F424B, 0xF55927AA, 0x3E826913, 0x32E04B0C,
    0x4F88862B, 0x803432EF, 0x42B27625, 0x9F8DA1E1, 0xF8472D1B, 0xD943D394,
    0x29261575, 0xBA434C68, 0x18EF349A, 0x27CA1CC4, 0xEC900397, 0x64A4A063,
    0x9BCEC4A6, 0xD05BA70F, 0xE7155BE1, 0x7FF09CC1, 0x6E2E2059, 0x7F1567E5,
    0x5616101C, 0xEA060E2B, 0xB673068B, 0x923BDF8B, 0xB9B9343D, 0x049C63A8,
    0x333E9CFE, 0x809B362D, 0x9D41634C, 0x404E17DA, 0x3B4161F2, 0x5235992E,
    0xEA4B4B8B, 0x4690BCE1, 0xF9DA36A1, 0x16439BB1, 0x5D7254B5, 0x15B4978B,
    0x00D05224, 0x107BD904, 0xC85D7E58, 0x0451F1A5, 0xEE9D1897, 0x913DA6F9,
    0x42819F61, 0x343773CA, 0x11A6492A, 0x4832F43F, 0x849C11ED, 0xF0FE864F,
    0xCC270400, 0x9726D66E, 0x89EE2A44, 0x685C1F67, 0x1DF6E416, 0x507BF2EF,
    0x8759C2FB, 0x52162ABF, 0x2B61D3FB, 0x988708C4, 0x4A8FEA09, 0x53452354,
    0xA33E2E73, 0x271E8211, 0x16DF62E5, 0x03DF81F4, 0x8848BD0F, 0xF95DF357,
    0x9BE0A7B3, 0x617256EB, 0x9A4D0BB4, 0xFE3A3A19, 0xFAA63D9E, 0x65328918,
    0xD699BA35, 0x4CDE6FE0, 0x848B1FE5, 0x0AB58A6F, 0x341707F1, 0xEF36474B,
    0xF623A7A5, 0xA35EC9BA, 0x24909B6E, 0x64A7A898, 0xBDDF3BAE, 0x7202FA26,
    0x86F90C57, 0xA0399F20, 0x972B9A31, 0x87B245AE, 0xE0C5A338, 0x4959AAD9,
    0xCF726C27, 0x7B38429A, 0xBA37C244, 0xEE7717DB, 0xE45C99CA, 0x7E3E013B,
    0x7B800CA4, 0x6527F2E7, 0x75C63782, 0x1CC40137, 0x51E69F16, 0x414B155F,
    0xDF1964DE, 0xF13C71F7, 0x6E9E8044, 0x6C5CEC86, 0x6F2A6DF8, 0x9FF2BF82,
    0xD3625355, 0x24466981, 0xD5F14AC1, 0xE1C24AEA, 0xA8850D83, 0x7A3C5120,
    0xBAABADC3, 0x1ECF066D, 0x76538348, 0xFC5D4D54, 0x43AD46CF, 0x3342012C,
    0x63EBE2DC, 0xD832EF8E, 0xE6EC82F1, 0x4AAFE782, 0x14D89E38, 0x23C83402,
    0x8B48D6BF, 0xC823B89A, 0x68A35626, 0xE89FE121, 0x4BBAA331, 0x20EC16C9,
    0x6ADABE06, 0xD803DA6D, 0xFCC89D41, 0xE57B10E8, 0xCC3FF014, 0x4DB74206,
    0x503FD586, 0x52F68B91, 0x97D69DF3, 0x129C764E, 0x8B2143F7, 0xA36EF3BA,
    0x7C27896C, 0x560F67B5, 0xD70390E6, 0x98B337EA, 0x89568363, 0x2A1681DF,
    0x4B4E928C, 0x41EC3D9C, 0xDFD92EB2, 0xA5D5C85C, 0x2A5088BD, 0x76CB6810,
    0xCB693D21, 0xC0E9EFD5, 0xF992506E, 0x299CE082, 0x901155A6, 0x0B93AA16,
    0x18FEFECE, 0xB0063536, 0x95487089, 0x4BB31BB9, 0x66F3FD97, 0xE32B58A0,
    0x2A39427A, 0x5CD8DE9F, 0x1A8F8616, 0xC5F7D2B2, 0x5AD2BC4E, 0xBF1E86DB,
    0xACF7BFFA, 0xF3589597, 0xA777654C, 0x12DD1364, 0xFFC03A59, 0xDC450527,
    0x33B4C871, 0xBAA2EA33, 0x93A751A6, 0xF9D72E4D, 0x69B50C7F, 0xF74151F9,
    0x7BE8519D, 0xAF6FFAFA, 0x268DBA73, 0xA356128C, 0x0418BE2C, 0x1A43465A,
    0x60C6DF65, 0x0E2438A0, 0xEC25DC05, 0x66AEE4A8, 0xA72A030A, 0xB11FB610,
    0xDD74DAF7, 0x62F6D565, 0x554EAEB7, 0x15F7AE6C, 0x5147F90A, 0xFF0EEC01,
    0x12A9966C, 0x871705B1, 0xE935FF30, 0x46E32957, 0x546D69FC, 0xB8A1BD06,
    0x6A80EA6F, 0x71A29506, 0xEF78AACF, 0x8D52B5ED, 0x9F0A4966, 0x61B3B68E,
    0x4B17AF96, 0x5B282C2E, 0x75582272, 0x16E54299, 0x7D070B9C, 0xAB130157,
    0x76C619D2, 0x5500E2D5, 0x1F980459, 0x5D9C7F83, 0x6A0DDA1D, 0xF6E8B610,
    0x25D0E0A1, 0x242749E0, 0xFEDA4A06, 0x072D69D6, 0x03C7DA79, 0x51AA3355,
    0x6E9FEFF0, 0x0797CBF1, 0xE936C824, 0xC9C1EAF5, 0xD4607E46, 0x88ED7B0E,
    0x92E160AD, 0x731140AD, 0x32FEFCAF, 0x70863B75, 0x3846F110, 0xC4E23DFF,
    0x79D3F753, 0x064648FA, 0x830452F5, 0xB9ED8445
};

/* Parity check matrices corresponding to the above codes.
 * This repesentation mirrors the definition in CCSDS 131.0-B-1,
 * and can be expanded at runtime to create the actual matrix in memory.
 * Each macro represents a single MxM sub-matrix, where M is a function
 * of the information block length and the rate.
 * The HP macro is now used for PI_K which goes via a lookup table.
 * The HZ macro is an MxM zero block and the HI macro an MxM identity, as
 * previously.
 * Each matrix is defined in three parts which are to be added together.
 * Additionally the matrices for the higher rate codes are assumed to be
 * left-prepended to the previous rate's matrix (forming a fatter matrix).
 */
static const uint8_t h_r12[3][3][5] = {
    {
        {HZ   , HZ   , HI   , HZ   , HI   },
        {HI   , HI   , HZ   , HI   , HP| 2},
        {HI   , HP| 5, HZ   , HP| 7, HI   },
    }, {
        {0    , 0    , 0    , 0    , HP| 1},
        {0    , 0    , 0    , 0    , HP| 3},
        {0    , HP| 6, 0    , HP| 8, 0    },
    }, {
        {0    , 0    , 0    , 0    , 0    },
        {0    , 0    , 0    , 0    , HP| 4},
        {0    , 0    , 0    , 0    , 0    },
    }
};

static const uint8_t h_r23[3][3][2] = {
    {
        {HZ   , HZ   },
        {HP| 9, HI   },
        {HI   , HP|12},
    }, {
        {0    , 0    },
        {HP|10, 0    },
        {0    , HP|13},
    }, {
        {0    , 0    },
        {HP|11, 0    },
        {0    , HP|14},
    }
};

static const uint8_t h_r45[3][3][4] = {
    {
        {HZ   , HZ   , HZ   , HZ   },
        {HP|21, HI   , HP|15, HI   },
        {HI   , HP|24, HI   , HP|18},
    }, {
        {0    , 0    , 0    , 0    },
        {HP|22, 0    , HP|16, 0    },
        {0    , HP|25, 0    , HP|19},
    }, {
        {0    , 0    , 0    , 0    },
        {HP|23, 0    , HP|17, 0    },
        {0    , HP|26, 0    , HP|20},
    }
};

/* Theta constants. Looked up against (K-1). */
static const uint8_t theta_k[26] = {3, 0, 1, 2, 2, 3, 0, 1, 0, 1, 2, 0, 2,
                                    3, 0, 1, 2, 0, 1, 2, 0, 1, 2, 1, 2, 3};

/* Phi constants. Looked up against j, log2(m)-7, (K-1).
 * Only the M=(128,256,512) constants are here; for k=4096 and k=16384 codes
 * you'll need to change these to uint16_t and add the extra constants, which
 * can be found in the Python script for easier copy/pasting.
 */
static const uint8_t phi_j_m_k[4][3][26] = {
    {
        {1, 22, 0, 26, 0, 10, 5, 18, 3, 22, 3, 8, 25, 25, 2, 27, 7, 7, 15, 10,
            4, 19, 7, 9, 26, 17},
        {59, 18, 52, 23, 11, 7, 22, 25, 27, 30, 43, 14, 46, 62, 44, 12, 38, 47,
            1, 52, 61, 10, 55, 7, 12, 2},
        {16, 103, 105, 0, 50, 29, 115, 30, 92, 78, 70, 66, 39, 84, 79, 70, 29,
            32, 45, 113, 86, 1, 42, 118, 33, 126},
    }, {
        {0, 27, 30, 28, 7, 1, 8, 20, 26, 24, 4, 12, 23, 15, 15, 22, 31, 3, 29,
            21, 2, 5, 11, 26, 9, 17},
        {0, 32, 21, 36, 30, 29, 44, 29, 39, 14, 22, 15, 48, 55, 39, 11, 1, 50,
            40, 62, 27, 38, 40, 15, 11, 18},
        {0, 53, 74, 45, 47, 0, 59, 102, 25, 3, 88, 65, 62, 68, 91, 70, 115, 31,
            121, 45, 56, 54, 108, 14, 30, 116},
    }, {
        {0, 12, 30, 18, 10, 16, 13, 9, 7, 15, 16, 18, 4, 23, 5, 3, 29, 11, 4,
            8, 2, 11, 11, 3, 15, 13},
        {0, 46, 45, 27, 48, 37, 41, 13, 9, 49, 36, 10, 11, 18, 54, 40, 27, 35,
            25, 46, 24, 33, 18, 37, 35, 21},
        {0, 8, 119, 89, 31, 122, 1, 69, 92, 47, 11, 31, 19, 66, 49, 81, 96, 38,
            83, 42, 58, 24, 25, 92, 38, 120},
    }, {
        {0, 13, 19, 14, 15, 20, 17, 4, 4, 11, 17, 20, 8, 22, 19, 15, 5, 21, 17,
            9, 20, 18, 31, 13, 2, 18},
        {0, 44, 51, 12, 15, 12, 4, 7, 2, 30, 53, 23, 29, 37, 42, 48, 4, 10, 18,
            56, 9, 11, 23, 8, 7, 24},
        {0, 35, 97, 112, 64, 93, 99, 94, 103, 91, 3, 6, 39, 113, 92, 119, 74,
            73, 116, 31, 127, 98, 23, 38, 18, 62},
    },
};


void ldpc_codes_init_paritycheck(enum ldpc_code code, uint32_t* h)
{
    switch(code) {
        case LDPC_CODE_N128_K64:
        case LDPC_CODE_N256_K128:
        case LDPC_CODE_N512_K256:
            init_parity_tc(code, h);
            return;

        case LDPC_CODE_N1280_K1024:
        case LDPC_CODE_N1536_K1024:
        case LDPC_CODE_N2048_K1024:
            init_parity_tm(code, h);
            return;
    }
}

static void init_parity_tc(enum ldpc_code code, uint32_t* h)
{
    int u, v, i, j, n, k, m;
    uint8_t const (* proto)[8];
    get_code_params(code, &n, &k, &m);

    switch(code) {
        case LDPC_CODE_N128_K64:
            proto = h_n128_k64;
            break;

        case LDPC_CODE_N256_K128:
            proto = h_n256_k128;
            break;

        case LDPC_CODE_N512_K256:
            proto = h_n512_k256;
            break;

        default:
            return;
    }

    /* Initialise h to all-zeros so we can just OR bits on later */
    memset(h, 0x00, ((n-k)*n)/8);

    /* For each row of the prototype */
    for(u=0; u<4; u++) {
        /* For each prototype sub-matrix entry in this row */
        for(v=0; v<8; v++) {
            uint8_t subm = proto[u][v];

            /* If we're setting either an identity matrix, or a rotated
             * identity matrix, or the sum of both... */
            if((subm & HP) || (subm & HI)) {
                uint8_t rot = subm & 0x3F;

                /* For each row in the MxM sub-matrix */
                for(i=0; i<m; i++) {
                    /* For each bit in the sub-matrix row */
                    for(j=0; j<m; j++) {
                        /* Compute the uint32_t containing this bit, and the
                         * bit offset */
                        int idx =  (((u * m) + i) * (n/32)) +
                                   ((v * m)/32) +
                                   (j / 32);
                        int shift = 31 - (j % 32);

                        /* Correct for when m<32 and we pack multiple blocks
                         * into each uint32 */
                        if(m<32) {
                            shift -= m * (v % (32/m));
                        }

                        /* Work out the rotated bit and set it if matching */
                        h[idx] |= (j == (i + rot) % m) << shift;

                        /* If HI and HP were both set, do the same with a rot
                         * of 0 (to add on the identity matrix).
                         */
                        if((subm & HP) && (subm & HI)) {
                            h[idx] ^= (j == i % m) << shift;
                        }
                    }
                }
            }
        }
    }
}

/*
 * Initialise a parity check matrix (h) to code (code).
 */
static void init_parity_tm(enum ldpc_code code, uint32_t* h)
{
    int n, k, m;

    get_code_params(code, &n, &k, &m);

    switch(code) {
        case LDPC_CODE_N2048_K1024:
            init_parity_tm_sub(m, 0,   5, 5*m, (uint8_t const *)h_r12, h);
            break;

        case LDPC_CODE_N1536_K1024:
            init_parity_tm_sub(m, 2*m, 5, 7*m, (uint8_t const *)h_r12, h);
            init_parity_tm_sub(m, 0,   2, 7*m, (uint8_t const *)h_r23, h);
            break;

        case LDPC_CODE_N1280_K1024:
            init_parity_tm_sub(m, 6*m, 5, 11*m, (uint8_t const *)h_r12, h);
            init_parity_tm_sub(m, 4*m, 2, 11*m, (uint8_t const *)h_r23, h);
            init_parity_tm_sub(m, 0,   4, 11*m, (uint8_t const *)h_r45, h);
            break;

        default:
            return;
    }
}

/*
 * Fill in a TM parity check matrix, either the H_1/2 or H_2/3 or H_4/5.
 * (m) is the sub-matrix size,
 * (col0) is the column offset to start filling in at (because the higher
 *        rate matrices are specified with the lower rate appended to the end),
 * (dwidth) is the number of sub-matrices in the width of the design matrix,
 * (hcols) is the total number of columns in h (including any before or after
 *         the part of the matrix being filled in),
 * (design) is a pointer to the design matrix, of shape [3][3][dwidth],
 * (h) is a pointer to the parity check matrix to fill in.
 */
static void init_parity_tm_sub(int m, int col0, int dwidth, int hcols,
                               uint8_t const * design, uint32_t* h)
{
    int logm, kk;
    int i, j, u, v, w;

    if(m == 128) {
        logm = 7;
    } else if(m == 256) {
        logm = 8;
    } else if(m == 512) {
        logm = 9;
    } else {
        return;
    }

    /* Initialise to all zeros and we'll just XOR bits on top */
    for(i=0; i<3*m; i++) {
        /* Each row of h is (hcols) bits long,
         * and we skip (i) of these rows, then start at (m*col0) bits after.
         * Each row of our sub matrix is (m*dwidth) bits long,
         * and that's what we'll set to zero.
         * Each bit length is divided by 32 to get the word offset.
         */
        memset(h + i*hcols/32 + col0/32, 0x00, m*dwidth/8);
    }

    /* For each of the three design matrices we add together */
    for(u=0; u<3; u++) {
        /* For each row of the design */
        for(v=0; v<3; v++) {
            /* For each column (in terms of sub-matrices) of that row */
            for(w=0; w<dwidth; w++) {
                uint8_t hh = design[u*3*dwidth + v*dwidth + w];

                /* Don't need to do anything to get zeros */
                if(hh == 0) {
                    continue;
                }

                /* If not zero then at least the identity bit or the
                 * permutation bit should be set, fail here if not */
                if(!(hh & HP) && !(hh & HI)) {
                    return;
                }

                /* For each row in the mxm sub-matrix */
                for(i=0; i<m; i++) {
                    if(hh & HP) {
                        /* This is (k), the subscript for pi_k(i), which we
                         * specify in the design matrix in the lower 5 bits.
                         */
                        kk = hh & 0x3F;

                        /* Compute pi(i) to get the column in this row which
                         * we'll set to 1.
                         */
                        j = m/4 * ((theta_k[kk-1] + ((4*i)/m)) % 4) +
                            (phi_j_m_k[(4*i)/m][logm-7][kk-1] + i) % (m/4);
                    } else {
                        /* For identity matrix, the bit to set is j=i */
                        j = i;
                    }

                    /* Compute the uint32_t holding bit j, and the
                     * shift into it, and then set that bit.
                     */
                    int idx;
                    idx = v*m*hcols/32; /* Skipped sub-matrices above       */
                    idx += i*hcols/32;  /* Skipped rows above i             */
                    idx += col0/32;     /* Skip to column 0                 */
                    idx += w*m/32;      /* Skip w sub-matrices left of us   */
                    idx += j/32;        /* Finally skip to the right column */
                    int shift = 31 - (j % 32);
                    h[idx] ^= 1 << shift;
                }
            }
        }
    }
}

/* n=code length, k=code dimension, m=sub-matrix size */
static void get_code_params(enum ldpc_code code, int* n, int* k, int* m)
{
    switch(code) {
        case LDPC_CODE_N128_K64:
            *n = 128;
            *k = 64;
            *m = *n/8;
            break;

        case LDPC_CODE_N256_K128:
            *n = 256;
            *k = 128;
            *m = *n/8;
            break;

        case LDPC_CODE_N512_K256:
            *n = 512;
            *k = 256;
            *m = *n/8;
            break;

        case LDPC_CODE_N1280_K1024:
            *n = 1280;
            *k = 1024;
            *m = 128;
            break;

        case LDPC_CODE_N1536_K1024:
            *n = 1536;
            *k = 1024;
            *m = 256;
            break;

        case LDPC_CODE_N2048_K1024:
            *n = 2048;
            *k = 1024;
            *m = 512;
            break;

        default:
            return;
    }
}

uint32_t const * ldpc_codes_get_g(enum ldpc_code code, int* n, int* k, int* b)
{
    int m = 0;
    get_code_params(code, n, k, &m);
    switch(code) {
        case LDPC_CODE_N128_K64:
            *b = m;
            return g_n128_k64;

        case LDPC_CODE_N256_K128:
            *b = m;
            return g_n256_k128;

        case LDPC_CODE_N512_K256:
            *b = m;
            return g_n512_k256;

        case LDPC_CODE_N1280_K1024:
            *b = m / 4;
            return g_n1280_k1024;

        case LDPC_CODE_N1536_K1024:
            *b = m / 4;
            return g_n1536_k1024;

        case LDPC_CODE_N2048_K1024:
            *b = m / 4;
            return g_n2048_k1024;

        default:
            return NULL;
    }
}
