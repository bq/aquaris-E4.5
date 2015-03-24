/* A stack unwinder. */

#ifndef _LIBC_CORKSCREW_H
#define _LIBC_CORKSCREW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/*
 * Describes a single frame of a backtrace.
 */
typedef struct {
    uintptr_t absolute_pc;     /* absolute PC offset */
    uintptr_t stack_top;       /* top of stack for this frame */
    size_t stack_size;         /* size of this stack frame */
} backtrace_frame_t;

//TODO: hash table size need be optimized
#define EXIDX_HASH_TABLE_SIZE      1543





typedef struct Exidx_buffer {
	size_t slot;
    struct Exidx_buffer * next;
	struct Exidx_buffer * pre;
    uintptr_t PC;
    uintptr_t entry_handler; //if not in  newest in link head, if exist,move to head
    
} Exidx_buffer,*PExidx_buffer;



typedef struct {
    size_t count;
    Exidx_buffer* slots[EXIDX_HASH_TABLE_SIZE];
}ExidxTable;

static inline uint32_t get_hash(intptr_t PCAddr)
{
    int hash =(hash * 33) + (PCAddr >> 2);
    return hash;
}


/*
 * Describes the symbols associated with a backtrace frame.
 */
typedef struct {
    uintptr_t relative_pc;       /* relative frame PC offset from the start of the library,
                                    or the absolute PC if the library is unknown */
    uintptr_t relative_symbol_addr; /* relative offset of the symbol from the start of the
                                    library or 0 if the library is unknown */
    char* map_name;              /* executable or library name, or NULL if unknown */
    char* symbol_name;           /* symbol name, or NULL if unknown */
    char* demangled_name;        /* demangled symbol name, or NULL if unknown */
} backtrace_symbol_t;

/*
 * Unwinds the call stack for the current thread of execution.
 * Populates the backtrace array with the program counters from the call stack.
 * Returns the number of frames collected, or -1 if an error occurred.
 */
ssize_t unwind_backtrace(backtrace_frame_t* backtrace, size_t ignore_depth, size_t max_depth);


/*
 * Gets the symbols for each frame of a backtrace.
 * The symbols array must be big enough to hold one symbol record per frame.
 * The symbols must later be freed using free_backtrace_symbols.
 */
void get_backtrace_symbols(const backtrace_frame_t* backtrace, size_t frames,
        backtrace_symbol_t* backtrace_symbols);

/*
 * Frees the storage associated with backtrace symbols.
 */
void free_backtrace_symbols(backtrace_symbol_t* backtrace_symbols, size_t frames);

enum {
    // A hint for how big to make the line buffer for format_backtrace_line
    MAX_BACKTRACE_LINE_LENGTH = 800,
};

/**
 * Formats a line from a backtrace as a zero-terminated string into the specified buffer.
 */
void format_backtrace_line(unsigned frameNumber, const backtrace_frame_t* frame,
        const backtrace_symbol_t* symbol, char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // _LIBC_CORKSCREW_H

