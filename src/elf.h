#ifndef ELF_HEADER
#define ELF_HEADER  "0.0.1"

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>

/*
  this elf interface is aimed at simple operation on elf file.
  Such as Section content extract or symbol address calculate.
  it will focus on the core operation on file format and you should
  use other wrapper interface instead of using this one.
*/

typedef uint8_t  octet;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef int8_t  octetx;
typedef int16_t wordx;
typedef int32_t dwordx;
typedef int64_t qwordx;

struct ELFHeader_32 {
  octet e_magic[16];  // magic info
  word  e_type;       // object file type
  word  e_machine;    // architecture
  dword e_version;    // object file version
  dword e_entry;      // entry point virtual address
  dword e_phoff;      // program header table file offset
  dword e_shoff;      // section header table file offset
  dword e_flags;      // processor-specific flags
  word  e_ehsize;     // elf header size
  word  e_phentsize;  // program header entry size
  word  e_phnum;      // program header entry count
  word  e_shentsize;  // section header entry size
  word  e_shnum;      // section header entry count
  word  e_shstrndx;   // section header string table index
};

struct ELFHeader_64 {
  octet e_magic[16];  // magic info
  word  e_type;       // object file type
  word  e_machine;    // architecture
  dword e_version;    // object file version
  qword e_entry;      // entry point virtual address
  qword e_phoff;      // program header table file offset
  qword e_shoff;      // section header table file offset
  dword e_flags;      // processor-specific flags
  word  e_ehsize;     // elf header size
  word  e_phentsize;  // program header entry size
  word  e_phnum;      // program header entry count
  word  e_shentsize;  // section header entry size
  word  e_shnum;      // section header entry count
  word  e_shstrndx;   // section header string table index
};

// used to determine the class of the elf file
struct ELFHeader_Pre {
  octet e_magic[16];  // magic info
  word  e_type;       // object file type
  word  e_machine;    // architecture
};

enum ELFHeader_Ident {
  ELFMAG0 = 0,
  ELFMAG1,
  ELFMAG2,
  ELFMAG3,
  ELFCLASS,       // file class
  ELFDATA,        // data encoding
  ELFVERSION,     // file version
  ELFOSABI,       // os identification
  ELFABIVERSION,  // ABI version
  ELFPAD,         // start of padding bytes, from here must be 0
  ELFIDENT = 16   // size of ident
};

static char MAGIC[4] = {0x7f, 'E', 'L', 'F'};

/*
 *  ELF 16 octets Identity value
 */
#define ELF_NONE 0

#define ELFCLASS_32 1
#define ELFCLASS_64 2

#define ELFDATA_LSB 1
#define ELFDATA_MSB 2

#define ELFVERSION_CURRENT 1

#define ELFOSABI_SYSV 0
#define ELFOSABI_HPUX 1
#define ELFOSABI_NETBSD 2
#define ELFOSABI_GNU 3
#define ELFOSABI_LINUX 3
#define ELFOSABI_SOLARIS 6
#define ELFOSABI_AIX 7
#define ELFOSABI_FREEBSD 9
#define ELFOSABI_OPENBSD 12
#define ELFOSABI_ARM 97
#define ELFOSABI_STANDALONE 255

// ELF HEADER entry
#define ELFHEADER_TYPE_REL  1 // relocatable file
#define ELFHEADER_TYPE_EXEC 2 // exectuable file
#define ELFHEADER_TYPE_DYN  3 // shared file
#define ELFHEADER_TYPE_CORE 4 // coredump file
#define ELFHEADER_TYPE_NUM  5 // number of defined types
#define ELFHEADER_TYPE_LOOS  0xFE00   // Environment specific use
#define ELFHEADER_TYPE_HIOS  0xFEFF
#define ELFHEADER_TYPE_LOPROC 0xFF00  // processor-psecific use
#define ELFHEADER_TYPE_HIPROC 0xFFFF

#define ELFHEADER_MACHINE_386   3     // Intel 80386
#define ELFHEADER_MACHINE_860   7     // Intel 80860
#define ELFHEADER_MACHINE_AARCH64 183 // ARM AARCH64
#define ELFHEADER_MACHINE_BPF   247   // Linux BPF
/* #define ELFHEADER_MACHINE */

//////////////////////////////////////////////////////////////////
//                Section Header and Entry                      //
//////////////////////////////////////////////////////////////////

typedef struct ELF_SHDR_32 {
  dword sh_name;        // Section name (string tbl index)
  dword sh_type;        // Section type
  dword sh_flags;       // Section flags
  dword sh_addr;        // Section virtual addr at execution
  dword sh_offset;      // Section file offset
  dword sh_size;        // Section size in bytes
  dword sh_link;        // Link to another section
  dword sh_info;        // Additional section information
  dword sh_addralign;   // Section alignment
  dword sh_entsize;     // Entry size if section holds table
} Shdr_32;

typedef struct ELF_SHDR_64 {
  dword sh_name;        // Section name (string tbl index)
  dword sh_type;        // Section type
  qword sh_flags;       // Section flags
  qword sh_addr;        // Section virtual addr at execution
  qword sh_offset;      // Section file offset
  qword sh_size;        // Section size in bytes
  dword sh_link;        // Link to another section
  dword sh_info;        // Additional section information
  qword sh_addralign;   // Section alignment
  qword sh_entsize;     // Entry size if section holds table
} Shdr_64;

// Special Section Indexes and it may be used in other places
#define SHDR_INDEX_UNDEF     0x00   // mean undefined section
#define SHDR_INDEX_LORESERVE 0xff00
#define SHDR_INDEX_LOPROC    0xff00
#define SHDR_INDEX_HIPROC    0xff1f
#define SHDR_INDEX_ABS       0xfff1
#define SHDR_INDEX_COMMON    0xfff2
#define SHDR_INDEX_HIRESERVE 0xffff

// Section types
// TODO: add description
enum SHDR_TYPE {
  SHTYPE_NULL = 0,
  SHTYPE_PROGBITS,
  SHTYPE_SYMTAB,
  SHTYPE_STRTAB,
  SHTYPE_RELA,
  SHTYPE_HASH,
  SHTYPE_DYNAMIC,
  SHTYPE_NOTE,
  SHTYPE_NOBITS,
  SHTYPE_REL,
  SHTYPE_SHLIB,
  SHTYPE_DYNSYM,
  SHTYPE_LOPROC = 0x70000000,
  SHTYPE_HIPROC = 0x7fffffff,
  SHTYPE_KIYSER = 0x80000000,
  SHTYPE_HIUSER = 0xffffffff
};

// Section Flags
enum SHDR_FLAG {
  SHF_WRITE = 0x01,
  SHF_ALLOC = 0x02,
  SHF_EXECINSTR = 0x04,
  SHF_MASKPROC = 0xf0000000
};

//TODO:: Complete this type def;
//////////////////////////////////////////////////////////////////
//                Program Header and Entry                      //
//////////////////////////////////////////////////////////////////
typedef struct ELF_PHDR_32 {
  dword p_type;
  dword p_offset;
  dword p_vaddr;
  dword p_paddr;
  dword p_memsz;
  dword p_flags;
  dword p_align;
} Phdr_32;

typedef struct ELF_PHDR_64 {

} Phdr_64;


//////////////////////////////////////////////////////////////////
//                 Symbol Header and Entry                      //
//////////////////////////////////////////////////////////////////
typedef struct ELF_SYM_32 {
  dword st_name;    // index to symbol string table
  dword st_value;   // value of symbol, its meaning depends
                    // on the context
  dword st_size;    // size of the symbol
  octet st_info;    // symbol type and its binding attributes.
  octet st_other;   // no meaning, hold 0.
  word  st_shndx;   // relevant section index.
} Sym_32;

typedef struct ELF_SYM_64 {
  dword st_name;    // Symbol name
  octet st_info;    // Type and Binding attributes
  octet st_other;   // Reserved, hold 0.
  word  st_shndx;   // Section table index
  qword st_value;   // Symbol value
  qword st_size;    // Size of object  (e.g. common)
} Sym_64;

// pass in a single byte, and return the relevant field.
#define SYM_BIND_FIELD(v) (((v)>>4)&0x0F)
#define SYM_TYPE_FIELD(v) ((v)&0x0F)

// symbol binding type
enum ELFSYM_BIND {
  STB_LOCAL   = 0,
  STB_GLOBAL  = 1,
  STB_WEAK    = 2,
  STB_LOPROC  = 13,
  STB_HIPROC  = 15
};

// symbol type
enum ELFSYM_TYPE {
  STT_NOTYPE = 0,
  STT_OBJECT,
  STT_FUNC,
  STT_SECTION,
  STT_FILE,
  STT_COMMON,
  STT_TLS,
  STT_NUM = 7,
  STT_LOOS = 10,
  STT_HIOS = 12,
  STT_LOPROC = 13,
  STT_HIPROC = 15
};

//////////////////////////////////////////////////////////////////
//                      RELOCATION ENTRY                        //
//////////////////////////////////////////////////////////////////

typedef struct ELF_REL_32 {
  dword r_offset;
  dword r_info;
} Rel_32;

typedef struct ELF_REL_64 {
  dword r_offset;
  dword r_info;
} Rel_64;

typedef struct ELF_RELA_32 {
  dword   r_offset;
  dword   r_info;
  dwordx  r_addend;
} Rela_32;

typedef struct EFL_RELA_64 {
  dword   r_offset;
  dword   r_info;
  dwordx  r_addend;
} Rela_64;

//////////////////////////////////////////////////////////////////
//                 Operation on ELF File                        //
//////////////////////////////////////////////////////////////////

/*
  All interface will return -1 to indicate invalid format;
  And return 0 to indicate invalid field value;
*/

bool isELFormat(void* memory);
int getABIClass(struct ELFHeader_Pre* hd);
#define classchk(i) ((i)==32?true:(i)==64?true:false)

char* const get_filetype_str(void* mem);
char* const get_os_str(void *mem);

// elf file header
union unihdr {
  void *all;
  struct ELFHeader_32* hdr_32;
  struct ELFHeader_64* hdr_64;
};

// section header
union unisechdr {
  void *all;
  Shdr_32 *shdr_32;
  Shdr_64 *shdr_64;
};

// segment header
union uniseghdr {
  void *all;
};

// symbol header
union unisymhdr {
  void *all;
  struct ELF_SYM_32 *sym_32;
  struct ELF_SYM_64 *sym_64;
};

///////////////////////////////////////////////
// section manipulation functions and macros //
///////////////////////////////////////////////

// structure holding all information
// about elf sections
struct sectbl {
  int class;
  int num;
  size_t name;
  struct section {
    void *hdr;
    void *sec;
  } *ents;
};

// build the section infomation, mem can be freed after this call.
struct sectbl build_sectbl(void* mem);
// free this structure.
void free_sectbl(struct sectbl *tb);
// query one section entry's name
char const* sectbl_getname(struct sectbl const * tb, size_t idx);
// search the name of one section and return its index.
size_t sectbl_search(struct sectbl const * tb, char* name);

///////////////////////////////////////////////
// Elf File Symbol info operation functions  //
///////////////////////////////////////////////

const char* get_symbind(octet);
const char* get_symtype(octet);

#endif
