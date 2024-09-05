#define ARCHIVE_BLOCK_SIZE    512 /* The size of an archive block */
#define ARCHIVE_NAME_SIZE     100 /* File name portion of the header */
#define ARCHIVE_MODE_SIZE     8   /* File modes portion of the header */
#define ARCHIVE_UID_SIZE      8   /* User id portion of the header */
#define ARCHIVE_GID_SIZE      8   /* Group id portion of the header */
#define ARCHIVE_SIZE_SIZE     12  /* File size portion of the header */
#define ARCHIVE_MTIME_SIZE    12  /* Modification time portion of the header */
#define ARCHIVE_CHKSUM_SIZE   8   /* Checksum portion of the header */
#define ARCHIVE_TYPEFLAG_SIZE 1   /* File type portion of the header */
#define ARCHIVE_LINKNAME_SIZE 100 /* Link name portion of the header */
#define ARCHIVE_MAGIC_SIZE    6   /* Magic number portion of the header */
#define ARCHIVE_MAGIC         "ustar" /* Magic number of the header */
#define ARCHIVE_VERSION_SIZE  2       /* Version portion of the header */
#define ARCHIVE_VERSION       "00"    /* Version of the header */
#define ARCHIVE_UNAME_SIZE    32      /* User name portion of the header */
#define ARCHIVE_GNAME_SIZE    32      /* Group name portion of the header */
#define ARCHIVE_DEVMAJOR_SIZE 8   /* Major device number portion of header */
#define ARCHIVE_DEVMINOR_SIZE 8   /* Minor device number portion of header */
#define ARCHIVE_PREFIX_SIZE   155 /* Prefix portion of the header */

#define PERMISSIONS_WIDTH 10
#define OWNER_GROUP_WIDTH 17
#define SIZE_WIDTH        8
#define MTIME_WIDTH       16

#define REGULAR_FILE           '0'
#define REGULAR_FILE_ALTERNATE '\0'
#define SYMBOLIC_LINK          '2'
#define DIRECTORY              '5'

typedef struct USTARHeader {
  /* File name portion of the header */
  char name[ARCHIVE_NAME_SIZE];
  /* File modes portion of the header */
  char mode[ARCHIVE_MODE_SIZE];
  /* User id portion of the header */
  char uid[ARCHIVE_UID_SIZE];
  /* Group id portion of the header */
  char gid[ARCHIVE_GID_SIZE];
  /* File size portion of the header */
  char size[ARCHIVE_SIZE_SIZE];
  /* Modification time portion of the header */
  char mtime[ARCHIVE_MTIME_SIZE];
  /* Checksum portion of the header */
  char chksum[ARCHIVE_CHKSUM_SIZE];
  /* File type portion of the header */
  char typeflag;
  /* Link name portion of the header */
  char linkname[ARCHIVE_LINKNAME_SIZE];
  /* Magic number portion of the header */
  char magic[ARCHIVE_MAGIC_SIZE];
  /* Version portion of the header */
  char version[ARCHIVE_VERSION_SIZE];
  /* User name portion of the header */
  char uname[ARCHIVE_UNAME_SIZE];
  /* Group name portion of the header */
  char gname[ARCHIVE_GNAME_SIZE];
  /* Major device number portion of header */
  char devmajor[ARCHIVE_DEVMAJOR_SIZE];
  /* Minor device number portion of header */
  char devminor[ARCHIVE_DEVMINOR_SIZE];
  /* Prefix portion of the header */
  char prefix[ARCHIVE_PREFIX_SIZE];
} USTARHeader;


void usage();
void create_archive (char *archive_name, char *paths[], int num_paths,
     int verbose, int strict);
void add_to_archive (int archive_fd, char *path, int verbose, int strict);
void writeFileContent(int archive_fd, char* path);
void checkDir(int archive_fd, char* path, int verbose, int strict);
void checkLink(int archive_fd, char* path, int verbose, int strict);
void format_permissions(mode_t mode, char *permissions);
void format_time(char *mtime, char *formatted_time);
void list_archive(char *name, char *paths[],
    int num_paths, int verbose, int strict);
void list_archive (char *name, char *paths[],
     int num_paths, int verbose, int strict);
void read_contents(int archive_fd, char *paths[],
    int num_paths, int verbose, int strict, USTARHeader **out_headers,
    unsigned char ***out_contents, size_t *out_count);
int matches_path(const char *name, char *paths[], int num_paths);
int read_header(int archive_fd, USTARHeader *header, int strict);
int is_block_empty(unsigned char *block);
void extract_archive (char *archive_name, char *paths[],
     int num_paths, int verbose, int strict);
uint32_t extract_special_int(char *where, int len);
int insert_special_int(char *where, size_t size, int32_t val);