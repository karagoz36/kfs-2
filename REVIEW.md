# KFS-2 review guide

Everything added on top of KFS-1 for this subject, the concepts behind it,
and the functions to be able to explain line by line at the defense.

## 1. What changed (diff summary)

| File | Status | What it does |
|------|--------|--------------|
| `include/gdt.h` | new | Descriptor structure, entry count, `gdt_init` / `gdt_print` API |
| `kernel/gdt.c` | new | Builds the 7 descriptors at 0x800, loads them, prints them |
| `boot/gdt_flush.asm` | new | `lgdt` + far jump + segment register reload |
| `include/stack.h`, `kernel/stack.c` | new | Human-friendly kernel stack dump and call-frame walk |
| `include/shell.h`, `kernel/shell.c` | new | Minimal debug shell (bonus) |
| `linker.ld` | modified | Defines the symbol `gdt_table = 0x800` |
| `boot/boot.asm` | modified | Exports `stack_bottom` / `stack_top`, zeroes `ebp` |
| `kernel/main.c` | modified | Calls `gdt_init` first, draws every screen, feeds the shell |
| `drivers/keyboard.c`, `include/keyboard.h` | modified | `keyboard_poll` returns the char instead of printing it; `keyboard_reboot` added |
| `kernel/printk.c`, `include/printk.h` | modified | Zero-padded widths (`%08x`) for aligned columns |
| `Makefile` | modified | New sources, `-fno-omit-frame-pointer`, `-mgeneral-regs-only`, GRUB `part_*` modules, `-boot d` |
| `grub/grub.cfg` | modified | Menu entry renamed |
| `README.md` | modified | Build, GDT layout, verification procedure |

Roughly 660 lines added, 60 removed. KFS-1 code (console, vga, string) is
untouched apart from the two additive changes listed above.

## 2. Concepts to master

### Segmentation and the GDT

- **Segment**: a region of memory described by a base, a limit and access
  rights. In protected mode every memory access goes through a segment
  register (CS for code, DS/ES/FS/GS for data, SS for the stack).
- **GDT (Global Descriptor Table)**: the array of 8-byte descriptors the CPU
  consults. The CPU cannot run protected-mode code without one. GRUB installs
  a temporary GDT; the subject asks us to replace it with ours at 0x800.
- **Descriptor layout** (8 bytes, historical order from the 80286):
  limit 0..15, base 0..15, base 16..23, access byte, limit 16..19 + flags
  nibble, base 24..31. `__attribute__((packed))` keeps the struct exactly
  8 bytes.
- **Access byte** bits: present (0x80), privilege level DPL (0x00 ring 0,
  0x60 ring 3), descriptor type (0x10 = code/data), executable (0x08),
  readable/writable (0x02). Hence 0x9A kernel code, 0x92 kernel data,
  0xFA user code, 0xF2 user data.
- **Flags nibble**: G = 1 (limit in 4 KB pages), D/B = 1 (32-bit). 0xC.
  With limit 0xFFFFF and G = 1 the segment covers 4 GB.
- **Flat model**: every segment has base 0 and limit 4 GB. Segmentation is
  not used to isolate memory (paging will, in later subjects); the table
  exists because the CPU requires it and because rings live here.
- **Selector**: the value loaded into a segment register. It is the byte
  offset of the entry in the table, `index * 8`. So 0x08 kernel code, 0x10
  kernel data, 0x18 kernel stack, 0x20 user code, 0x28 user data, 0x30 user
  stack. The low 2 bits are the requested privilege level (0 for us).
- **Null descriptor**: entry 0 must be all zeros. Loading selector 0 into a
  data register means "no segment"; using it faults.
- **Accessed bit**: bit 0 of the access byte is set by the CPU itself the
  first time a selector is loaded. That is why `gdt` shows 0x93 for the
  segments in use and 0x92 in the source.
- **Expand-down**: bit 0x04 makes a data segment grow downwards, meant for
  stacks. With a full 4 GB limit it would reject every offset on real
  hardware (QEMU does not check limits and hides the mistake). A plain
  writable segment in its own entry is correct and simpler.
- **lgdt**: loads the 6-byte GDTR (limit = size - 1, base address). The CPU
  keeps a copy, so the pointer can be a local variable.
- **Reloading segment registers**: after `lgdt` the registers still hold
  GRUB's selectors. DS/ES/FS/GS/SS are reloaded with `mov`. CS cannot be
  written with `mov`: a **far jump** `jmp 0x08:label` reloads it.
- **"Declare the GDT to the BIOS"**: the subject's wording for `lgdt`.
  The BIOS is not involved after GRUB; the CPU is what reads the table.
- **Why the table is written at run time**: GRUB refuses to load ELF
  segments below 1 MB, so the linker cannot place data at 0x800. The linker
  script only defines the *symbol* `gdt_table = 0x800`; `gdt_init` fills
  the memory. GCC also refuses a constant pointer to 0x800 with `-Werror`
  (`-Warray-bounds`), the linker symbol avoids that.

### The stack

- Grows **downwards**: `push` decrements `esp` then writes. `esp` points at
  the most recent value, everything from `esp` up to `stack_top` is in use.
- 16 KB reserved in `.bss` by `boot.asm` (`stack_bottom` .. `stack_top`),
  16-byte aligned as the System V i386 ABI requires. `_start` sets `esp`
  to `stack_top` because the multiboot spec does not guarantee `esp`.
- **Frame pointer**: every function prologue does `push ebp; mov ebp, esp`.
  `[ebp]` is the caller's saved `ebp`, `[ebp+4]` is the return address.
  Following the chain of saved `ebp` values walks the callers (a
  backtrace). GCC drops this at `-O2` unless `-fno-omit-frame-pointer`.
- `_start` zeroes `ebp` so the walk stops at 0 (outside the stack bounds).
- Reading registers from C: `__builtin_frame_address(0)` gives `ebp`;
  `esp` needs one line of inline assembly.

### Build and boot

- **`-mgeneral-regs-only`**: on an x86-64 host `gcc -m32` still enables
  SSE2 and vectorises loops with `xmm` registers. SSE is disabled at boot
  (`CR4.OSFXSR = 0`), so the first SSE instruction raises #UD (invalid
  opcode). Without an IDT that becomes a double then triple fault, and the
  machine resets: a reboot loop. Found with `qemu -d int -no-reboot`.
- **GRUB modules**: the ISO only ships the modules it needs. GRUB's `normal`
  module tries to autoload every partition-map module listed in its index
  and prints an error for each missing one, hence the `part_*` list.
- **`-boot d`**: QEMU's default order tries the hard disk and the floppy
  before the CD, which fail and slow the boot.
- **Reboot**: writing 0xFE to the PS/2 controller command port 0x64 pulses
  the CPU reset line (pre-ACPI method).
- **Halt**: `cli` then `hlt` in a loop. With interrupts disabled `hlt` never
  wakes up, which is also why the main loop polls instead of halting.

## 3. File by file, functions to know

### `linker.ld`

- `gdt_table = 0x00000800;` before the sections: an absolute symbol, not a
  section. Be ready to explain why it is not a section (GRUB, 1 MB).

### `include/gdt.h`

- `t_gdt_entry`: the packed 8-byte descriptor, field by field.
- `GDT_ENTRIES` (7) and `extern t_gdt_entry gdt_table[GDT_ENTRIES]`.

### `kernel/gdt.c`

- `ACC_*` bits, `ACCESS_CODE(ring)`, `ACCESS_DATA(ring)`: rebuild 0x9A,
  0x92, 0xFA, 0xF2 from the bits on paper.
- `FLAGS_32BIT_4K` (0xC), `LIMIT_4GB` (0xFFFFF).
- `t_gdt_ptr`: the 6-byte GDTR image.
- `g_segments[]`: name + access byte for the 7 entries, the single source
  of the table layout; also used by `gdt_print`.
- `gdt_set_entry()`: how base and limit are split across the fields.
- `gdt_init()`: null entry, loop over the six segments, GDTR fill,
  `gdt_flush(&ptr)`.
- `gdt_print()`: reassembles base and limit from the fields; explains the
  0x93 accessed bit.

### `boot/gdt_flush.asm`

- `mov eax, [esp + 4]`: first C argument in the cdecl convention.
- `lgdt [eax]`, `jmp KERNEL_CODE:.reload_segments`, the five `mov` into
  DS/ES/FS/GS then SS with the stack selector, `ret`.
- Why CS needs a jump and the others a `mov`.

### `boot/boot.asm`

- The `.bss` stack, `global stack_bottom` / `stack_top`.
- `mov esp, stack_top` then `xor ebp, ebp`, and why.
- `kernel_main` is called *after* the stack exists; `gdt_init` is the first
  thing it does.

### `kernel/stack.c`

- `dump_line()`: one line = address, four 32-bit words, ASCII column.
- `print_frames()`: the saved-`ebp` walk, its bounds check
  (`stack_bottom <= frame` and `frame + 8 <= stack_top`) and the depth cap.
- `print_kernel_stack()`: reads `esp` and `ebp`, prints the bounds and
  bytes in use, aligns the start address down to 16, caps the lines, then
  the frames. Know how to read one line of the output and name what the
  three frames are (shell → kernel_main → _start).

### `kernel/shell.c`

- `t_command` and `g_commands[]`: name, help, function pointer. Note that
  `print_kernel_stack`, `gdt_print`, `console_clear`, `keyboard_reboot`
  are used directly as entries, no wrappers.
- `shell_input()`: Enter runs the line, backspace edits, printable chars
  are stored and echoed. The 64-byte line buffer and its bound.
- `run_line()`: trims spaces, empty line does nothing, `k_strcmp` lookup,
  quoted error message.
- `cmd_halt()`: `cli` + `hlt` loop.

### `drivers/keyboard.c`

- `keyboard_poll()` now returns the character (0 when nothing printable):
  the driver no longer echoes, the shell owns the echo.
- `keyboard_reboot()`: `outb(0x64, 0xFE)`.

### `kernel/printk.c`

- The `%0N` width parsing and `print_uint(value, base, width)` padding.
  Used by `%08x` in the stack dump and the GDT table.

### `kernel/main.c`

- `kernel_main()`: `gdt_init` → `console_init` → `draw_screens` →
  polling loop feeding `shell_input`.
- `draw_screen()` / `draw_screens()`: header, banner and prompt on each of
  the four virtual screens.

### `Makefile`

- The four new flags/entries and the reason for each (see section 2).
- `make check`: multiboot header and the 10 MB limit.

## 4. Verification procedure (do it live at the defense)

```sh
make re && make check
make run
```

In the QEMU window: `gdt`, `stack`, `help`, `halt`, `reboot`.

From the QEMU monitor (`-monitor stdio` or Ctrl+Alt+2 in the window):

```
(qemu) info registers      # CS=0008  DS=ES=FS=GS=0010  SS=0018  GDT=00000800 00000037
(qemu) xp /14wx 0x800      # the 7 descriptors, 2 words each
```

Expected words at 0x800:

```
00000000 00000000   null
0000ffff 00cf9a00   kernel code   (0x9b once accessed)
0000ffff 00cf9200   kernel data   (0x93 once accessed)
0000ffff 00cf9200   kernel stack  (0x93 once accessed)
0000ffff 00cffa00   user code
0000ffff 00cff200   user data
0000ffff 00cff200   user stack
```

## 5. Questions to expect

- *Why 0x800 and not a section in the linker script?* GRUB will not load
  below 1 MB; the symbol names the address, the code fills it.
- *What does `lgdt` take?* A 6-byte structure: 16-bit limit (size - 1) and
  32-bit base.
- *Why a far jump?* CS can only be changed by a control transfer.
- *Why is access 0x93 and not 0x92?* The CPU sets the accessed bit.
- *Why not use the expand-down bit for the stack segments?* With a 4 GB
  limit it invalidates every offset on real hardware.
- *How does the backtrace work?* Saved `ebp` chain, needs
  `-fno-omit-frame-pointer`, ends at the 0 written by `_start`.
- *What is the difference between `esp` and `ebp`?* `esp` moves with every
  push/pop; `ebp` is fixed for the duration of a function and anchors its
  frame.
- *What would happen without `-mgeneral-regs-only`?* Invalid opcode on the
  first SSE instruction, triple fault, reboot loop.
- *Why does the main loop not `hlt`?* Interrupts are off and there is no
  IDT, `hlt` would never return.
- *User segments exist, are they used?* Not yet: no user mode until a TSS
  and an IDT exist. They are defined so the table is complete.
