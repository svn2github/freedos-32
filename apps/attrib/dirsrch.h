

#define ALL_FINISHED 1
#define DIR_FINISHED 2
#define FIRST_ROUND 3
#define NO_SUBDIRS 4


#define ERR_TRUENAME -1
#define ERR_NONAME -2
#define PATH_NOT_FOUND -3
#define ERR_DEPTH -4
#define ERR_INT -5

#define MAX_DEPTH 128
#define FILE_FIND_FLAGS    FD32_FARDONLY | FD32_FAHIDDEN | FD32_FASYSTEM \
            | FD32_FAARCHIV | FD32_FRNONE | FD32_FWILDCRD
#define DIR_FIND_FLAGS    FD32_FADIR | FD32_FRDIR | FD32_FWILDCRD



typedef struct
{
    int dir_s_handle[MAX_DEPTH];  /* array of search handles for dirs*/
    int file_s_handle;            /* search handle for files */
    int current_dir;              /* index of the the last search handle */
    char path[FD32_LFNPMAX];
    char* f_path;
    char* s_fname;                /* the name part of the search path */
    int end_path;                 /* index into "path" poining to the end of the */
                                  /* current path-part of the search path */
    int first_dir;                /* index into "path" poining to the end of the */
                                  /* original path-part of the search path */
    int search_subdir;            /* TRUE if subdirs should be searched */
    char err_no;
}
search_data;

int init_dir_search( search_data* sd);
int nextfind(search_data* sd, fd32_fs_lfnfind_t* fd);
void err_close_dirs(search_data* sd);
