/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */
static const char sccsid[] = "%Z% %W% %I% %E% %U%";
/*********************************************************************/
/*                                                                   */
/* Module Name: coreutil.c                                           */
/*                                                                   */
/* Description: Utilities to use when parsing core files             */
/*                                                                   */
/*********************************************************************/
/* Following text will be included in the Service Reference Manual.  */
/* Ensure that the content is correct and up-to-date.                */
/* All updates must be made in mixed case.                           */
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Utilities to use when parsing core files.                         */
/*                                                                   */
/* End of text to be included in SRM                                 */
/*********************************************************************/

/*********************************************************************/
/*                                                                   */
/* Includes                                                          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#if defined(AMQ_PC)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <amqxsbhz.h>            /* Copyright information            */
#define xcsINCL_INITTERM
#define xcsINCL_QUERY_SYSTEM
#define xcsINCL_TRANSLATION
#include <amqxcsha.h>
#include <amqxugh0.h>            /* Common Services xugGetOpt        */
#include <amqcccha.h>

#include <fmtexternal.h>
#include <fmtstructs.h>
#include <fmtformat.h>


/*********************************************************************/
/*                                                                   */
/* Typedef's                                                         */
/*                                                                   */
/*********************************************************************/

typedef enum
{
  CORE_OK=0,
  CORE_WARNING=10,
  CORE_ERROR=20
} CORE_RETURN_CODES;

typedef enum
{
  CORE_MODE_NONE=0,
  CORE_MODE_MEMORY,
  CORE_MODE_STRUCTURES,
  CORE_MODE_INFO,
  CORE_MODE_LAST
} CORE_MODE;

typedef enum
{
  CORE_PLATFORM_UNKNOWN=0,
  CORE_PLATFORM_LINUX_X86,
  CORE_PLATFORM_LINUX_X86_64,
  CORE_PLATFORM_LINUX_PPC,
  CORE_PLATFORM_AIX,
  CORE_PLATFORM_SOLARIS_SPARC,
  CORE_PLATFORM_SOLARIS_X86_64,
  CORE_PLATFORM_HP,
} CORE_PLATFORM;

typedef enum
{
  CORE_ADDRESSING_MODE_UNKNOWN=0,
  CORE_ADDRESSING_MODE_32=32,
  CORE_ADDRESSING_MODE_64=64
} CORE_ADDRESSING_MODE;

typedef enum
{
  CORE_STATE_UNKNOWN=0,
  CORE_STATE_ALLOCATED,
  CORE_STATE_CACHED
} CORE_STATE;

typedef struct _CORE_POINTER CORE_POINTER;
struct _CORE_POINTER
{
  UINT32        Offset;
  CORE_STATE    State;
  ULONG64       Size;
  PMQCHAR       Data;
  CORE_POINTER* pNext;
};

typedef struct _CORE_STORAGE
{
  UINT32        Count;
  ULONG64       Size;
} CORE_STORAGE;

typedef struct _CORE_FUNCTION CORE_FUNCTION;
struct _CORE_FUNCTION
{
  PSZ            Name;
  USHORT         Component;
  USHORT         Function;
  CORE_STORAGE   Allocated;
  CORE_STORAGE   Cached;
  CORE_POINTER*  pPtr;
};

typedef struct _CORE_MEMORY_ANCHOR
{
  CORE_FUNCTION*  StorageTable;
  CORE_FUNCTION** FunctionTable;
  CORE_STORAGE    AllocatedStorage;
  CORE_STORAGE    CachedStorage;
  CORE_STORAGE    TotalStorage;
  UINT32          MaxNameLength;
  UINT32          InUse;
} CORE_MEMORY_ANCHOR;

typedef struct _CORE_STRUCTURE CORE_STRUCTURE;
struct _CORE_STRUCTURE
{
  xcsEYECATCHER   EyeCatcher;
  UINT32          Count;
  CORE_POINTER*   Blocks;
  CORE_STRUCTURE* pNext;
};

typedef struct _CORE_STRUCTURE_ANCHOR
{
  UINT32          Count;
  CORE_STRUCTURE* StrucList;
  UINT32          HighestOffset;
} CORE_STRUCTURE_ANCHOR;

typedef enum _CORE_FORMAT
{
  CORE_FORMAT_UNKNOWN=0,
  CORE_FORMAT_ELF,
  CORE_FORMAT_XCOFF
} CORE_FORMAT;

typedef union _CORE_VAR
{
  MQUINT64 l;
  MQUINT32 s;
} CORE_VAR;

typedef enum _CORE_MEM_TYPE
{
  CORE_MEM_TYPE_UNKNOWN=0,
  CORE_MEM_TYPE_PROGRAM_HEADER,
  CORE_MEM_TYPE_SECTION_HEADER
} CORE_MEM_TYPE;

typedef struct _CORE_MEMORY_MAP CORE_MEMORY_MAP;
struct _CORE_MEMORY_MAP
{
  CORE_MEM_TYPE    Type;
  MQUINT16         Number;
  MQUINT64         Size;
  CORE_VAR         Virtual;
  MQUINT64         Offset;
  CORE_MEMORY_MAP* pNext;
};

typedef struct _CORE_INFO
{
  char*                filename;
  char*                platform_string;
  CORE_PLATFORM        platform;
  CORE_ADDRESSING_MODE bits;
  char*                vrmf;
  unsigned int         filesize;
  char*                filebuffer;
  CORE_FORMAT          format;
  MQBOOL               byteswap;
  CORE_MEMORY_MAP*     offset_map;
  CORE_MEMORY_MAP*     address_map;
} CORE_INFO;

typedef union _CORE_DATA_UNION
{
  char     array[8];
  MQUINT64 number;
} CORE_DATA_UNION;


/*********************************************************************/
/* ELF related structures and types.                                 */
/*********************************************************************/

typedef MQUINT32 Elf32_Addr;
typedef MQUINT16 Elf32_Half;
typedef MQUINT32 Elf32_Off;
typedef MQINT32  Elf32_Sword;
typedef MQUINT32 Elf32_Word;
typedef MQUINT64 Elf32_Lword;

typedef MQUINT64 Elf64_Addr;
typedef MQUINT16 Elf64_Half;
typedef MQUINT64 Elf64_Off;
typedef MQINT32  Elf64_Sword;
typedef MQUINT32 Elf64_Word;
typedef MQINT64  Elf64_Sxword;
typedef MQUINT64 Elf64_Xword;
typedef MQUINT64 Elf64_Lword;

typedef struct _Elf32_Ehdr   /* 32-bit ELF main header */
{
  unsigned char e_ident[16]; /* ident bytes */
  Elf32_Half    e_type;      /* file type */
  Elf32_Half    e_machine;   /* target machine */
  Elf32_Word    e_version;   /* file version */
  Elf32_Addr    e_entry;     /* start address */
  Elf32_Off     e_phoff;     /* phdr file offset */
  Elf32_Off     e_shoff;     /* shdr file offset */
  Elf32_Word    e_flags;     /* file flags */
  Elf32_Half    e_ehsize;    /* sizeof ehdr */
  Elf32_Half    e_phentsize; /* sizeof phdr */
  Elf32_Half    e_phnum;     /* number phdrs */
  Elf32_Half    e_shentsize; /* sizeof shdr */
  Elf32_Half    e_shnum;     /* number shdrs */
  Elf32_Half    e_shstrndx;  /* shdr string index */
} Elf32_Ehdr;

typedef struct _Elf64_Ehdr   /* 64-bit ELF main header */
{
  unsigned char e_ident[16]; /* ident bytes */
  Elf64_Half    e_type;      /* file type */
  Elf64_Half    e_machine;   /* target machine */
  Elf64_Word    e_version;   /* file version */
  Elf64_Addr    e_entry;     /* start address */
  Elf64_Off     e_phoff;     /* phdr file offset */
  Elf64_Off     e_shoff;     /* shdr file offset */
  Elf64_Word    e_flags;     /* file flags */
  Elf64_Half    e_ehsize;    /* sizeof ehdr */
  Elf64_Half    e_phentsize; /* sizeof phdr */
  Elf64_Half    e_phnum;     /* number phdrs */
  Elf64_Half    e_shentsize; /* sizeof shdr */
  Elf64_Half    e_shnum;     /* number shdrs */
  Elf64_Half    e_shstrndx;  /* shdr string index */
} Elf64_Ehdr;

typedef struct _Elf32_Phdr   /* 32-bit ELF program header */
{
  Elf32_Word    p_type;      /* entry type */
  Elf32_Off     p_offset;    /* file offset */
  Elf32_Addr    p_vaddr;     /* virtual address */
  Elf32_Addr    p_paddr;     /* physical address */
  Elf32_Word    p_filesz;    /* file size */
  Elf32_Word    p_memsz;     /* memory size */
  Elf32_Word    p_flags;     /* entry flags */
  Elf32_Word    p_align;     /* memory/file alignment */
} Elf32_Phdr;

typedef struct _Elf64_Phdr   /* 64-bit ELF program header */
{
  Elf64_Word    p_type;      /* entry type */
  Elf64_Word    p_flags;     /* entry flags */
  Elf64_Off     p_offset;    /* file offset */
  Elf64_Addr    p_vaddr;     /* virtual address */
  Elf64_Addr    p_paddr;     /* physical address */
  Elf64_Xword   p_filesz;    /* file size */
  Elf64_Xword   p_memsz;     /* memory size */
  Elf64_Xword   p_align;     /* memory/file alignment */
} Elf64_Phdr;

typedef struct _Elf32_Shdr
{
  Elf32_Word    sh_name;     /* section name */
  Elf32_Word    sh_type;     /* SHT_... */
  Elf32_Word    sh_flags;    /* SHF_... */
  Elf32_Addr    sh_addr;     /* virtual address */
  Elf32_Off     sh_offset;   /* file offset */
  Elf32_Word    sh_size;     /* section size */
  Elf32_Word    sh_link;     /* misc info */
  Elf32_Word    sh_info;     /* misc info */
  Elf32_Word    sh_addralign;/* memory alignment */
  Elf32_Word    sh_entsize;  /* entry size if table */
} Elf32_Shdr;

typedef struct _Elf64_Shdr
{
  Elf64_Word    sh_name;     /* section name */
  Elf64_Word    sh_type;     /* SHT_... */
  Elf64_Xword   sh_flags;    /* SHF_... */
  Elf64_Addr    sh_addr;     /* virtual address */
  Elf64_Off     sh_offset;   /* file offset */
  Elf64_Xword   sh_size;     /* section size */
  Elf64_Word    sh_link;     /* misc info */
  Elf64_Word    sh_info;     /* misc info */
  Elf64_Xword   sh_addralign;/* memory alignment */
  Elf64_Xword   sh_entsize;  /* entry size if table */
} Elf64_Shdr;


/*********************************************************************/
/*                                                                   */
/* Macros                                                            */
/*                                                                   */
/*********************************************************************/

#define USAGE "Usage: coreutil -c filename [-p platform] [-v vrmf] [-x 32|64]\n"         \
              "  -c  Core file name to analyze\n"                                        \
              "  -p  Platform core file produced on\n"                                   \
              "        linux_x86\n"                                                      \
              "        linux_x86-64    ** default **\n"                                  \
              "        linux_power\n"                                                    \
              "        aix_power\n"                                                      \
              "        solaris_sparc\n"                                                  \
              "        solaris_x86-64\n"                                                 \
              "        hp_itanium\n"                                                     \
              "  -v  MQ VRMF that produced core file\n"                                  \
              "  -x  Addressing mode of application producing the core file\n"

#if defined(AMQ_PC)
#define open       _open
#define O_BINARY   _O_BINARY
#define O_CREAT    _O_CREAT
#define O_WRONLY   _O_WRONLY
#define O_APPEND   _O_APPEND
#define O_EXCL     _O_EXCL
#define fstat      _fstat
#define stat       _stat
#define close      _close
#define read       _read
#define creat      _creat
#define write      _write
#define unlink     _unlink
#define vsnprintf  _vsnprintf
#define PROT_READ  0
#define PROT_WRITE 0
#else
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif

/*********************************************************************/
/* Extremely basic mmap support for Windows!                         */
/*********************************************************************/
#if defined(AMQ_PC)
#define mmap(_start, _length, _prot, _flags, _fd, _offset) \
        readfile(_length, _fd)
#endif

#define xstEYE_FREED        "?SPG"
#define xstLENGTH_OFFSET_32 12
#define xstLENGTH_OFFSET_64 16
#define xstDATA_OFFSET_32   16
#define xstDATA_OFFSET_64   24

#ifndef xcsMAX_FUNCS_PER_COMPONENT
#define xcsMAX_FUNCS_PER_COMPONENT  USHRT_MAX
#endif

#define RecordStorageUse(_storage, _length)                           \
{                                                                     \
  (_storage).Count++;                                                 \
  (_storage).Size += (_length);                                       \
}

#define BLANK_STRING "                                        "
#define DASH_STRING  "--------------------------------------------------------------------------------------------------------------------------------------------"

#define QUIT_OR_BLANK(_c) (((_c) == 'Q') || ((_c) == 'q') || ((_c) == '\n'))
#define BACK_OR_BLANK(_c) (((_c) == 'B') || ((_c) == 'b') || ((_c) == '\n'))
#define BACK(_c)          (((_c) == 'B') || ((_c) == 'b'))
#define SEARCH(_c)        (((_c) == 'S') || ((_c) == 's'))
#define HEX(_c)           (((_c) == 'H') || ((_c) == 'h'))

#define INFO_DEFAULT NULL,\
                     "linux_x86-64",\
                     CORE_PLATFORM_LINUX_X86_64,\
                     CORE_ADDRESSING_MODE_UNKNOWN,\
                     "99.9.9.9",\
                     0,\
                     NULL,\
                     CORE_FORMAT_UNKNOWN,\
                     TRUE,\
                     NULL


/*********************************************************************/
/*                                                                   */
/* Global variables                                                  */
/*                                                                   */
/*********************************************************************/

extern FILE*                 outfd;
extern FILE*                 errfd;
static CORE_INFO             info            = {INFO_DEFAULT};
static CORE_MEMORY_ANCHOR    manchor         = {0};
static CORE_STRUCTURE_ANCHOR sanchor         = {0};


/*********************************************************************/
/*                                                                   */
/* Function definitions                                              */
/*                                                                   */
/*********************************************************************/

/*********************************************************************/
/* Print a buffer in text and in hex.                                */
/*********************************************************************/
void printfhex(PMQCHAR DataBuffer, ULONG64 DataLength)
{
  size_t i  = 0, k = 0, z = 0;
  char   AsciiBuffer[20] = "";
  char   LineBuffer[256] = "";
  char * temp  = AsciiBuffer;
  char * start = DataBuffer;
  char * p     = LineBuffer;

  for (i = 0; i < DataLength; i++, k++, start++, temp++)
  {
    if (k == 16)
    {
      k = 0;
      *temp = '\0';
      p += sprintf(p, "   '%s'\n", AsciiBuffer);
      printf("%s", LineBuffer);
      p = LineBuffer;
    }

    if (k == 0)
    {
      p += sprintf(p, "      ");
      temp = AsciiBuffer;                         /* reset ascii output pointer */
    }

    p += sprintf(p, "%s%02.2x", (k && !(k % 4)) ? " " : "", (unsigned char)*start);

    *temp = (unsigned char) (isprint((unsigned char)*start) ? *start : '.');
  }

  *temp = '\0';
  z = 37 - 2 * k   -   (k+3) / 4;
  p += sprintf(p, "  %*s'%s'\n", z, " ", AsciiBuffer);
  printf("%s", LineBuffer);

  return;
}


/*********************************************************************/
/* Read a file fully into a buffer.                                  */
/*********************************************************************/
static void* readfile(size_t length, int fd)
{
  char*        buffer     = NULL;
  unsigned int count      = (unsigned int)length;
  int          bytes_read = 0;
  int          savederrno;

  buffer = (char*)malloc(count);
  savederrno = errno;
  if(buffer == NULL)
  {
    printf("ERROR: malloc for %u bytes failed: errno %d\n",
           count, savederrno);
    goto exit;
  }

  bytes_read = read(fd, buffer, count);
  savederrno = errno;
  if(bytes_read == -1)
  {
    printf("ERROR: read failed: errno %d\n", savederrno);
    free(buffer);
    buffer = NULL;
    goto exit;
  }

  if((unsigned int)bytes_read != count)
  {
    printf("ERROR: read only read %d bytes of %u: errno %d\n", bytes_read, count, savederrno);
    free(buffer);
    buffer = NULL;
    goto exit;
  }

exit:
  return buffer;
}


#if defined(AMQ_PC)
/*********************************************************************/
/* munmap support to undo the basic mmap support above.              */
/*********************************************************************/
int munmap(void* start, size_t length)
{
  if(start != NULL)
  {
    free(start);
  }

  return 0;
}
#endif


/*********************************************************************/
/* Load a core file and map its contents into memory.                */
/*********************************************************************/
int load_core( char*         filename
             , int*          pfd
             , unsigned int* pfilesize
             , char**        pfilebuffer )
{
  int rc = CORE_OK;
  int savederrno;
  struct stat filestatbuf;
  unsigned int filesize = 0;
  char* filebuffer = NULL;

  /*******************************************************************/
  /* Open the core file.                                             */
  /*******************************************************************/
  *pfd = open(filename, O_RDONLY | O_BINARY);
  savederrno = errno;
  if(*pfd == -1)
  {
    printf("ERROR: Unable to open file '%s': errno %d\n", filename, savederrno);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Fstat the file so we can get its size.                          */
  /*******************************************************************/
  if(fstat(*pfd, &filestatbuf) == -1)
  {
    savederrno = errno;
    printf("ERROR: Unable to determine size for file '%s': errno %d\n", filename, savederrno);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Check for a zero file size.                                     */
  /*******************************************************************/
  if(filestatbuf.st_size == 0)
  {
    printf("ERROR: Zero sized file '%s' is not a binary file\n", filename);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Check for a file larger than this program will tolerate.        */
  /*******************************************************************/
  if(filestatbuf.st_size > UINT_MAX)
  {
    printf("ERROR: File '%s' too large\n", filename);
    rc = CORE_ERROR;
    goto exit;
  }
  filesize = (unsigned int)filestatbuf.st_size;

  /*******************************************************************/
  /* Map the entire file into memory.                                */
  /*******************************************************************/
  filebuffer = (char*)mmap(NULL, filesize, PROT_READ, MAP_SHARED, *pfd, 0);
  savederrno = errno;
#if defined(AMQ_PC)
  if(filebuffer == NULL)
  {
    /*****************************************************************/
    /* It is assumed that the Windows mmap implementation defined in */
    /* this file has already printed a debug message.                */
    /*****************************************************************/
    rc = CORE_ERROR;
    goto exit;
  }
#else
  if(filebuffer == MAP_FAILED)
  {
    printf("ERROR: mmap of file '%s' failed: errno %d\n", filename, savederrno);
    rc = CORE_ERROR;
    goto exit;
  }
#endif

exit:
  /*******************************************************************/
  /* Set the filebuffer return value.                                */
  /*******************************************************************/
  if(filebuffer)
  {
    *pfilesize   = filesize;
    *pfilebuffer = filebuffer;
  }
  else
  {
    *pfilesize   = 0;
    *pfilebuffer = NULL;
  }

  return rc;
}


/*********************************************************************/
/* Unload a core file from memory and close the file.                */
/*********************************************************************/
int unload_core( char*         filename
               , int*          pfd
               , unsigned int* pfilesize
               , char**        pfilebuffer )
{
  int rc = CORE_OK;

  /*******************************************************************/
  /* Check a valid file descriptor was passed in.                    */
  /*******************************************************************/
  if(*pfd != -1)
  {
    /*****************************************************************/
    /* If a file buffer was passed in, unmap it.                     */
    /*****************************************************************/
    if(*pfilebuffer)
    {
      munmap(*pfilebuffer, *pfilesize);
      *pfilesize   = 0;
      *pfilebuffer = NULL;
    }

    /*****************************************************************/
    /* Now close the file.                                           */
    /*****************************************************************/
    close(*pfd);
    *pfd = -1;
  }

  return rc;
}


/*********************************************************************/
/* Display core file info.                                           */
/*********************************************************************/
int display_info()
{
  int rc = CORE_OK;
  CORE_MEMORY_MAP* pMap;
  UINT32 i;

  printf("CORE_INFO\n");
  printf("{\n");
  printf("  filename:         %s\n", info.filename);
  printf("  platform_string:  %s\n", info.platform_string);
  printf("  platform:         %d\n", info.platform);
  printf("  bits:             %d\n", info.bits);
  printf("  vrmf:             %s\n", info.vrmf);
  printf("  filesize:         %u\n", info.filesize);
  printf("  filebuffer:       0x%p\n", info.filebuffer);
  printf("  format:           %d\n", info.format);
  printf("  byteswap:         %d\n", info.byteswap);
  printf("}\n");

  if(info.format == CORE_FORMAT_ELF)
  {
    printf("ELF\n");
    printf("{\n");
    if(info.bits == CORE_ADDRESSING_MODE_32)
    {
      Elf32_Ehdr* pHdr = (Elf32_Ehdr*)info.filebuffer;
      printf("  e_ident[16]:");
      printfhex(pHdr->e_ident, 16);
      printf("  e_type:           %hu\n", pHdr->e_type);
      printf("  e_machine:        %hu\n", pHdr->e_machine);
      printf("  e_version:        %u\n", pHdr->e_version);
      printf("  e_entry:          %u\n", pHdr->e_entry);
      printf("  e_phoff:          %u\n", pHdr->e_phoff);
      printf("  e_shoff:          %u\n", pHdr->e_shoff);
      printf("  e_flags:          %u\n", pHdr->e_flags);
      printf("  e_ehsize:         %hu\n", pHdr->e_ehsize);
      printf("  e_phentsize:      %hu\n", pHdr->e_phentsize);
      printf("  e_phnum:          %hu\n", pHdr->e_phnum);
      printf("  e_shentsize:      %hu\n", pHdr->e_shentsize);
      printf("  e_shnum:          %hu\n", pHdr->e_shnum);
      printf("  e_shstrndx:       %hu\n", pHdr->e_shstrndx);
    }
    else
    {
      Elf64_Ehdr* pHdr = (Elf64_Ehdr*)info.filebuffer;
      printf("  e_ident[16]:");
      printfhex(pHdr->e_ident, 16);
      printf("  e_type:           %hu\n", pHdr->e_type);
      printf("  e_machine:        %hu\n", pHdr->e_machine);
      printf("  e_version:        %u\n", pHdr->e_version);
      printf("  e_entry:          %"Int64"u\n", pHdr->e_entry);
      printf("  e_phoff:          %"Int64"u\n", pHdr->e_phoff);
      printf("  e_shoff:          %"Int64"u\n", pHdr->e_shoff);
      printf("  e_flags:          %u\n", pHdr->e_flags);
      printf("  e_ehsize:         %hu\n", pHdr->e_ehsize);
      printf("  e_phentsize:      %hu\n", pHdr->e_phentsize);
      printf("  e_phnum:          %hu\n", pHdr->e_phnum);
      printf("  e_shentsize:      %hu\n", pHdr->e_shentsize);
      printf("  e_shnum:          %hu\n", pHdr->e_shnum);
      printf("  e_shstrndx:       %hu\n", pHdr->e_shstrndx);
    }
    printf("}\n");
  }

  printf("ADDRESSES\n");
  printf("{\n");
  for(i = 0, pMap = info.offset_map; pMap; pMap = pMap->pNext, i++)
  {
    if(info.bits == CORE_ADDRESSING_MODE_32)
      printf("  Virtual: 0x%8X   Offset: %"Int64"u\n",
             pMap->Virtual.s, pMap->Offset);
    else
      printf("  %c:%5hu Virtual: 0x%16"Int64"X Offset: %20"Int64"u Size: %"Int64"u\n",
             (pMap->Type == CORE_MEM_TYPE_PROGRAM_HEADER) ? 'P' : ((pMap->Type == CORE_MEM_TYPE_SECTION_HEADER) ? 'S' : 'U'),
             pMap->Number,
             pMap->Virtual.l, pMap->Offset,
             pMap->Size);
  }
  printf("  Total: %u\n", i);
  printf("}\n");

  return rc;
}


/*********************************************************************/
/* Structure formatter callback routine.                             */
/*********************************************************************/
int SBCallbackFunc(char *str)
{
  if(str != NULL)
  {
    printf("%s\n", str);
  }
  return OK;
}


/*********************************************************************/
/* Calculate the address of a structure given its offset in the file */
/*********************************************************************/
int calculate_address(MQUINT64 Offset, CORE_VAR* Address)
{
  int rc = CORE_OK;
  CORE_MEMORY_MAP* pNextMap;
  CORE_MEMORY_MAP* pPreviousMap;

  printf("Calculating address\n");

  for( pNextMap = info.offset_map, pPreviousMap = NULL_POINTER
     ; pNextMap
     ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
  {
    if(pNextMap->Offset > Offset)
      break;
  }

  if(pPreviousMap == NULL)
  {
    rc = CORE_ERROR;
    Address->l = 0;
    goto exit;
  }
  else if(info.bits == CORE_ADDRESSING_MODE_32)
  {
    printf("Structure offset: %"Int64"u\n", Offset);
    printf("Closest matching header has Offset: %"Int64"u  Virtual: 0x%8X\n",
           pPreviousMap->Offset, pPreviousMap->Virtual.s);
    Address->s = pPreviousMap->Virtual.s + (MQUINT32)(Offset - pPreviousMap->Offset);
    printf("Virtual address of this offset is: 0x%8X\n", Address->s);
  }
  else
  {
    printf("Structure offset: %"Int64"u\n", Offset);
    printf("Closest matching header has %c:%5u Offset: %20"Int64"u Virtual: 0x%016"Int64"X Size:%"Int64"u\n",
           (pPreviousMap->Type == CORE_MEM_TYPE_PROGRAM_HEADER) ? 'P' : ((pPreviousMap->Type == CORE_MEM_TYPE_SECTION_HEADER) ? 'S' : 'U'),
           pPreviousMap->Number,
           pPreviousMap->Offset, pPreviousMap->Virtual.l,
           pPreviousMap->Size);
    Address->l = pPreviousMap->Virtual.l + (Offset - pPreviousMap->Offset);
    printf("Virtual address of this offset is: 0x%016"Int64"X\n", Address->l);
  }

exit:
  return rc;
}


/*********************************************************************/
/* Calculate the offset of a structure given its address in the file */
/*********************************************************************/
MQUINT64 calculate_offset(MQUINT64 Address)
{
  MQUINT64 Offset = 0;
  CORE_MEMORY_MAP* pNextMap;
  CORE_MEMORY_MAP* pPreviousMap;

  printf("Calculating offset\n");

  for( pNextMap = info.address_map, pPreviousMap = NULL_POINTER
     ; pNextMap
     ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
  {
    if(pNextMap->Virtual.l > Address)
      break;
  }

  if(pPreviousMap == NULL)
  {
    goto exit;
  }
  else if(info.bits == CORE_ADDRESSING_MODE_32)
  {
    printf("Structure address: %"Int64"u\n", Offset);
    printf("Closest matching header has Offset: %"Int64"u  Virtual: 0x%8X\n",
           pPreviousMap->Offset, pPreviousMap->Virtual.s);
    Offset = pPreviousMap->Offset + (MQUINT32)(Address - pPreviousMap->Virtual.s);
    printf("Offset of this virtual address is: %"Int64"u\n", Offset);
  }
  else
  {
    printf("Structure address: %"Int64"u\n", Address);
    printf("Closest matching header has %c:%5u Offset: %20"Int64"u Virtual: 0x%016"Int64"X Size:%"Int64"u\n",
           (pPreviousMap->Type == CORE_MEM_TYPE_PROGRAM_HEADER) ? 'P' : ((pPreviousMap->Type == CORE_MEM_TYPE_SECTION_HEADER) ? 'S' : 'U'),
           pPreviousMap->Number,
           pPreviousMap->Offset, pPreviousMap->Virtual.l,
           pPreviousMap->Size);
    Offset = pPreviousMap->Offset + (Address - pPreviousMap->Virtual.l);
    printf("Offset of this virtual address is: %"Int64"u\n", Offset);
  }

exit:
  return Offset;
}


/*********************************************************************/
/* Search for a reference to a block in the core file.               */
/*********************************************************************/
int search_for_block(CORE_POINTER* pPtr)
{
  MQUINT64        Offset = pPtr->Offset;
  unsigned int    i;
  CORE_DATA_UNION unswapped, swapped;
  CORE_VAR        address;
  CORE_STRUCTURE* pStruc;
  CORE_POINTER*   pBlock;
  CORE_POINTER*   pPreviousBlock;
  CORE_POINTER*   pNearestBlock;
  UINT32          Difference = (UINT32)-1;

  if(!xcsCheckEyeCatcher(pPtr->Data, cccSTORAGE_EYECATCHER))
  {
    Offset += sizeof(cccSTORAGE);
  }
  calculate_address(Offset, &address);

  unswapped.number = address.l;
  swapped.array[0] = unswapped.array[7];
  swapped.array[1] = unswapped.array[6];
  swapped.array[2] = unswapped.array[5];
  swapped.array[3] = unswapped.array[4];
  swapped.array[4] = unswapped.array[3];
  swapped.array[5] = unswapped.array[2];
  swapped.array[6] = unswapped.array[1];
  swapped.array[7] = unswapped.array[0];

  for(i = 0; i < sanchor.HighestOffset; i += 8)
  {
    if(!memcmp(info.filebuffer+i, &unswapped.number, 8))
    {
      printf("Found match for 0x%016"Int64"X at offset %d: ", unswapped.number, i);
      printfhex((info.filebuffer+i) - 512, 520);
      for(pNearestBlock = NULL_POINTER, pStruc = sanchor.StrucList; pStruc; pStruc = pStruc->pNext)
      {
        for( pPreviousBlock = NULL_POINTER, pBlock = pStruc->Blocks
           ; pBlock
           ; pPreviousBlock = pBlock, pBlock = pBlock->pNext)
        {
          if(pBlock->Offset > i)
            break;
        }
        if(pPreviousBlock)
        {
          if(((i - pPreviousBlock->Offset) < pPreviousBlock->Size) &&
             ((i - pPreviousBlock->Offset) < Difference))
          {
            pNearestBlock = pPreviousBlock;
            Difference    = (UINT32)(i - pPreviousBlock->Offset);
          }
        }
      }
      if(pNearestBlock)
      {
        calculate_address(pNearestBlock->Offset, &address);
        printf("Closest matching block is %u bytes off and has address: 0x%016"Int64"X, offset %u:",
               Difference, address.l, pNearestBlock->Offset);
        printfhex(pNearestBlock->Data, 16);
      }
    }
    if(!memcmp(info.filebuffer+i, &swapped.number, 8))
    {
      printf("Found match for reversed 0x%p at offset %d: ", pPtr->Data, (int)i);
      printfhex((info.filebuffer+i) - 4, 16);
    }
  }

  return OK;
}


CORE_POINTER* find_block(MQUINT64 Offset)
{
  CORE_STRUCTURE* pStruc;
  CORE_POINTER*   pBlock;

  for(pStruc = sanchor.StrucList; pStruc; pStruc = pStruc->pNext)
  {
    for( pBlock = pStruc->Blocks
       ; pBlock
       ; pBlock = pBlock->pNext)
    {
      if(pBlock->Offset == Offset)
      {
        return pBlock;
      }
    }
  }

  return NULL_POINTER;
}


/*********************************************************************/
/* Display the full data for a block.                                */
/*********************************************************************/
int display_block( int            Block
                 , CORE_POINTER*  pPtr )
{
  PMQCHAR         pData;
  ULONG64         Size;
  ULONG64         Printing;
  ULONG64         Remaining;
  MQCHAR          UserInput[1024];
  MQBOOL          Hex = FALSE;
  MQUINT64        ByteString;
  UINT32          ByteStringLen;
  CORE_POINTER    Ptr;
  CORE_POINTER*   pFoundPtr;
  CORE_VAR        Address;
  MQUINT64        Offset;

restart:
  pData  = pPtr->Data;
  Size   = pPtr->Size;
  Offset = pPtr->Offset;
  if(!xcsCheckEyeCatcher(pData, cccSTORAGE_EYECATCHER))
  {
    pData  += sizeof(cccSTORAGE);
    Size   -= sizeof(cccSTORAGE);
    Offset += sizeof(cccSTORAGE);
  }
  calculate_address(Offset, &Address);

  printf( " %4d %s %20"Int64"u     @ 0x%016"Int64"X\n"
        , Block
        , pPtr->State == CORE_STATE_ALLOCATED ? "A" : "C"
        , pPtr->Size
        , Address.l );

  if(Hex || !SBTryFormatInternalStructure(info.vrmf, info.platform_string, info.bits, TRUE, (long*)&Size, &pData, SBCallbackFunc, "Data Structure:"))
  {
    Hex = FALSE;
    Remaining = Size;
    while(Remaining)
    {
      Printing = min(Remaining, 4096);
      printfhex(pData, Printing);
      pData += Printing;
      Remaining -= Printing;
      if(Remaining)
      {
        printf("* Enter B to return to the block list or enter to continue viewing *\n");
        fgets(UserInput, 1024, stdin);
        if(BACK(UserInput[0]))
          break;
      }
    }
  }

  while(TRUE)
  {
    printf("%s\n", DASH_STRING);
    printf("* Enter S to search for references to this block in the file, H to print in hex, or B to return to the block list *\n");
    fgets(UserInput, 1024, stdin);
    if(SEARCH(UserInput[0]))
    {
      search_for_block(pPtr);
    }
    else if(BACK_OR_BLANK(UserInput[0]))
    {
      break;
    }
    else if(HEX(UserInput[0]))
    {
      Hex = TRUE;
      goto restart;
    }
    else if((UserInput[0] == '0') && (UserInput[1] == 'x'))
    {
      INT32 Remaining = 19 - (INT32)strlen(UserInput);
      if(Remaining > 0)
      {
        memmove(UserInput+Remaining+2, UserInput+2, 16);
        memset(UserInput+2, '0', Remaining);
      }
      if(xcsConvertStringToByteString( MQENC_NATIVE
                                     , CSCtrl->xihCS_CCSID
                                     , (UINT32)strlen(UserInput)-2
                                     , UserInput+2
                                     , 16
                                     , (PMQBYTE)&ByteString
                                     , &ByteStringLen ) != OK)
      {
        printf("ERROR: string not valid hex\n");
        continue;
      }
      else if(ByteStringLen != 8)
      {
        printf("ERROR: string not a full pointer type\n");
        continue;
      }

      if(MQENC_NATIVE & MQENC_INTEGER_REVERSED)
        xcsConvertInt64(MQENC_INTEGER_NORMAL, MQENC_INTEGER_REVERSED, 1, &ByteString);

      Ptr.Offset = (UINT32)calculate_offset(ByteString);
      Ptr.State  = CORE_STATE_UNKNOWN;
      Ptr.Size   = 16;
      Ptr.Data   = info.filebuffer + Ptr.Offset;
      Ptr.pNext  = NULL_POINTER;
      pFoundPtr = find_block(Ptr.Offset);
      if(pFoundPtr)
      {
        display_block(1, pFoundPtr);
      }
      else if(Ptr.Offset < info.filesize)
      {
        display_block(1, &Ptr);
      }
      else
      {
        printf("Address at 0x%016"Int64"X, Offset %"Int64"u not mapped\n", ByteString, Ptr.Offset);
      }
    }
    else
    {
      printf("ERROR: Select S or B\n");
      continue;
    }
  }

  return OK;
}


/*********************************************************************/
/* Display the list of memory that a function has allocated.         */
/*********************************************************************/
int display_function(int Selection)
{
  CORE_FUNCTION*  pStorage = manchor.FunctionTable[Selection-1];
  CORE_POINTER*   pPtr;
  PMQCHAR         pData;
  long            Size;
  int             i;
  MQCHAR          UserInput[1024];
  int             Block;

  while(TRUE)
  {
    /*************************************************************/
    /* Print out the details for this function.                  */
    /*************************************************************/
    printf( "%3u  %-*.*s\n"
          , (unsigned int)Selection
          , manchor.MaxNameLength, manchor.MaxNameLength, pStorage->Name );
    printf("Block A/C              Size       Data (16 bytes)\n");
    printf("%s\n", DASH_STRING);
    for(i = 1, pPtr = pStorage->pPtr; pPtr; pPtr = pPtr->pNext, i++)
    {
      printf( " %4d %s %20"Int64"u"
            , i
            , pPtr->State == CORE_STATE_ALLOCATED ? "A" : "C"
            , pPtr->Size );
      pData = pPtr->Data;
      Size  = (long)pPtr->Size;
      printfhex(pData, pPtr->Size < 16 ? pPtr->Size : 16);
    }
    printf( "        %20"Int64"u\n"
          , pStorage->Allocated.Size + pStorage->Cached.Count );
    printf("%s\n", DASH_STRING);

    printf("\n* Enter a block between 1 and %d to view the data in full or B to return the function list *\n", i-1);
    fgets(UserInput, 1024, stdin);
    if(BACK_OR_BLANK(UserInput[0]))
      break;
    Block = xcsAtoi32(UserInput);
    if((Block < 1) || (Block > (i-1)))
    {
      printf("ERROR: block not between 1 and %d\n", i-1);
      continue;
    }

    for(i = 1, pPtr = pStorage->pPtr; i < Block; pPtr = pPtr->pNext, i++);
    display_block(Block, pPtr);
  }

  return OK;
}


/*********************************************************************/
/* Display what storage MQ is using.                                 */
/*********************************************************************/
UINT32 display_memory()
{
  unsigned int    i;
  CORE_FUNCTION*  pStorage;
  MQCHAR          UserInput[1024];
  int             Selection;

  while(TRUE)
  {
    /*****************************************************************/
    /* Display what we've found still in use.                        */
    /*****************************************************************/
    printf("Function%-.*sA/Blocks              A/Bytes   C/Blocks              C/Bytes\n", manchor.MaxNameLength, BLANK_STRING);
    printf("%s\n", DASH_STRING);
    for(i = 0; i < manchor.InUse; i++)
    {
      pStorage = manchor.FunctionTable[i];

      /***************************************************************/
      /* Print out the details for this function.                    */
      /***************************************************************/
      printf( "%3u  %-*.*s %10u %20"Int64"u %10u %20"Int64"u (%hu,%hu)\n"
            , i+1
            , manchor.MaxNameLength, manchor.MaxNameLength, pStorage->Name
            , pStorage->Allocated.Count
            , pStorage->Allocated.Size
            , pStorage->Cached.Count
            , pStorage->Cached.Size
            , pStorage->Component, pStorage->Function );
    }
    printf("%s\n", DASH_STRING);
    printf("Total%-.*s %10u %20"Int64"u %10u %20"Int64"u\n",
           manchor.MaxNameLength, BLANK_STRING,
           manchor.AllocatedStorage.Count, manchor.AllocatedStorage.Size,
           manchor.CachedStorage.Count, manchor.CachedStorage.Size);
    printf("     %-.*s %10u %20"Int64"u\n",
           manchor.MaxNameLength, BLANK_STRING, manchor.TotalStorage.Count, manchor.TotalStorage.Size);

    printf("\n* Enter a function between 1 and %u to view block or B to return to the main menu *\n", manchor.InUse+1);
    fgets(UserInput, 1024, stdin);
    if(BACK_OR_BLANK(UserInput[0]))
      break;
    Selection = xcsAtoi32(UserInput);
    if((Selection < 1) || (Selection > (int)manchor.InUse))
    {
      printf("ERROR: function selection not between 1 and %u\n", manchor.InUse+1);
      continue;
    }

    display_function(Selection);
  }

  return OK;
}


/*********************************************************************/
/* Display the specified structure.                                  */
/*********************************************************************/
UINT32 display_structure(int Structure)
{
  CORE_STRUCTURE* pStruc;
  int             i;
  CORE_POINTER*   pPtr;
  MQCHAR          UserInput[1024];
  int             Block;
  CORE_VAR        Address;

  for(i = 1, pStruc = sanchor.StrucList; i < Structure; pStruc = pStruc->pNext, i++);

  while(TRUE)
  {
    printf("Block  Pointer                    Size      Data (16 bytes)\n");
    printf("%s\n", DASH_STRING);

    for(i = 1, pPtr = pStruc->Blocks; pPtr; pPtr = pPtr->pNext, i++)
    {
      calculate_address(pPtr->Offset, &Address);
      printf(" %4d  %10u 0x%016"Int64"X %20"Int64"u", i, pPtr->Offset, Address.l, pPtr->Size);
      printfhex(pPtr->Data, pPtr->Size < 16 ? pPtr->Size : 16);
    }

    printf("\n* Enter a block between 1 and %d to view the data in full or B to return the structure list *\n", i-1);
    fgets(UserInput, 1024, stdin);
    if(BACK_OR_BLANK(UserInput[0]))
      break;
    Block = xcsAtoi32(UserInput);
    if((Block < 1) || (Block > (i-1)))
    {
      printf("ERROR: block not between 1 and %d\n", i-1);
      continue;
    }

    for(i = 1, pPtr = pStruc->Blocks; i < Block; pPtr = pPtr->pNext, i++);
    display_block(Block, pPtr);
  }

  return OK;
}


/*********************************************************************/
/* Display the structures we've discovered in the core file.         */
/*********************************************************************/
UINT32 display_structures()
{
  int             i;
  CORE_STRUCTURE* pStruc;
  MQCHAR          UserInput[1024];
  int             Structure;

  while(TRUE)
  {
    printf("Structure Eye\n");
    printf("%s\n", DASH_STRING);

    for(i = 1, pStruc = sanchor.StrucList; pStruc; pStruc = pStruc->pNext, i++)
    {
      printf(" %4d     '%-4.4s'\n", i, pStruc->EyeCatcher);
    }

    printf("\n* Select an eye catcher between 1 and %d to view the list of structures or B to return the main menu *\n", i-1);
    fgets(UserInput, 1024, stdin);
    if(BACK_OR_BLANK(UserInput[0]))
      break;
    Structure = xcsAtoi32(UserInput);
    if((Structure < 1) || (Structure > (i-1)))
    {
      printf("ERROR: structure not between 1 and %d\n", i-1);
      continue;
    }

    display_structure(Structure);
  }

  return OK;
}


/*********************************************************************/
/* Analyze what storage MQ is using.                                 */
/*********************************************************************/
int analyze_memory(int filesize, char* filebuffer)
{
  int             rc               = CORE_OK;
  size_t          TableSize        = xcsCOMPONENT_COUNT *         /* Num of components */
                                     xcsMAX_FUNCS_PER_COMPONENT * /* Num of funcs allowed per component */
                                     sizeof(CORE_FUNCTION);       /* Amount of storage required per function */
  CORE_FUNCTION*  pStorage;
  unsigned int    i;
  USHORT          comp;
  USHORT          func;
  PSZ             func_name;
  ULONG64         length64;
  CORE_POINTER*   pPtr;
  CORE_POINTER*   pNextPtr;
  CORE_POINTER*   pPreviousPtr;

  printf("Analyzing allocated memory...\n");

  /*******************************************************************/
  /* Malloc storage for each function that could be called.          */
  /*******************************************************************/
  manchor.StorageTable = (CORE_FUNCTION*)malloc(TableSize);
  memset(manchor.StorageTable, 0, TableSize);

  /*******************************************************************/
  /* Check for the XSPG memory eye catcher.                          */
  /*******************************************************************/
  for(i = 0; i < (filesize-sizeof(xstSTORHEAD)); i++)
  {
    /*****************************************************************/
    /* Pretend this bit of the file is MQ allocated memory.          */
    /*****************************************************************/
    xstPSTORHEAD head = (xstPSTORHEAD)(filebuffer+i);

    /*****************************************************************/
    /* Check if the eyecatchers match.                               */
    /*****************************************************************/
    if(!xcsCheckEyeCatcher(head->eyec, xstEYE))
    {
      /***************************************************************/
      /* Check if this storage is still in use.                      */
      /***************************************************************/
      if( ((head->state == xstState_allocated) || (head->state == xstState_cached)) &&
          (head->function <= xcsMAX_FUNCS_PER_COMPONENT) )
      {
        /*************************************************************/
        /* Record this use.                                          */
        /*************************************************************/
        pPtr = (CORE_POINTER*)malloc(sizeof(CORE_POINTER));
        pPtr->Offset = i;

        /*************************************************************/
        /* Find the length of this storage.                          */
        /*************************************************************/
        if(info.bits == CORE_ADDRESSING_MODE_64)
        {
          length64   = *(ULONG64*)(filebuffer+i+xstLENGTH_OFFSET_64);
          pPtr->Data = (PMQCHAR)(filebuffer+i+xstDATA_OFFSET_64);
          pPtr->Offset += xstDATA_OFFSET_64;
        }
        else
        {
          length64   = (ULONG64)(*(PUINT32)(filebuffer+i+xstLENGTH_OFFSET_32));
          pPtr->Data = (PMQCHAR)(filebuffer+i+xstDATA_OFFSET_32);
          pPtr->Offset += xstDATA_OFFSET_32;
        }
        pPtr->Size   = length64;

        /*************************************************************/
        /* Find this function in the array.                          */
        /*************************************************************/
        pStorage = &manchor.StorageTable[(head->component * xcsMAX_FUNCS_PER_COMPONENT) + head->function];

        /*************************************************************/
        /* Record either allocated or cached use.                    */
        /*************************************************************/
        if(head->state == xstState_allocated)
        {
          pPtr->State = CORE_STATE_ALLOCATED;
          RecordStorageUse(pStorage->Allocated, length64);
          RecordStorageUse(manchor.AllocatedStorage, length64);

        }
        else
        {
          pPtr->State = CORE_STATE_CACHED;
          RecordStorageUse(pStorage->Cached, length64);
          RecordStorageUse(manchor.CachedStorage, length64);
        }

        /*************************************************************/
        /* Record this in the total use.                             */
        /*************************************************************/
        RecordStorageUse(manchor.TotalStorage, length64);

        /*************************************************************/
        /* Chain in this storage in order of size.                   */
        /*************************************************************/
        for( pNextPtr = pStorage->pPtr, pPreviousPtr = NULL_POINTER
           ; pNextPtr
           ; pPreviousPtr = pNextPtr, pNextPtr = pNextPtr->pNext )
        {
          if(pNextPtr->Size < pPtr->Size)
          {
            break;
          }
        }
        if(pPreviousPtr)
        {
          pPreviousPtr->pNext = pPtr;
        }
        else
        {
          pStorage->pPtr = pPtr;
        }
        pPtr->pNext = pNextPtr;

        /*************************************************************/
        /* How big is the function name?                             */
        /*************************************************************/
        if(!pStorage->Name)
        {
          if(xtrGetFunction(head->component, head->function, &func_name) == OK)
          {
            pStorage->Name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_engine_misc,1000),func_name);
            if((UINT32)strlen(func_name) > manchor.MaxNameLength)
              manchor.MaxNameLength = (UINT32)strlen(func_name);
          }
          else
          {
            pStorage->Name = "<unknown>";
          }
          pStorage->Component = head->component;
          pStorage->Function  = head->function;
          manchor.InUse++;
        }
      }

      /***************************************************************/
      /* Skip past the tail.                                         */
      /***************************************************************/
      i += (UINT32)length64 + sizeof(xstSTORHEAD) + 12;
    }
    else if(!xcsCheckEyeCatcher(head->eyec, xstEYE_FREED))
    {
    }
  }

  /*******************************************************************/
  /* Create an array of in use functions.                            */
  /*******************************************************************/
  manchor.FunctionTable = (CORE_FUNCTION**)malloc(sizeof(CORE_FUNCTION*) * manchor.InUse);
  for(i = 0, comp = 1; (comp < xcsCOMPONENT_COUNT); comp++)
  {
    for(func = 0; (func < xcsMAX_FUNCS_PER_COMPONENT); func++)
    {
      pStorage = &manchor.StorageTable[(comp * xcsMAX_FUNCS_PER_COMPONENT) + func];
      if(pStorage->Name)
      {
        manchor.FunctionTable[i++] = pStorage;
      }
    }
  }

  return rc;
}


/*********************************************************************/
/* Analyze what structures are in the core file that we know how to  */
/* format.                                                           */
/*********************************************************************/
int analyze_structures(int filesize, char* filebuffer)
{
  int             rc = CORE_OK;
  int             i;
  unsigned char*  pData;
  unsigned char*  pDataStart;
  TPTopStructs    ptoptype         = NULL;
  FILE*           tempFile         = NULL;
  CORE_POINTER*   pPtr;
  CORE_STRUCTURE* pStruc;
  CORE_STRUCTURE* pNextStruc;
  CORE_STRUCTURE* pPreviousStruc;
  xcsEYECATCHER   Eye;
  CORE_POINTER*   pPreviousBlock;
  CORE_POINTER*   pNextBlock;

  printf("Analyzing structures...\n");

  errfd = stdout;

  /* Set hints: VRMF */
  if((info.vrmf[0] == '9') && (info.vrmf[1] == '9'))
  {
    info.vrmf[1] = '8';
  }

  SetFormatVariants(info.platform_string, info.vrmf, TRUE, (info.bits==CORE_ADDRESSING_MODE_64));

  /* Open temporary file */
  #define TEMPFILE "���DMFORMAT���"
  tempFile = fopen(TEMPFILE, "w+");
  if (tempFile != NULL)
  {
    outfd = tempFile;
  }
  else
  {
    printf("ERROR: unable to open temporary file\n");
    rc = CORE_ERROR;
    goto exit;
  }

  for (i = 0; i < filesize; i += 4)
  {
    pDataStart = pData = (unsigned char*)(filebuffer + i);

    if(DetermineDataType(pData, NULL, 0, &ptoptype) != OK) continue;

    FormatKnownData(ptoptype, filesize-i, &pData);

    xcsSetEyeCatcher(Eye, pDataStart);
    for(pStruc = sanchor.StrucList; pStruc; pStruc = pStruc->pNext)
    {
      if(!xcsCheckEyeCatcher(pStruc->EyeCatcher, Eye))
      {
        break;
      }
    }
    if(!pStruc)
    {
      pStruc = (CORE_STRUCTURE*)malloc(sizeof(CORE_STRUCTURE));
      xcsSetEyeCatcher(pStruc->EyeCatcher, Eye);
      pStruc->Count = 0;
      pStruc->Blocks = NULL_POINTER;

      for( pNextStruc = sanchor.StrucList, pPreviousStruc = NULL_POINTER
         ; pNextStruc
         ; pPreviousStruc = pNextStruc, pNextStruc = pNextStruc->pNext )
      {
        if(strncmp(Eye, pNextStruc->EyeCatcher, 4) < 0)
        {
          break;
        }
      }

      if(pPreviousStruc)
      {
        pPreviousStruc->pNext = pStruc;
      }
      else
      {
        sanchor.StrucList = pStruc;
      }
      pStruc->pNext = pNextStruc;
      sanchor.Count++;
    }

    pPtr = (CORE_POINTER*)malloc(sizeof(CORE_POINTER));
    pPtr->Offset = (UINT32)i;
    pPtr->State  = CORE_STATE_UNKNOWN;
    pPtr->Size   = (ULONG64)(pData - pDataStart);
    pPtr->Data   = (PMQCHAR)pDataStart;

    for( pPreviousBlock = NULL_POINTER, pNextBlock = pStruc->Blocks
       ; pNextBlock
       ; pPreviousBlock = pNextBlock, pNextBlock = pNextBlock->pNext)
    {
      if(pNextBlock->Offset > (MQUINT64)i)
        break;
    }
    if(pPreviousBlock)
    {
      pPreviousBlock->pNext = pPtr;
    }
    else
    {
      pStruc->Blocks = pPtr;
    }
    pPtr->pNext  = pNextBlock;
    pStruc->Count++;

    sanchor.HighestOffset = (UINT32)max((i + pPtr->Size + 32768), info.filesize);
  }

  outfd = stdout;

exit:
  if(tempFile )
  {
   fclose(tempFile);
   remove(TEMPFILE);
  }

  return rc;
}


/*********************************************************************/
/* Do some non-MQ analysis on the core file.                         */
/*********************************************************************/
int analyze_core()
{
  int rc = CORE_OK;
  CORE_ADDRESSING_MODE bits;
  MQUINT16 i;
  CORE_MEMORY_MAP* pMap;
  CORE_MEMORY_MAP* pMap2;
  CORE_MEMORY_MAP* pNextMap;
  CORE_MEMORY_MAP* pPreviousMap;

  printf("Analyzing core file headers...\n");

  /*******************************************************************/
  /* If the file is less than eight bytes in size, that it's not     */
  /* going to contain very much!                                     */
  /*******************************************************************/
  if(info.filesize < 8)
  {
    printf("ERROR: File %s too large small to be of much use (%u bytes)\n",
           info.filename, info.filesize);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Check for an ELF header.                                        */
  /*******************************************************************/
  if(!memcmp(info.filebuffer, "\177ELF", 4))
  {
    /*****************************************************************/
    /* It is an ELF header.                                          */
    /*****************************************************************/
    info.format = CORE_FORMAT_ELF;

    /*****************************************************************/
    /* Check for the addressing mode.                                */
    /*****************************************************************/
    if(info.filebuffer[4] == 1)
      bits = CORE_ADDRESSING_MODE_32;
    else if(info.filebuffer[4] == 2)
      bits = CORE_ADDRESSING_MODE_64;

    /*****************************************************************/
    /* Check for consistency with the specified value.               */
    /*****************************************************************/
    if((info.bits != CORE_ADDRESSING_MODE_UNKNOWN) &&
       (info.bits != bits))
    {
      printf("ERROR: core file's internal addressing mode (%d bit) does not match the specified value\n", bits);
      rc = CORE_ERROR;
      goto exit;
    }
    info.bits = bits;

    /*****************************************************************/
    /* Check the endian-ness.                                        */
    /*****************************************************************/
    if(info.filebuffer[5] == 1)
    {
      if(MQENC_NATIVE & MQENC_INTEGER_REVERSED)
        info.byteswap = FALSE;
      else
        info.byteswap = TRUE;
    }
    else if(info.filebuffer[5] == 2)
    {
      if(MQENC_NATIVE & MQENC_INTEGER_REVERSED)
        info.byteswap = TRUE;
      else
        info.byteswap = FALSE;
    }

    /*****************************************************************/
    /* Parse the core dumps headers.                                 */
    /*****************************************************************/
    if(bits == CORE_ADDRESSING_MODE_32)
    {
      Elf32_Ehdr* pHdr = (Elf32_Ehdr*)info.filebuffer;
      Elf32_Phdr* pPhdr;

      for( i = 0, pPhdr = (Elf32_Phdr*)(info.filebuffer+pHdr->e_phoff)
         ; i < pHdr->e_phnum
         ; i++, pPhdr++)
      {
        pMap = (CORE_MEMORY_MAP*)malloc(sizeof(CORE_MEMORY_MAP));
        pMap->Virtual.s = pPhdr->p_vaddr;
        pMap->Offset    = (MQUINT64)pPhdr->p_offset;
        pMap->pNext     = info.offset_map;
        if(info.offset_map == NULL)
        {
          info.offset_map = pMap;
        }
      }
    }
    else
    {
      Elf64_Ehdr* pHdr = (Elf64_Ehdr*)info.filebuffer;
      Elf64_Phdr* pPhdr;
      Elf64_Shdr* pShdr;

      for( i = 0, pPhdr = (Elf64_Phdr*)(info.filebuffer+pHdr->e_phoff)
         ; i < pHdr->e_phnum
         ; i++, pPhdr++)
      {
        if(pPhdr->p_vaddr == 0) continue;
        pMap = (CORE_MEMORY_MAP*)malloc(sizeof(CORE_MEMORY_MAP));
        pMap->Type      = CORE_MEM_TYPE_PROGRAM_HEADER;
        pMap->Number    = i;
        pMap->Size      = pPhdr->p_memsz;
        pMap->Virtual.l = pPhdr->p_vaddr;
        pMap->Offset    = pPhdr->p_offset;
        pMap2 = (CORE_MEMORY_MAP*)malloc(sizeof(CORE_MEMORY_MAP));
        memcpy(pMap2, pMap, sizeof(CORE_MEMORY_MAP));

        for( pNextMap = info.offset_map, pPreviousMap = NULL_POINTER
           ; pNextMap
           ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
        {
          if(pNextMap->Offset > pMap->Offset)
          {
            break;
          }
        }
        if(pPreviousMap)
        {
          pPreviousMap->pNext = pMap;
        }
        else
        {
          info.offset_map = pMap;
        }
        pMap->pNext = pNextMap;

        for( pNextMap = info.address_map, pPreviousMap = NULL_POINTER
           ; pNextMap
           ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
        {
          if(pNextMap->Virtual.l > pMap2->Virtual.l)
          {
            break;
          }
        }
        if(pPreviousMap)
        {
          pPreviousMap->pNext = pMap2;
        }
        else
        {
          info.address_map = pMap2;
        }
        pMap2->pNext = pNextMap;
      }

      for( i = 0, pShdr = (Elf64_Shdr*)(info.filebuffer+pHdr->e_shoff)
         ; i < pHdr->e_shnum
         ; i++, pShdr++)
      {
        if(pShdr->sh_addr == 0) continue;
        pMap = (CORE_MEMORY_MAP*)malloc(sizeof(CORE_MEMORY_MAP));
        pMap->Type      = CORE_MEM_TYPE_SECTION_HEADER;
        pMap->Number    = i;
        pMap->Size      = pShdr->sh_size;
        pMap->Virtual.l = pShdr->sh_addr;
        pMap->Offset    = pShdr->sh_offset;
        pMap2 = (CORE_MEMORY_MAP*)malloc(sizeof(CORE_MEMORY_MAP));
        memcpy(pMap2, pMap, sizeof(CORE_MEMORY_MAP));

        for( pNextMap = info.offset_map, pPreviousMap = NULL_POINTER
           ; pNextMap
           ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
        {
          if(pNextMap->Offset > pMap->Offset)
          {
            break;
          }
        }
        if(pPreviousMap)
        {
          pPreviousMap->pNext = pMap;
        }
        else
        {
          info.offset_map = pMap;
        }
        pMap->pNext = pNextMap;

        for( pNextMap = info.address_map, pPreviousMap = NULL_POINTER
           ; pNextMap
           ; pPreviousMap = pNextMap, pNextMap = pNextMap->pNext )
        {
          if(pNextMap->Virtual.l > pMap2->Virtual.l)
          {
            break;
          }
        }
        if(pPreviousMap)
        {
          pPreviousMap->pNext = pMap2;
        }
        else
        {
          info.address_map = pMap2;
        }
        pMap2->pNext = pNextMap;
      }
    }
  }

exit:
  return rc;
}


int main(int argc, char* argv[])
{
  int                  rc           = CORE_OK;
  int                  c;
  int                  fd           = -1;
  CORE_MODE            mode         = CORE_MODE_NONE;
  MQCHAR               UserInput[1024];

  /*******************************************************************/
  /* Initialize with common services.                                */
  /*******************************************************************/
  xcsInitialize( xcsIC_CREATE_OR_CONNECT_SUBPOOL
               , xcsIC_MINI_INITIALISE
               , xcsIC_MINI_SUBPOOL_NAME
               , NULL_POINTER
               , NULL_POINTER );

  xcsProgramInit(argc, argv, xcsPROGRAMFLAGS_NONE);

  /*******************************************************************/
  /* If there is a single argument which is just a question mark,    */
  /* they're after the command's usage message.                      */
  /*******************************************************************/
  if((argc == 2) && (strcmp(argv[1], "?") == 0))
  {
    printf(USAGE);
    goto exit;
  }

  /*******************************************************************/
  /* Now get whatever we were given from the command line.           */
  /*******************************************************************/
  while ((c = xugGetOpt(argc, argv, "c:p:v:x:")) != EOF)
  {
    /*****************************************************************/
    /* Now process the flag which has been found.                    */
    /*****************************************************************/
    switch (c)
    {
      case 'c':
        info.filename = xugoptarg;
        break;

      case 'p':
        info.platform_string = xugoptarg;
        if(!strcmp(info.platform_string, "linux_x86"))
          info.platform = CORE_PLATFORM_LINUX_X86;
        else if(!strcmp(info.platform_string, "linux_x86-64"))
          info.platform = CORE_PLATFORM_LINUX_X86_64;
        else if(!strcmp(info.platform_string, "linux_power"))
          info.platform = CORE_PLATFORM_LINUX_PPC;
        else if(!strcmp(info.platform_string, "aix_power"))
          info.platform = CORE_PLATFORM_AIX;
        else if(!strcmp(info.platform_string, "solaris_sparc"))
          info.platform = CORE_PLATFORM_SOLARIS_SPARC;
        else if(!strcmp(info.platform_string, "solaris_x86"))
          info.platform = CORE_PLATFORM_SOLARIS_X86_64;
        else if(!strcmp(info.platform_string, "hp_itanium"))
          info.platform = CORE_PLATFORM_HP;
        else
        {
          printf("ERROR: Invalid platform specified.\n");
          printf(USAGE);
          rc = CORE_ERROR;
          goto exit;
        }
        break;

      case 'v':
        info.vrmf = xugoptarg;
        break;

      case 'x':
        if(!strcmp(xugoptarg, "32"))
          info.bits = 32;
        else if(!strcmp(xugoptarg, "64"))
          info.bits = 64;
        else
        {
          printf("ERROR: Invalid addressing mode specified.\n");
          printf(USAGE);
          rc = CORE_ERROR;
          goto exit;
        }
        break;

      default:
        printf("ERROR: Invalid argument specified.\n");
        printf(USAGE);
        rc = CORE_ERROR;
        goto exit;
    }
  }

  /*******************************************************************/
  /* Make sure there are no dangling, unused arguments               */
  /*******************************************************************/
  if(xugoptind != argc)
  {
    printf("ERROR: Incorrect number of arguments specified.\n");
    printf(USAGE);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Make sure the core file name was specified.                     */
  /*******************************************************************/
  if(info.filename == NULL)
  {
    printf("ERROR: Core file name not specified.\n");
    printf(USAGE);
    rc = CORE_ERROR;
    goto exit;
  }

  /*******************************************************************/
  /* Attempt to load the core file.                                  */
  /*******************************************************************/
  rc = load_core(info.filename, &fd, &info.filesize, &info.filebuffer);
  if(rc == CORE_ERROR) goto exit;

  /*******************************************************************/
  /* Analyze the structure of the core file.                         */
  /*******************************************************************/
  rc = analyze_core();
  if(rc == CORE_ERROR) goto exit;

  /*******************************************************************/
  /* Analyze which structures are in the core file.                  */
  /*******************************************************************/
  rc = analyze_structures(info.filesize, info.filebuffer);
  if(rc == CORE_ERROR) goto exit;

  /*******************************************************************/
  /* Analyze allocated memory.                                       */
  /*******************************************************************/
  rc = analyze_memory(info.filesize, info.filebuffer);
  if(rc == CORE_ERROR) goto exit;

  /*******************************************************************/
  /* Ask the user which action they'd like to perform.               */
  /*******************************************************************/
  while(rc != CORE_ERROR)
  {
    printf("%s\n", DASH_STRING);
    printf("1. Display allocated memory\n");
    printf("2. Display discovered structures\n");
    printf("3. Display core file information\n");
    printf("Q. Quit the program\n");
    printf("\n* Enter an operation between 1 and %u or Q to quit *\n", (unsigned int)CORE_MODE_LAST-1);
    fgets(UserInput, 1024, stdin);
    if(QUIT_OR_BLANK(UserInput[0]))
      break;
    mode = xcsAtoi32(UserInput);
    if((mode <= CORE_MODE_NONE) || (mode >= CORE_MODE_LAST))
    {
      printf("ERROR: operation selection not between 1 and %u\n", (unsigned int)CORE_MODE_LAST-1);
      continue;
    }

    /*****************************************************************/
    /* Perform the specified action.                                 */
    /*****************************************************************/
    switch(mode)
    {
      /***************************************************************/
      /* Display allocated memory.                                   */
      /***************************************************************/
      case CORE_MODE_MEMORY:
        rc = display_memory();
        break;

      /***************************************************************/
      /* Display discovered structures.                              */
      /***************************************************************/
      case CORE_MODE_STRUCTURES:
        rc = display_structures();
        break;

      /***************************************************************/
      /* Dump out information about the core file.                   */
      /***************************************************************/
      case CORE_MODE_INFO:
        rc = display_info();
        break;

      default:
        break;
    }
  }

exit:
  /*******************************************************************/
  /* If we opened the file, unmap it and close it.                   */
  /*******************************************************************/
  if(fd != -1)
  {
    unload_core(info.filename, &fd, &info.filesize, &info.filebuffer);
  }

  return rc;
}
