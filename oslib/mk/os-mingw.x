/* The link script (Note: the format should be "pe-i386" instead of
   "pei-i386" to ged rid of the "MZ" header.
   and text and data sections should be close to each other.
*/
OUTPUT_FORMAT("pe-i386")
ENTRY(start)
SECTIONS
{
 .text          :
 {
        *(.text)
 }
 .data          ADDR(.text) + SIZEOF(.text) :
 {
        *(.data)
        *(.data2)
        *(.rdata)
        __data_end__ = . ;
 }
 .bss           ADDR(.data) + SIZEOF(.data) :
 {
        *(.bss)
        *(COMMON)
        __bss_end__ = . ;
 }
}

