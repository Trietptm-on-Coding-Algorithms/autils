#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#else
#include <sys/stat.h>
#include <unistd.h> /* for mkstemp */
#endif

int
get_tmp_path(char *buf, int len)
{
    int rc = -1;
#ifdef _WIN32
   /* Windows you can no longer write to program files without permissions */
    char path[MAX_PATH+16];
    if(0 == GetTempPath(sizeof(path), path)) {
        //debug("GetTempPath()");
        goto cleanup;
    }

    strcpy(buf, path);
#else
    char *path = "/tmp";
    if(len < strlen(path)+1) {
        //debug("%s(): buffer not big enough", __func__);
        goto cleanup;
    }
    strcpy(buf, path);
#endif

    rc = 0;
    cleanup:
    return rc;
}

int
gen_tmp_file(const char *templ, char *path_out, FILE **fp_out)
{
    int rc = -1;

#ifdef _WIN32
    // TODO: implement me
#else
    int fd, i_XXXXXX, suffix_len;

    // TODO: secure this string crap
    char path[64];
    get_tmp_path(path, sizeof(path));
    strcat(path, "/");
    strcat(path, templ);

    // find length of suffix
    for(i_XXXXXX=0; i_XXXXXX < strlen(path)-5; i_XXXXXX++) {
        //debug("at idx=%d looking at: %s\n", i_XXXXXX, path+i_XXXXXX);
        if(0 == strncmp(path+i_XXXXXX, "XXXXXX", 6)) {
            break;
        }
    }
    if(i_XXXXXX >= strlen(path)-5) {
        //debug("ERROR: couldn't find XXXXXX in \"%s\"\n", path);
        goto cleanup;
    }

    suffix_len = strlen(path) - (i_XXXXXX+6);

    //debug("sending mkstemp \"%s\" with suffix length %d\n", path, suffix_len);
    fd = mkstemps(path, suffix_len);
    if(fd < 0) {
        //debug("ERROR: mkstemp() returned %d\n", fd);
        goto cleanup;
    }

    strcpy(path_out, path);

    /* file descriptor is linux specific int inside proc_struct,
        convert this to stdlib file pointer */
    *fp_out = fdopen(fd, "w");
    if(*fp_out == NULL) {
        //debug("ERROR: fdopen()\n");
        goto cleanup;
    }
#endif

    rc = 0;
    cleanup:
    return rc;
}

int 
check_file_exists(char *path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

