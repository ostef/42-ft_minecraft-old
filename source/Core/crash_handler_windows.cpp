#include "Core.hpp"

struct __declspec (align (16)) CONTEXT
{
    u8 _padding0[48];

    u32 ContextFlags;

    u8 _padding1[100];

    u64 Rsp;
    u64 Rbp;

    u8 _padding2[80];

    u64 Rip;

    u8 _padding3[976];
};

struct ADDRESS64
{
    u64 Offset;
    u16 Segment;
    u32 Mode;
};

union STACKFRAME64
{
    char _total_size[264];
    struct
    {
        ADDRESS64 AddrPC;
        ADDRESS64 AddrReturn;
        ADDRESS64 AddrFrame;
        ADDRESS64 AddrStack;
    };
};

struct EXCEPTION_RECORD
{
    u32               ExceptionCode;
    u32               ExceptionFlags;
    EXCEPTION_RECORD *ExceptionRecord;
    void             *ExceptionAddress;
    u32               NumberParameters;
    u64               ExceptionInformation[15];
};

enum : u32
{
    EXCEPTION_ACCESS_VIOLATION = 3221225477,
    EXCEPTION_ARRAY_BOUNDS_EXCEEDED = 3221225612,
    EXCEPTION_BREAKPOINT = 2147483651,
    EXCEPTION_DATATYPE_MISALIGNMENT = 2147483650,
    EXCEPTION_FLT_INVALID_OPERATION = 3221225616,
    EXCEPTION_ILLEGAL_INSTRUCTION = 3221225501,
    EXCEPTION_IN_PAGE_ERROR = 3221225478,
    EXCEPTION_INT_DIVIDE_BY_ZERO = 3221225620,
    EXCEPTION_PRIV_INSTRUCTION = 3221225622,
    EXCEPTION_STACK_OVERFLOW = 3221225725,
};

struct EXCEPTION_POINTERS
{
    EXCEPTION_RECORD *ExceptionRecord;
    CONTEXT *ContextRecord;
};

enum : u32
{
    IMAGE_FILE_MACHINE_I386 = 0x014c,
    IMAGE_FILE_MACHINE_IA64 = 0x0200,
    IMAGE_FILE_MACHINE_AMD64 = 0x8664,
};

struct IMAGEHLP_SYMBOL64
{
    u32  SizeOfStruct;
    u64  Address;
    u32  Size;
    u32  Flags;
    u32  MaxNameLength;
    char Name[1];
};

struct IMAGEHLP_LINE64
{
    u32   SizeOfStruct;
    void *Key;
    u32   LineNumber;
    char *FileName;
    u64   Address;
};

extern "C"
{

typedef long (*LPTOP_LEVEL_EXCEPTION_FILTER) (EXCEPTION_POINTERS *);

typedef int (*PREAD_PROCESS_MEMORY_ROUTINE64) (void *, u64, void *, u32, u32 *);
typedef u64 (*PGET_MODULE_BASE_ROUTINE64) (void *, u64);
typedef u64 (*PTRANSLATE_ADDRESS_ROUTINE64) (void *, void *, ADDRESS64 *);
typedef void *(*PFUNCTION_TABLE_ACCESS_ROUTINE64) (void *, u64);

void *GetCurrentThread ();
void *SetUnhandledExceptionFilter (LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
int SymInitialize (void *hProcess, char *UserSearchPath, int fInvadeProcess);
u32 SymSetOptions (u32 SymOptions);
int StackWalk64 (
    u32 MachineType,
    void *hProcess,
    void *hThread,
    STACKFRAME64 *StackFrame,
    void *ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
);
void *SymFunctionTableAccess64 (void *hProcess, u64 AddrBase);
u64 SymGetModuleBase64 (void *hProcess, u64 dwAddr);
int SymGetSymFromAddr64 (void *hProcess, u64 Address, u64 *Displacement, void *Symbol);
int SymGetLineFromAddr64 (void *hProcess, u64 dwAddr, u32 *pdwDisplacement, IMAGEHLP_LINE64 *Line);
void RtlCaptureContext (CONTEXT *ContextRecord);
int SetThreadStackGuarantee (u32 *StackSizeInBytes);

}

// @Todo: nothing here is thread safe, have a mutex or something to synchronize

void print_stack_trace (CONTEXT *ctx, int skips)
{
    static const u32 MACHINE_TYPE = IMAGE_FILE_MACHINE_AMD64;
    static const int MAX_TRACE = 50000;

    // Putting this on the stack makes StackWalk64 fail at some point for some reason
    static STACKFRAME64 stack_frame;

    stack_frame.AddrPC.Offset = ctx->Rip;
    stack_frame.AddrFrame.Offset = ctx->Rbp;
    stack_frame.AddrStack.Offset = ctx->Rsp;

    stack_frame.AddrPC.Mode = 3;
    stack_frame.AddrFrame.Mode = 3;
    stack_frame.AddrStack.Mode = 3;

    void *thread = GetCurrentThread ();

    int recursion_depth = 0;
    String last_function_name = string_make ("");
    String last_filename = string_make ("");
    int last_line_number = 0;

    int i = 0;
    while (i < MAX_TRACE)
    {
        if (!StackWalk64 (MACHINE_TYPE, cast (void *) -1, thread, &stack_frame, ctx, null, &SymFunctionTableAccess64, &SymGetModuleBase64, null))
            break;

        if (i >= 0 && i <= skips)
        {
            i += 1;

            continue;
        }

        bool recursive_call = false;

        String filename = string_make ("unknown");
        int line = 0;
        u32 line_displacement = 0;
        IMAGEHLP_LINE64 imagehlp_line = {};
        imagehlp_line.SizeOfStruct = sizeof (imagehlp_line);

        if (SymGetLineFromAddr64 (cast (void *) -1, stack_frame.AddrPC.Offset, &line_displacement, &imagehlp_line))
        {
            filename = string_make (imagehlp_line.FileName);
            line = imagehlp_line.LineNumber;

            recursive_call = string_equals (filename, last_filename) && line == last_line_number;
            last_filename = filename;
            last_line_number = line;
        }
        else
        {
            last_filename = string_make ("");
            last_line_number = 0;
        }

        String func_name = string_make ("symbol not found");
        u64 symbol_displacement = 0;
        IMAGEHLP_SYMBOL64 symbol = {};
        symbol.SizeOfStruct = sizeof (symbol);
        symbol.MaxNameLength = 512;

        if (SymGetSymFromAddr64 (cast (void *) -1, stack_frame.AddrPC.Offset, &symbol_displacement, &symbol))
        {
            func_name = string_make (symbol.Name);

            recursive_call &= string_equals (func_name, last_function_name);
            last_function_name = func_name;
        }
        else
        {
            last_function_name = string_make ("");
        }

        if (recursion_depth > 0 && !recursive_call)
        {
            fprint_string (stderr, "\t... ");
            fprint_u64 (stderr, cast (u64) recursion_depth);
            fprint_string (stderr, " recursive call(s)\n");
            recursion_depth = 0;
        }

        if (recursive_call)
        {
            recursion_depth += 1;
            i += 1;

            continue;
        }

        fprint_string (stderr, "\t");
        fprint_string (stderr, func_name);

        static const int NAME_WIDTH = 32;
        int padding = NAME_WIDTH - func_name.count;
        for_range (i, 0, padding)
            fprint_string (stderr, " ");

        fprint_string (stderr, " ");
        fprint_string (stderr, filename);
        fprint_string (stderr, ":");
        fprint_u64 (stderr, cast (u64) line);
        fprint_string (stderr, "\n");

        if (string_equals (func_name, "main"))
            break;

        i += 1;
    }

    if (recursion_depth > 0)
    {
        fprint_string (stderr, "\t... at least ");
        fprint_u64 (stderr, cast (u64) recursion_depth);
        fprint_string (stderr, " recursive call(s)\n");
    }

    if (i >= MAX_TRACE)
    {
        fprint_string (stderr, "Stopping at ");
        fprint_u64 (stderr, MAX_TRACE);
        fprint_string (stderr, " function calls\n");
    }
}

bool g_symbols_initialized;

long handle_exception (EXCEPTION_POINTERS *info)
{
    fprint_string (stderr, "\n\033[" REPORT_FATAL_COLOR "mThe program crashed\033[0m");
    switch (info->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_IN_PAGE_ERROR: {
        u64 action = info->ExceptionRecord->ExceptionInformation[0];
        u64 addr = info->ExceptionRecord->ExceptionInformation[1];
        if (addr == 0)
        {
            if (action == 0)
                fprint_string (stderr, ": reading from a null pointer");
            else
                fprint_string (stderr, ": writing to a null pointer");
        }
        else
        {
            if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
                fprint_string (stderr, ": access violation");
            else
                fprint_string (stderr, ": page fault");

            switch (action)
            {
            case 0:
            case 1:
                if (action == 0)
                    fprint_string (stderr, " reading location 0x");
                else
                    fprint_string (stderr, " writing location 0x");

                fprint_u64 (stderr, addr, HEXADECIMAL_BASE);

                break;

            case 8:
                fprint_string (stderr, " (user-mode data execution prevention) at location 0x");
                fprint_u64 (stderr, addr, HEXADECIMAL_BASE);
                break;

            default:
                fprint_string (stderr, " at location 0x");
                fprint_u64 (stderr, addr, HEXADECIMAL_BASE);
                break;
            }
        }

    } break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        fprint_string (stderr, ": array bounds exceeded");
        break;
    case EXCEPTION_BREAKPOINT:
        fprint_string (stderr, ": a breakpoint was hit");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        fprint_string (stderr, ": datatype misalignment");
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        fprint_string (stderr, ": invalid floating point operation");
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        fprint_string (stderr, ": illegal instruction");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        fprint_string (stderr, ": integer division by zero");
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        fprint_string (stderr, ": unauthorized instruction");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        fprint_string (stderr, ": stack overflow");
        break;
    }

    fprint_string (stderr, "\n");

    static CONTEXT context;

    if (g_symbols_initialized)
    {
        RtlCaptureContext (&context);
        fprint_string (stderr, "Stack trace:\n");
        print_stack_trace (&context, 6);
    }
    else
    {
        fprint_string (stderr, "Failed to initialize symbols, cannot print stack trace\n");
    }

    exit (2);

    return 1;
}

void crash_handler_init ()
{
    // @Todo: these should be static assertions
    assert (sizeof (STACKFRAME64) == 264);
    assert (sizeof (CONTEXT) == 1232);

    SetUnhandledExceptionFilter (handle_exception);

    if (!SymSetOptions (0x216))
        return;

    if (!SymInitialize (cast (void *) -1, null, true))
        return;

    // Reserve 25k bytes of the total available stack so we have memory
    // when a stack overflow occurs (i.e. for printing the stack trace)
    u32 guaranteed_stack_size = 25000;
    SetThreadStackGuarantee (&guaranteed_stack_size);

    g_symbols_initialized = true;
}
