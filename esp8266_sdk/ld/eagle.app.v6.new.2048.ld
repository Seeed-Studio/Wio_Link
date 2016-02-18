/* flash address starts from 0x40200000 */
/* 0x1000 reserved for boot, and 0x10 reserved for rom flags */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xE0000
}

PHDRS
{
  dport0_0_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  dram0_0_bss_phdr PT_LOAD;
  iram1_0_phdr PT_LOAD;
  irom0_0_phdr PT_LOAD;
}

INCLUDE "../ld/eagle.app.v6.new.sections.ld"


/* For 4MByte Flash */
/*       user1.bin         user2.bin             spiffs        eep  system para */
/* |------------------+--------------------|-----------------|----|----|        */
/*         1024k            1024k                2028k         4k   16k         */
PROVIDE ( _SPIFFS_start = 0x40400000 );
PROVIDE ( _SPIFFS_end = 0x405FB000 );



