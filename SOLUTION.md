# LordUrsula — Solution Overview

## Goal

Inject `TargetDLL.dll` into a running `ConsoleAppTarget.exe` process at runtime, then patch the target's machine code so its call to `sum(int, int)` is redirected to `Hack()` from the injected DLL. The mechanism is a **trampoline hook**: overwrite the original `call sum` instruction with a `call <code cave>`, where the code cave holds a `call Hack` instruction written into unused null bytes in the target's `.text` section.

---

## The Three Projects

### `ConsoleAppTarget.exe`
The victim process. Prompts the user for two integers, calls `sum(int, int)`, and prints the result.

`sum` is marked `__declspec(noinline)`. Without this, the compiler would inline the function body directly into `main`, eliminating the `call` instruction entirely — there would be nothing to patch. `noinline` guarantees a real call site exists in the compiled `.text` section.

### `TargetDLL.dll`
The injected payload. Exports a single function `Hack()` which prints `"You have been hacked!"` to stdout.

`Hack` is declared `extern "C"`. Without this, C++ name mangling would turn `Hack` into something like `?Hack@@YAXXZ` in the DLL's export table, making it unpredictable to locate. `extern "C"` keeps the exported symbol name a plain, undecorated `Hack`.

### `LordUrsula.exe`
The injector and patcher. Finds the target process, injects the DLL, reads both modules' memory, parses their PE structure, locates the code cave and call instruction, and builds the trampoline. This is where all the work happens.

---

## LordUrsula File Layout

All source files are flat in the project root. Logical grouping is via Visual Studio filters, namespaces, and file naming conventions.

```
LordUrsula/
├── LordUrsula.cpp               # main() — entry point and test harness
├── pch.h / pch.cpp              # Precompiled header (Windows.h, STL)

Models/
├── Module.h                     # Owns raw memory bytes + remote base address
├── Trampoline.h                 # Data holder: what bytes to write and where
├── PortableExecutable.h/.cpp    # Parses a Module as a Windows PE binary

Managers/
├── IProcessManager.h            # Interface for all process memory operations
├── ProcessManager.h/.cpp        # Concrete: wraps a live Windows process handle
├── IModuleManager.h             # Interface for loaded module/DLL operations
├── ModuleManager.h/.cpp         # Concrete: wraps GetModuleHandle/GetProcAddress

Builders/
├── ICodeBuilder.h               # Interface for code manipulation
├── CodeBuilder.h/.cpp           # Concrete: finds code caves, builds trampolines

Helpers/
├── SnapshotHelper.h             # Templated TlHelp32 snapshot reader (C++20 concepts)
├── CoutHelper.h                 # Hex dump printer to stdout

Exceptions/
├── Exception.h                  # Base wide-string exception with std::format support
├── WindowsException.h           # Captures GetLastError() on construction
```

---

## Architecture Principles

### Interface-based dependency injection (manual)
Every manager and builder has a pure abstract interface (`IProcessManager`, `IModuleManager`, `ICodeBuilder`). Concrete classes implement those interfaces. `main()` constructs the concretes and passes them around as interface references.

Why: it decouples callers from implementation details. `CodeBuilder` doesn't care how memory is read — it only knows `IProcessManager`. This makes each piece independently testable and swappable.

### Move-only models
`Module` and `PortableExecutable` delete their copy constructors and copy assignment operators. Only move is allowed.

Why: both types own a large heap buffer (the raw bytes of a remote module). Allowing copies would silently duplicate that buffer, creating two owners of what should be a single resource. Deleting copy forces callers to be explicit about ownership transfer.

### `std::span` at memory boundaries
Functions that read or write raw bytes take `std::span<const std::byte>` (read) or `std::span<std::byte>` (write), never raw pointers.

Why: a raw pointer carries no size information — the callee has no way to bounds-check without an accompanying length parameter. `std::span` bundles pointer and length together in a single type, making the interface self-describing and safer.

### RAII resource management
`ProcessManager` opens a Windows process handle in its constructor and closes it in its destructor. The handle never escapes the class.

Why: if an exception is thrown after the handle is opened but before it would be manually closed, the destructor still runs and the handle is released. Manual open/close pairs are fragile; RAII makes cleanup automatic and exception-safe.

### C++20 concepts in `SnapshotHelper`
`SnapshotHelper` is a template that works with both `PROCESSENTRY32` and `MODULEENTRY32`. A C++20 concept constrains the template to only those two types, and `if constexpr` dispatches to the correct Win32 API at compile time.

Why: the alternative is two separate functions that are nearly identical. The concept approach eliminates the duplication while keeping the type safety — if you pass the wrong type, you get a clear compile error, not a runtime failure.

---

## Domain Objects

### `Models::Module`
Owns a `std::vector<std::byte>` — the raw bytes of a module as read from a remote process via `ReadProcessMemory`. Also stores the `uintptr_t` base address at which that module lives in the remote process's address space.

The base address matters because all pointer values inside the PE headers are relative to where the module is loaded. When you read the bytes locally but need to reason about remote addresses, you need to know where the module actually sits in the target.

### `Models::PortableExecutable`
Takes ownership of a `Module` and parses it as a Windows PE binary. Exposes typed pointers (`IMAGE_DOS_HEADER*`, `IMAGE_FILE_HEADER*`, `IMAGE_OPTIONAL_HEADER64*`, section headers) and named sections as `std::span<std::byte>` (`.text`, `.pdata`).

All those pointers and spans are non-owning views into the single buffer owned by `Module`. Nothing is copied — it is all pointer arithmetic into the same memory. If the `Module` is destroyed, all the PE pointers become dangling. Only supports 64-bit PE (PE32+); throws on 32-bit.

### `Models::Trampoline`
A plain data struct. Holds two things:
1. The replacement bytes for the original call site, and the address in the remote process where they should be written.
2. The trampoline bytes for the code cave, and the address where those should be written.

It carries no logic — it is just the description of two write operations. `CodeBuilder` produces it; `ProcessManager::WriteMemory` will consume it.

---

## Services

### `Managers::ProcessManager`
The core Windows interop layer. Everything that touches a live process goes through here.

- Finds the target process by name using a TlHelp32 process snapshot.
- Opens a handle with `PROCESS_ALL_ACCESS`.
- Injects a DLL via the classic `CreateRemoteThread(LoadLibraryW)` technique: allocates memory in the remote process, writes the DLL path, creates a remote thread that calls `LoadLibraryW`, and waits for it to complete.
- Reads a named module's full bytes out of the remote process into a local `Module`.
- Exposes `ReadMemory`, `WriteMemory`, and `AllocateVirtualMemory` as primitive operations for the patcher to use.

### `Managers::ModuleManager`
Resolves function addresses from a DLL loaded in the *current* process. Used specifically to get the real address of `LoadLibraryW` from `kernel32.dll`.

This works for injection because `kernel32.dll` is always loaded at the same base address in every process on the same Windows session (it is a system DLL that ASLR does not relocate between processes). So the `LoadLibraryW` address in the injector process is the same address in the target process.

### `Builders::CodeBuilder`
Owns the patching logic. Takes a `const PortableExecutable&` reference to `ConsoleAppTarget`'s parsed PE.

- `FindCodeCave(size)`: scans the `.text` section for a contiguous run of `size` null bytes. Null bytes in `.text` are compiler-generated padding — guaranteed to be executable memory that isn't doing anything.
- `CreateTrampoline(targetFunction)`: locates the `E8` CALL instruction for the target function, computes a new relative offset pointing to the code cave, and constructs the byte sequences that need to be written.

---

## The Trampoline Mechanism (Conceptual)

A `call` instruction in x64 is 5 bytes: `E8` followed by a 4-byte signed relative offset. The offset is relative to the address of the *next* instruction (i.e. `call address + 5`).

The patch involves two writes:

1. **Code cave write**: Place a `call Hack` instruction into the null-byte padding area of `.text`. This is the trampoline.

2. **Call site overwrite**: Replace the original `call sum` bytes with `call <code cave address>`. When `main` reaches this instruction, instead of calling `sum`, it calls the code cave, which calls `Hack`.

`sum` is never called. `Hack` runs in its place. The process never knew anything changed.

---

## Current State

| Area | Status |
|---|---|
| DLL injection via `LoadLibraryW` | Working |
| PE parsing (`PortableExecutable`) | Working |
| Code cave search (`FindCodeCave`) | Working |
| `CreateTrampoline` byte construction | Stubbed — returns empty `Trampoline{}` |
| Export table parsing (to locate `Hack`) | Not implemented (TODO in `main`) |
| Writing trampoline back to target process | Not wired up yet |

The next concrete steps are:
1. Parse `TargetDLL.dll`'s export table to get the RVA of `Hack`, convert it to a remote address.
2. Complete `CreateTrampoline` to build the correct replacement bytes and populate `Trampoline`.
3. Call `WriteMemory` twice with the two entries from `Trampoline` to apply the patch.

---

## Conventions

- **Private members**: `_camelCase` (e.g. `_processHandle`, `_moduleName`)
- **Wide strings**: all Win32-facing strings are `std::wstring` / `L""` literals
- **No raw `new`/`delete`**: heap allocation via `std::vector`, `std::make_unique`, or Win32 APIs
- **`const` correctness**: methods that do not mutate state are marked `const`
- **Exception formatting**: both `Exception` and `WindowsException` accept `std::wformat_string` + variadic args — use `{0}`, `{1}` positional placeholders
- **`std::bit_cast`** for reinterpreting raw bytes — preferred over `reinterpret_cast` in C++20
