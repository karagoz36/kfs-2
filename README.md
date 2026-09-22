# kfs-2 — GDT & Stack

Second step of the 42 *Kernel From Scratch* series, built on top of kfs-1.
The kernel is an i386 multiboot ELF booted by GRUB, written in C and NASM.

## Build and run

```sh
make            # builds kfs.iso
make run        # boots it in QEMU (KVM=1 make run to enable KVM)
make check      # verifies the multiboot header and the 10 MB limit
```

Requirements: gcc (32-bit target), nasm, ld, grub-mkrescue, xorriso, qemu-system-i386.

On macOS there is no native toolchain: `make docker` builds the ISO inside a
Linux container (see [Dockerfile](Dockerfile)), then `make run` boots it with
the host QEMU. `make check` also works there: `grub-file` does not exist on
macOS, so the multiboot check runs in the container. `make docker-shell`
opens a shell in it.

## What kfs-2 adds

### Global Descriptor Table at 0x800

The CPU cannot run protected-mode code without a GDT. GRUB provides a
temporary one; `gdt_init()` in [kernel/gdt.c](kernel/gdt.c) writes our own
table at physical address `0x00000800` and `gdt_flush` in
[boot/gdt_flush.asm](boot/gdt_flush.asm) loads it with `lgdt`, then reloads
CS (far jump), DS, ES, FS, GS and SS.

The table is written at run time because GRUB refuses to load ELF segments
below 1 MB, so it cannot be placed there by the linker script.

| Selector | Segment      | Base | Limit  | Access | Flags |
|----------|--------------|------|--------|--------|-------|
| 0x00     | null         | 0    | 0      | 0x00   | 0x0   |
| 0x08     | kernel code  | 0    | 4 GB   | 0x9A   | 0xC   |
| 0x10     | kernel data  | 0    | 4 GB   | 0x92   | 0xC   |
| 0x18     | kernel stack | 0    | 4 GB   | 0x92   | 0xC   |
| 0x20     | user code    | 0    | 4 GB   | 0xFA   | 0xC   |
| 0x28     | user data    | 0    | 4 GB   | 0xF2   | 0xC   |
| 0x30     | user stack   | 0    | 4 GB   | 0xF2   | 0xC   |

Every segment is flat (base 0, limit 4 GB with 4 KB granularity, 32-bit).
The stack segments are plain writable data segments: the expand-down bit
combined with a full limit would reject every offset on real hardware.

When the table is read back (`gdt` command or `xp` in QEMU) the segments in
use show access `0x93` rather than `0x92`: bit 0 is the *accessed* flag,
set by the CPU itself when a segment register is loaded.

### Kernel stack

The 16 KB stack is reserved in `.bss` by [boot/boot.asm](boot/boot.asm)
(`stack_bottom` .. `stack_top`). `_start` sets `esp` to `stack_top` and
`ebp` to 0, and `gdt_flush` makes SS point at the kernel stack selector.

### Stack printer

`print_kernel_stack()` in [kernel/stack.c](kernel/stack.c) prints:

- the stack bounds, how many bytes are in use, `esp` and `ebp`;
- a hexdump of the bytes in use (address, four 32-bit words, ASCII);
- the chain of call frames (saved `ebp` and return address), which works
  because the kernel is compiled with `-fno-omit-frame-pointer` and `_start`
  zeroes `ebp` to mark the end of the chain.

`printk` gained zero-padded widths (`%08x`) to align the columns.

### Compiler flag worth knowing

`-mgeneral-regs-only` is required: on an x86-64 host, `gcc -m32` still
enables SSE2 and vectorises loops with `xmm` registers. SSE is disabled at
boot (`CR4.OSFXSR = 0`), so the first such instruction raises an invalid
opcode exception, which becomes a triple fault and a reboot loop without
an IDT.

### Debug shell (bonus)

Characters read from the PS/2 keyboard are fed to
[kernel/shell.c](kernel/shell.c). Commands:

| Command  | Effect                              |
|----------|-------------------------------------|
| `help`   | list the commands                   |
| `stack`  | print the kernel stack              |
| `gdt`    | print the 7 descriptors at 0x800    |
| `clear`  | clear the screen                    |
| `reboot` | pulse the CPU reset line (port 0x64)|
| `halt`   | `cli` + `hlt` forever               |

Alt+1..4 still switches between the four virtual screens from kfs-1.

## Checking the GDT from the QEMU monitor

```
(qemu) info registers      # CS=0008, SS=0018, GDT= 00000800 00000037
(qemu) xp /14wx 0x800      # the seven 8-byte descriptors
```

## Layout

```
boot/      boot.asm (multiboot header, stack, _start), gdt_flush.asm
kernel/    main.c, gdt.c, stack.c, shell.c, console.c, printk.c
drivers/   vga.c, keyboard.c
lib/       string.c
include/   headers
linker.ld  our own linker script (kernel loaded at 1 MB)
grub/      grub.cfg copied into the ISO
```
