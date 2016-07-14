#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "bytes.h"

/* convert ["AB", "CD", "EF", "12", ...] -> "\xAB\xCD\xEF\x12..." */
int parse_byte_list(char **str_bytes, unsigned int n_bytes, uint8_t *result)
{
    int rc = -1;
    int i = 0;

    while(i < n_bytes) {
        if(parse_uint8_hex(str_bytes[i], result+i)) {
            goto cleanup;
        }

        i++;
    }

    rc = 0;

    cleanup:
    return rc;
}

/* convert "DEADBEEF LBABE"    or 
           "DEADBEEF DEAE79AC" -> [0xDEADBEEF, 0xDEAE79AC] */
int parse_addr_range(int ac, char **av, uintptr_t *addr, unsigned int *len)
{
    int rc = -1;

    if(ac >= 3) {
        if(!parse_uintptr_hex(av[2], addr)) {
            rc = 0;
        }
    }

    if(ac >= 4) {
        /* allow 'L' prefix (windbg length nomenclature) */
        if(av[3][0]=='L') {
            rc = parse_uint32_hex(av[3]+1, len);
        }
        /* otherwise this parameter is an end address and
            we calculate a length manually */
        else {
            rc = -1;

            if(!parse_uint32_hex(av[3], len)) {
                if(*len > *addr) {
                    *len = *len - *addr;
                    rc = 0;
                }
            }
        }
    }
    else {
        *len = 4;
    }

    //printf("parsed address: 0x%p\n", *addr);
    //printf("parsed length: 0x%08X\n", *len);

    return rc;
}

/* convert "DEADBEEF AB CD EF ..." -> [0xDEADBEEF, "\xAB\xCD\xEF..."] */
int parse_addr_bytelist(int ac, char **av, uintptr_t *addr, uint8_t *bytes)
{
    int rc = -1;
    int i;

    if(ac >= 3) {
        if(parse_uintptr_hex(av[2], addr)) {
            goto cleanup;
        }
    }

    if(ac >= 4 && !parse_byte_list(av+3, ac-3, bytes)) {
        rc = 0;

        //printf("parsed address: 0x%X\n", *addr);
        //printf("parsed bytes: ");
        for(i=0; i<(ac-3); ++i) {
            printf("%02X ", bytes[i] & 0xFF);
        }
        printf("\n");
    }

    cleanup:
    return rc;
}

/* this parses commands like search, which take form:
    search <start_address> <end_address> <0_or_more_bytes>
*/
int parse_addr_range_bytelist(int ac, char **av, uintptr_t *addr, unsigned int *len, uint8_t *bytes)
{
    int rc = -1;
    int i;

    if(parse_addr_range(ac, av, addr, len)) {
        goto cleanup;
    }

    if(parse_byte_list(av+4, ac-4, bytes)) {
        goto cleanup;
    }

    rc = 0;
    cleanup:
    return rc;
}

/* */
int
parse_values_array_hex(char **strings, int len, uint8_t *data, unsigned int *len_data)
{
    int rc=-1;
    int i, len_string;

    /* get the data parts */
    for(*len_data=0, i=0; i<len; ++i) {
        len_string = strlen(strings[i]);

        if(len_string <= 2 || (len_string <= 4 && strings[i][1] == 'x')) {
            if(parse_uint8_hex(strings[i], data + *len_data) < 0) {
                goto cleanup;
            }

            *len_data += 1;
        }
        else if(len_string <= 4 || (len_string <= 8 && strings[i][1] == 'x')) {
            if(parse_uint16_hex(strings[i], (uint16_t *) (data + *len_data))) {
                goto cleanup;
            }

            *len_data += 2;
        }
        else if(len_string <= 8 || (len_string <= 16 && strings[i][1] == 'x')) {
            if(parse_uint32_hex(strings[i], (uint32_t *) (data + *len_data))) {
                goto cleanup;
            }

            *len_data += 4;
        }
        else {
            if(parse_uint64_hex(strings[i], (uint64_t *) (data + *len_data))) {
                goto cleanup;
            }

            *len_data += 8;
        }
    }
   
    /* made it this far? success */
    rc = 0;

    cleanup:

    return rc;
}

int 
parse_mac(char *mac, unsigned char *bytes)
{
    int i;
    int rc = -1;
    int loc_seps[5] = {2,5,8,11,14};
    int loc_bytes[6] = {0,3,6,9,12,15};

    if(strlen(mac) != (2*6 + 5)) {
        /* note: zu is C99 for size_t */
        printf("ERROR: expected 17, but strlen(mac)==%zu\n", strlen(mac));
        goto cleanup;
    }

    for(i=0; i<5; ++i) {
        char *x = mac + loc_seps[i];

        if(*x != ':') {
            printf("ERROR: mac[%d] was not a colon\n", loc_seps[i]);
            goto cleanup;
        }

        *x = '\0';
    }
   
    for(i=0; i<6; ++i) {
        if(parse_uint8_hex(mac + loc_bytes[i], bytes+i)) {
            printf("ERROR: mac[%d] was not a valid byte\n", loc_bytes[i]);
            goto cleanup;
        }
    }

    rc = 0;
 
    cleanup:
    return rc;

}