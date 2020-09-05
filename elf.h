#include <stdint.h>
#include <stdbool.h>

#ifndef ELF_HEADER
#define ELF_HEADER  "0.0.1"

typedef uint8_t octet;
typedef uint16_t word;
typedef uint32_t dword;
typedef uint64_t qword;

typedef int8_t octet_s;
typedef int16_t word_s;
typedef int32_t dword_s;
typedef int64_t qword_s;

#define 

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
#define ELFHEADER_TYPE_LOOS  0xFE00 // Environment specific use
#define ELFHEADER_TYPE_HIOS  0xFEFF
#define ELFHEADER_TYPE_LOPROC 0xFF00  // processor-psecific use
#define ELFHEADER_TYPE_HIPROC 0xFFFF

#define ELFHEADER_MACHINE_386   3   // Intel 80386
#define ELFHEADER_MACHINE_860   7   // Intel 80860
#define ELFHEADER_MACHINE_AARCH64 183 // ARM AARCH64
#define ELFHEADER_MACHINE_BPF   247 // Linux BPF
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

#endif
