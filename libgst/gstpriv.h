/******************************** -*- C -*- ****************************
 *
 *	GNU Smalltalk generic inclusions.
 *
 *
 ***********************************************************************/

/***********************************************************************
 *
 * Copyright 1988,89,90,91,92,94,95,99,2000,2001,2002
 * Free Software Foundation, Inc.
 * Written by Steve Byrne.
 *
 * This file is part of GNU Smalltalk.
 *
 * GNU Smalltalk is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later 
 * version.
 * 
 * GNU Smalltalk is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * GNU Smalltalk; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 *
 ***********************************************************************/

#ifndef GST_GSTPRIV_H
#define GST_GSTPRIV_H

#include "config.h"
#include "gst.h"

/* Convenience macros to test the versions of GCC.  Note - they won't
   work for GCC1, since the _MINOR macros were not defined then, but
   we don't have anything interesting to test for that. :-) */
#if defined __GNUC__ && defined __GNUC_MINOR__
# define GNUC_PREREQ(maj, min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define GNUC_PREREQ(maj, min) 0
#endif

/* For internal functions, we can use the ELF hidden attribute to
   improve code generation.  Unluckily, this is only in GCC 3.2 and
   later */
#if HAVE_VISIBILITY_HIDDEN
#define ATTRIBUTE_HIDDEN __attribute__ ((visibility ("hidden")))
#else
#define ATTRIBUTE_HIDDEN
#endif

/* At some point during the GCC 2.96 development the `pure' attribute
   for functions was introduced.  We don't want to use it
   unconditionally (although this would be possible) since it
   generates warnings.

   GCC 2.96 also introduced branch prediction hints for basic block
   reordering.  We use a shorter syntax than the wordy one that GCC
   wants.  */
#if GNUC_PREREQ (2, 96)
#define UNCOMMON(x) (__builtin_expect ((x) != 0, 0))
#define COMMON(x)   (__builtin_expect ((x) != 0, 1))
#else
#define UNCOMMON(x) (x)
#define COMMON(x)   (x)
#endif

/* Prefetching macros.  The NTA version is for a read that has no
   temporal locality.  The second argument is the kind of prefetch
   we want, using the flags that follow (T0, T1, T2, NTA follow
   the names of the instructions in the SSE instruction set).  The
   flags are hints, there is no guarantee that the instruction set
   has the combination that you ask for -- just trust the compiler.

   There are three macros.  PREFETCH_ADDR is for isolated prefetches,
   for example it is used in the garbage collector's marking loop
   to be reasonably sure that OOPs are in the cache before they're
   marked.  PREFETCH_START and PREFETCH_LOOP usually go together, one
   in the header of the loop and one in the middle.  However, you may
   use PREFETCH_START only for small loops, and PREFETCH_LOOP only if
   you know that the loop is invoked often (this is done for alloc_oop,
   for example, to keep the next allocated OOPs in the cache).
   PREF_BACKWARDS is for use with PREFETCH_START/PREFETCH_LOOP.  */
#define PREF_READ 0
#define PREF_WRITE 1
#define PREF_BACKWARDS 2
#define PREF_T0 0
#define PREF_T1 4
#define PREF_T2 8
#define PREF_NTA 12

#if GNUC_PREREQ (3, 1)
#define DO_PREFETCH(x, distance, k) \
  __builtin_prefetch (((char *) (x)) \
		      + (((k) & PREF_BACKWARDS ? -(distance) : (distance)) \
			 << L1_CACHE_SHIFT), \
		      (k) & PREF_WRITE, \
		      3 - (k) / (PREF_NTA / 3))
#else
#define DO_PREFETCH(x, distance, kind) ((void)(x))
#endif

#define PREFETCH_START(x, k) do { \
  const char *__addr = (const char *) (x); \
  DO_PREFETCH (__addr, 0, (k)); \
  if (L1_CACHE_SHIFT >= 7) break; \
  DO_PREFETCH (__addr, 1, (k)); \
  if (L1_CACHE_SHIFT == 6) break; \
  DO_PREFETCH (__addr, 2, (k)); \
  DO_PREFETCH (__addr, 3, (k)); \
} while (0)

#define PREFETCH_LOOP(x, k) \
  DO_PREFETCH ((x), (L1_CACHE_SHIFT >= 7 ? 1 : 128 >> L1_CACHE_SHIFT), (k));

#define PREFETCH_ADDR(x, k) \
  DO_PREFETCH ((x), 0, (k));

/* Kill a warning when using GNU C.  Note that this allows using
   break or continue inside a macro, unlike do...while(0) */
#ifdef __GNUC__
#define BEGIN_MACRO ((void) (
#define END_MACRO ))
#else
#define BEGIN_MACRO if (1) 
#define END_MACRO else (void)0
#endif


/* ENABLE_SECURITY enables security checks in the primitives as well as
   special marking of untrusted objects.  Note that the code in the
   class library to perform the security checks will be present
   notwithstanding the setting of this flag, but they will be disabled
   because the corresponding primitives will be made non-working.
   It is undefined because the Makefiles take care of defining it for
   security-enabled builds.  */
#define ENABLE_SECURITY

/* OPTIMIZE disables many checks, including consistency checks at GC
   time and bounds checking on instance variable accesses (not on #at:
   and #at:put: which would violate language semantics).  It can a)
   greatly speed up code by simplifying the interpreter's code b) make
   debugging painful because you know of a bug only when it's too
   late.  It is undefined because the Makefiles take care of defining
   it for optimized builds.  Bounds-checking and other errors will
   call abort().  */
/* #define OPTIMIZE */

typedef unsigned char gst_uchar;

#ifdef NO_INLINES
#define inline
#else
#  if defined (__GNUC__)
#    undef inline
#    define inline __inline__	/* let's not lose when --ansi is
				   specified */
#  endif
#endif

/* If they have no const, they're likely to have no volatile, either.  */
#ifdef const
#define volatile
#endif

#ifndef HAVE_STRDUP
extern char *strdup ();
/* else it is in string.h */
#endif


/* Run-time flags are allocated from the top, while those
   that live across image saves/restores are allocated
   from the bottom.

   bit 0-3: reserved for distinguishing byte objects and saving their size.
   bit 4-11: non-volatile bits (special kinds of objects).
   bit 12-23: volatile bits (GC/JIT-related).
   bit 24-30: reserved for counting things.
   bit 31: unused to avoid signedness mess. */
enum {
  /* Place to save various OOP counts (how many fields we have marked
     in the object, how many pointer instance variables there are, 
     etc.).  Here is a distribution of frequencies in a standard image:
       2 to   31 words      24798 objects (96.10%)
      32 to   63 words        816 objects ( 3.16%)
      64 to   95 words         82 objects ( 0.32%)
      96 to  127 words         54 objects ( 0.21%)
     128 or more words         54 objects ( 0.21%)

    which I hope justifies the choice :-) */
  F_COUNT = (int) 0x7F000000U,
  F_COUNT_SHIFT = 24,

  /* Set if the object is reachable, during the mark phases of oldspace
     garbage collection.  */
  F_REACHABLE = 0x800000U,

  /* Set if a translation to native code is available, when running
     with the JIT compiler enabled.  */
  F_XLAT = 0x400000U,

  /* Set if a translation to native code is used by the currently
     reachable contexts.  */
  F_XLAT_REACHABLE = 0x200000U,

  /* Set if a translation to native code is available but not used by
     the reachable contexts at the time of the last GC.  We give
     another chance to the object, but if the translation is not used
     for two consecutive GCs we discard it.  */
  F_XLAT_2NDCHANCE = 0x100000U,

  /* Set if a translation to native code was discarded for this
     object (either because the programmer asked for this, or because
     the method conflicted with a newly-installed method).  */
  F_XLAT_DISCARDED = 0x80000U,

  /* One of this is set for objects that live in newspace.  */
  F_SPACES = 0x60000U,
  F_EVEN = 0x40000U,
  F_ODD = 0x20000U,

  /* Set if the OOP is allocated by the pools of contexts maintained
     in interp.c (maybe it belongs above...) */
  F_POOLED = 0x10000U,

  /* Set if the bytecodes in the method have been verified. */
  F_VERIFIED = 0x8000U,

  /* The grouping of all the flags which are not valid across image
     saves and loads.  */
  F_RUNTIME = 0xFFF000U,

  /* Set if the OOP is currently unused.  */
  F_FREE = 0x10U,

  /* Set if the references to the instance variables of the object
     are weak.  */
  F_WEAK = 0x20U,

  /* Set if the object is read-only.  */
  F_READONLY = 0x40U,

  /* Set if the object is a context and hence its instance variables
     are only valid up to the stack pointer.  */
  F_CONTEXT = 0x80U,

  /* Answer whether we want to mark the key based on references found
     outside the object.  */
  F_EPHEMERON = 0x100U,

  /* Set for objects that live in oldspace.  */
  F_OLD = 0x200U,

  /* Set together with F_OLD for objects that live in fixedspace.  */
  F_FIXED = 0x400U,

  /* Set for untrusted classes, instances of untrusted classes,
     and contexts whose receiver is untrusted.  */
  F_UNTRUSTED = 0x800U,

  /* Set to the number of bytes unused in an object with byte-sized
     instance variables.  Note that this field and the following one
     should be initialized only by INIT_UNALIGNED_OBJECT (not really 
     aesthetic but...) */
  EMPTY_BYTES = (sizeof (PTR) - 1),

  /* A bit more than what is identified by EMPTY_BYTES.  Selects some
     bits that are never zero if and only if this OOP identifies an
     object with byte instance variables.  */
  F_BYTE = 15
};

/* Answer whether a method, OOP, has already been verified. */
#define IS_OOP_VERIFIED(oop) \
  ((oop)->flags & F_VERIFIED)

/* Answer whether an object, OOP, is weak.  */
#define IS_OOP_WEAK(oop) \
  ((oop)->flags & F_WEAK)

/* Answer whether an object, OOP, is readonly.  */
#define IS_OOP_READONLY(oop) \
  (IS_INT ((oop)) ? F_READONLY : (oop)->flags & F_READONLY)

/* Set whether an object, OOP, is readonly or readwrite.  */
#define MAKE_OOP_READONLY(oop, ro) \
  (((oop)->flags &= ~F_READONLY), \
   ((oop)->flags |= (ro) ? F_READONLY : 0))

#ifdef ENABLE_SECURITY

/* Answer whether an object, OOP, is untrusted.  */
#define IS_OOP_UNTRUSTED(oop) \
  (IS_INT ((oop)) ? 0 : ((oop)->flags & F_UNTRUSTED))

/* Set whether an object, OOP, is trusted or untrusted.  */
#define MAKE_OOP_UNTRUSTED(oop, untr) \
  (((oop)->flags &= ~F_UNTRUSTED), \
   ((oop)->flags |= (untr) ? F_UNTRUSTED : 0))

#else
#define IS_OOP_UNTRUSTED(oop) 0
#define MAKE_OOP_UNTRUSTED(oop, untr) 0
#endif

/* Set whether an object, OOP, has ephemeron semantics.  */
#define MAKE_OOP_EPHEMERON(oop) \
  (oop)->flags |= F_EPHEMERON;


/*
      3                   2                   1 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    # fixed fields             |      unused       |I| kind  |1|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   I'm moving it to bits 13-30 (it used to stay at bits 5-30),
   allocating space for more flags in case they're needed.
   If you change ISP_NUMFIXEDFIELDS you should modify Behavior.st too.
   Remember to shift by ISP_NUMFIXEDFIELDS-1 there, since Smalltalk does
   not see ISP_INTMARK!!

   Keep these in sync with _gst_sizes, in dict.c.

   FIXME: these should be exported in a pool dictionary.  */

enum {
  /* This is a shift count.  */
  ISP_NUMFIXEDFIELDS = 13,

  /* Set if the indexed instance variables are pointers.  */
  ISP_FIXED = 0,
  ISP_SCHAR = 32,
  ISP_UCHAR = 34,
  ISP_SHORT = 36,
  ISP_USHORT = 38,
  ISP_INT = 40,
  ISP_UINT = 42,
  ISP_FLOAT = 44,
  ISP_INT64 = 46,
  ISP_UINT64 = 48,
  ISP_DOUBLE = 50,
  ISP_LAST_SCALAR = 50,
  ISP_POINTER = 62,

#if SIZEOF_LONG == 8
  ISP_LONG = ISP_INT64,
  ISP_ULONG = ISP_UINT64,
  ISP_LAST_UNALIGNED = ISP_FLOAT,
#else
  ISP_LONG = ISP_INT,
  ISP_ULONG = ISP_UINT,
  ISP_LAST_UNALIGNED = ISP_USHORT,
#endif

  /* Set if the instances of the class have indexed instance
     variables.  */
  ISP_ISINDEXABLE = 32,

  /* These represent the shape of the indexed instance variables of
     the instances of the class.  */
  ISP_INDEXEDVARS = 62,
  ISP_SHAPE = 30,

  /* Set to 1 to mark a SmallInteger.  */
  ISP_INTMARK = 1
};


/* the current execution stack pointer */
#ifndef ENABLE_JIT_TRANSLATION
# define sp		_gst_sp
#endif

/* The VM's stack pointer */
extern OOP *sp 
  ATTRIBUTE_HIDDEN;

/* Some useful constants */
extern OOP _gst_nil_oop 
  ATTRIBUTE_HIDDEN, _gst_true_oop 
  ATTRIBUTE_HIDDEN, _gst_false_oop 
  ATTRIBUTE_HIDDEN;

/* Some stack operations */
#define UNCHECKED_PUSH_OOP(oop) \
  (*++sp = (oop))

#define UNCHECKED_SET_TOP(oop) \
  (*sp = (oop))

#ifndef OPTIMIZE
#define PUSH_OOP(oop) \
  do { \
    OOP __pushOOP = (oop); \
    if (IS_OOP (__pushOOP) && !IS_OOP_VALID (__pushOOP)) \
      abort (); \
    UNCHECKED_PUSH_OOP (__pushOOP); \
  } while (0)
#else
#define PUSH_OOP(oop) \
  do { \
    OOP __pushOOP = (oop); \
    UNCHECKED_PUSH_OOP (__pushOOP); \
  } while (0)
#endif

#define POP_OOP() \
  (*sp--)

#define POP_N_OOPS(n) \
  (sp -= (n))

#define UNPOP(n) \
  (sp += (n))

#define STACKTOP() \
  (*sp)

#ifndef OPTIMIZE
#define SET_STACKTOP(oop) \
  do { \
    OOP __pushOOP = (oop); \
    if (IS_OOP (__pushOOP) && !IS_OOP_VALID (__pushOOP)) \
      abort (); \
    UNCHECKED_SET_TOP(__pushOOP); \
  } while (0)
#else
#define SET_STACKTOP(oop) \
  do { \
    OOP __pushOOP = (oop); \
    UNCHECKED_SET_TOP(__pushOOP); \
  } while (0)
#endif

#define SET_STACKTOP_INT(i) \
  UNCHECKED_SET_TOP(FROM_INT(i))

#define SET_STACKTOP_BOOLEAN(exp) \
  UNCHECKED_SET_TOP((exp) ? _gst_true_oop : _gst_false_oop)

#define STACK_AT(i) \
  (sp[-(i)])

#define PUSH_INT(i) \
  UNCHECKED_PUSH_OOP(FROM_INT(i))

#define POP_INT() \
  TO_INT(POP_OOP())

#define PUSH_BOOLEAN(exp) \
  PUSH_OOP((exp) ? _gst_true_oop : _gst_false_oop)


/* Answer whether CLASS is the class that the object pointed to by OOP
   belongs to.  OOP can also be a SmallInteger.  */
#define IS_CLASS(oop, class) \
  (OOP_INT_CLASS(oop) == class)

/* Answer the CLASS that the object pointed to by OOP belongs to.  OOP
   can also be a SmallInteger. */
#define OOP_INT_CLASS(oop) \
  (IS_INT(oop) ? _gst_small_integer_class : OOP_CLASS(oop))


/* Answer whether OOP is nil.  */
#define IS_NIL(oop) \
  ((OOP)(oop) == _gst_nil_oop)


/* This macro should only be used right after an alloc_oop, when the
   emptyBytes field is guaranteed to be zero.

   Note that F_BYTE is a bit more than EMPTY_BYTES, so that if value 
   is a multiple of sizeof (PTR) the flags identified by F_BYTE are
   not zero.  */
#define INIT_UNALIGNED_OBJECT(oop, value) \
    ((oop)->flags |= sizeof (PTR) | (value))


/* Generally useful conversion functions */
#define SIZE_TO_BYTES(size) \
  ((size) * sizeof (PTR))

#define BYTES_TO_SIZE(bytes) \
  ((bytes) / sizeof (PTR))


/* integer conversions and some information on SmallIntegers.  */

#define TO_INT(oop) \
  ((intptr_t)(oop) >> 1)

#define FROM_INT(i) \
  (OOP)( ((intptr_t)(i) << 1) + 1)

#define ST_INT_SIZE        ((sizeof (PTR) * 8) - 2)
#define MAX_ST_INT         ((1L << ST_INT_SIZE) - 1)
#define MIN_ST_INT         ( ~MAX_ST_INT)
#define INT_OVERFLOW(i)    ( (i) > MAX_ST_INT || (i) < MIN_ST_INT )
#define OVERFLOWING_INT    (MAX_ST_INT + 1)

#define INCR_INT(i)         ((OOP) (((intptr_t)i) + 2))	/* 1 << 1 */
#define DECR_INT(i)         ((OOP) (((intptr_t)i) - 2))	/* 1 << 1 */

/* Endian conversions, using networking functions if they do
   the correct job (that is, on 32-bit little-endian systems)
   because they are likely to be optimized.  */

#if SIZEOF_OOP == 4
# if defined(WORDS_BIGENDIAN) || !defined (HAVE_INET_SOCKETS)
#  define BYTE_INVERT(x) \
        ((uintptr_t)((((uintptr_t)(x) & 0x000000ffU) << 24) | \
                     (((uintptr_t)(x) & 0x0000ff00U) <<  8) | \
                     (((uintptr_t)(x) & 0x00ff0000U) >>  8) | \
                     (((uintptr_t)(x) & 0xff000000U) >> 24)))
# else
#  define BYTE_INVERT(x) htonl((x))
# endif

#else /* SIZEOF_OOP == 8 */
# define BYTE_INVERT(x) \
        ((uintptr_t)((((uintptr_t)(x) & 0x00000000000000ffU) << 56) | \
                     (((uintptr_t)(x) & 0x000000000000ff00U) << 40) | \
                     (((uintptr_t)(x) & 0x0000000000ff0000U) << 24) | \
                     (((uintptr_t)(x) & 0x00000000ff000000U) <<  8) | \
                     (((uintptr_t)(x) & 0x000000ff00000000U) >>  8) | \
                     (((uintptr_t)(x) & 0x0000ff0000000000U) >> 24) | \
                     (((uintptr_t)(x) & 0x00ff000000000000U) >> 40) | \
                     (((uintptr_t)(x) & 0xff00000000000000U) >> 56)))
#endif /* SIZEOF_OOP == 8 */

/* The standard min/max macros...  */

#ifndef ABS
#define ABS(x) (x >= 0 ? x : -x)
#endif
#ifndef MAX
#define MAX(x, y) 		( ((x) > (y)) ? (x) : (y) )
#endif
#ifndef MIN
#define MIN(x, y) 		( ((x) > (y)) ? (y) : (x) )
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stddef.h>
#include <setjmp.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <obstack.h>
#include <fcntl.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <poll.h>
#include <ctype.h>

#ifdef HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if defined(HAVE_SYS_TIME_H) || defined(TIME_WITH_SYS_TIME)
# include <sys/time.h>
#endif
#if !defined(HAVE_SYS_TIME_H) || defined(TIME_WITH_SYS_TIME)
# include <time.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "stdintx.h"
#include "ansidecl.h"
#include "signalx.h"
#include "mathl.h"
#include "socketx.h"
#include "strspell.h"
#include "alloc.h"
#include "md-config.h"
#include "memzero.h"
#include "avltrees.h"
#include "rbtrees.h"

#include "tree.h"
#include "input.h"
#include "callin.h"
#include "cint.h"
#include "dict.h"
#include "events.h"
#include "gstpub.h"
#include "heap.h"
#include "lex.h"
#include "gst-parse.h"
#include "lib.h"
#include "oop.h"
#include "byte.h"
#include "comp.h"
#include "interp.h"
#include "opt.h"
#include "save.h"
#include "str.h"
#include "sysdep.h"
#include "sym.h"
#include "xlat.h"
#include "mpz.h"
#include "print.h"
#include "security.h"
#include "match.h"

/* Include this last, it has the bad habit of #defining printf
   and this fools gcc's __attribute__ (format) */
#include "snprintfv/printf.h"

#undef obstack_init
#define obstack_init(h)                                         \
  _obstack_begin ((h), 0, LONG_DOUBLE_ALIGNMENT,                \
                  (void *(*) (long)) obstack_chunk_alloc,       \
                  (void (*) (void *)) obstack_chunk_free)

#undef obstack_begin
#define obstack_begin(h, size)                                  \
  _obstack_begin ((h), (size), LONG_DOUBLE_ALIGNMENT,           \
                  (void *(*) (long)) obstack_chunk_alloc,       \
                  (void (*) (void *)) obstack_chunk_free)

#include "oop.inl"
#include "dict.inl"
#include "interp.inl"
#include "comp.inl"

#endif /* GST_GSTPRIV_H */
