#ifndef MK1MF_BUILD
    /* auto-generated by util/mkbuildinf.pl for crypto/cversion.c */
    #define CFLAGS cflags
    /*
     * Generate CFLAGS as an array of individual characters. This is a
     * workaround for the situation where CFLAGS gets too long for a C90 string
     * literal
     */
    static const char cflags[] = {
        'c','o','m','p','i','l','e','r',':',' ','g','c','c',' ','-','I','.',' ',
        '-','I','.','.',' ','-','I','.','.','/','i','n','c','l','u','d','e',' ',
        ' ','-','f','P','I','C',' ','-','D','O','P','E','N','S','S','L','_','P',
        'I','C',' ','-','D','O','P','E','N','S','S','L','_','T','H','R','E','A',
        'D','S',' ','-','D','_','R','E','E','N','T','R','A','N','T',' ','-','D',
        'D','S','O','_','D','L','F','C','N',' ','-','D','H','A','V','E','_','D',
        'L','F','C','N','_','H',' ','-','D','L','_','E','N','D','I','A','N',' ',
        '-','O','3',' ','-','f','o','m','i','t','-','f','r','a','m','e','-','p',
        'o','i','n','t','e','r',' ','-','W','a','l','l','\0'
    };
    #define PLATFORM "platform: linux-elf"
    #define DATE "built on: Wed Jul  6 00:25:16 2016"
#endif
