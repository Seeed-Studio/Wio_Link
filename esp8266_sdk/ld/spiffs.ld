
/* For 4MByte Flash */
/* 1MB user1 + 1MB user2 */
/* spiffs 2028KB */
/* eeprom 4K + system para 16K = 20KB */
PROVIDE ( _SPIFFS_start = 0x40400000 );
PROVIDE ( _SPIFFS_end = 0x405FB000 );


/* For 2MByte Flash */
/* 1MB user1 + 1MB user2 */
/* spiffs 492KB at the end of user2 before 20K eeprom+syspara*/
/* eeprom 4K + system para 16K = 20KB 
PROVIDE ( _SPIFFS_start = 0x40380000 );
PROVIDE ( _SPIFFS_end = 0x403F0000 );*/
