OUTPUT_FORMAT("coff-go32")
ENTRY(start)
SECTIONS
{
	.text :
	{
		*(.text)
		etext  =  . ; _etext = .;
	}
	.data :
	{
	    djgpp_first_ctor = . ;
		*(.ctor)
	    djgpp_last_ctor = . ;
	    djgpp_first_dtor = . ;
		*(.dtor)
	    djgpp_last_dtor = . ;
		*(.data)
		edata  =  . ; _edata = .;
	}
	.bss  SIZEOF(.data) + ADDR(.data) :
	{
		*(.bss)
		*(COMMON)
		end = . ; _end = .;
	}
}
