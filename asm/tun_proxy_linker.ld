/*
 * Linker script fragment used to keep low-level ASM routines grouped near .text.
 * This is intentionally minimal so normal system linker behavior remains intact.
 */
SECTIONS
{
  .tunproxy_asm :
  {
    KEEP(*(.text.packet_checksum32))
  }
}
INSERT AFTER .text;
