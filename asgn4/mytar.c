#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <time.h>
#include <utime.h>
#include <errno.h>

#include "mytar.h"



int main (int argc, char* argv[]) {
    char* options;
    char option[2];
    int optlen, i;
    int create = 0, list = 0, extract = 0, verbose, strict = 0;
    char *archive_name = NULL;

    /* Check if we didn't provide enough args */
    if (argc < 3) {
        usage();
    }

    /* Set the flags here */
    options = argv[1];
    /* check the length of the flags */
    optlen = strlen(options);

    /* Loop through each flag */
    for (i = 0; i <= optlen; i++) {
        /* Assign the char */
        option[0] = options[i];
        option[1] = '\0';
        /* Check which flag the char is */
        if (!strcmp("c", option)) {
            /*printf("Found Create\n");*/
            create = 1;
        } else if (!strcmp("t", option)) {
            /*printf("Found List\n");*/
            list = 1;
        } else if (!strcmp("x", option)) {
            /*printf("Found Extract\n");*/
            extract = 1;
        } else if (!strcmp("v", option)) {
            /*printf("Found Verbose\n");*/
            verbose = 1;
        } else if (!strcmp("S", option)) {
            /*printf("Found Strict\n");*/
            strict = 1;
        } else if (!strcmp("f", option)) {
            /*printf("Found F\n");*/
            /* Assign the archive name */
            archive_name = argv[2]; 
        }
    }

    /* Check if multiple options were selected or f option was not selected */ 
    if ((create + list + extract) != 1 || archive_name == NULL) {
        usage();
    }

    /* If create was selected */
    if (create) {
        /* Check if we provided enough args */
        if (argc <= 3) {
            usage();
        }
        /* Create the archive */
        create_archive(archive_name, &argv[3], argc - 3, verbose, strict);
    /* If list was selected  */
    } else if (list) {
        /* If no pathnames were selected */
        if (argc == 3) {
            /* Put NULL for pathnames, and 0 for num paths */
            list_archive(archive_name, NULL, 0, verbose, strict);
        } else {
            /* Else point to argv[3] and set the count to argc - 3 */
            list_archive(archive_name, &argv[3], argc - 3, verbose, strict);
        }
    /* If extract was selected */
    } else if (extract) {
        /* Same as for list */
        if (argc == 3) {
            extract_archive(archive_name, NULL, 0, verbose, strict);
        } else {
            extract_archive(archive_name, &argv[3], argc - 3, verbose, strict);
        }
    } else {
        /* No valid option was selected */
        usage();
    }


    return 0;
}

/* Print out the usage message and exit */
void usage() {
  fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
  exit(EXIT_FAILURE);
}

/* 
 * Create archive based on name provided
 * The Paths to compile together along with the num of them
 * And lastly flags of verbose or strict
 */
void create_archive (char *archive_name, char *paths[],
     int num_paths, int verbose, int strict) {
    int i;
    int archive_fd;
    char *padding;

    /* Open a new file for the archive */
    archive_fd = open(archive_name, (O_WRONLY | O_CREAT | O_TRUNC), S_IRWXU);
    if (archive_fd == -1) {
        perror("Archive Opening");
        return;
    }

    /* Loop through the paths to compile */
    for (i = 0; i < num_paths; i++) {
        /* Add each path to the archive */
        add_to_archive(archive_fd, paths[i], verbose, strict);
    }

    /* Create padding for end of archive */
    padding = (char *)calloc(sizeof(char), ARCHIVE_BLOCK_SIZE * 2);
    if (padding == NULL) {
        perror("Memory Allocation");
        exit(EXIT_FAILURE);
    }
    /* Write the padding to the end of archive */
    if ((i = write(archive_fd, padding, ARCHIVE_BLOCK_SIZE * 2)) == -1) {
        perror("Writing File");
        exit(EXIT_FAILURE);
    }
    /*printf("Padding Written: %x\n", i);*/

    free(padding);

    if (close(archive_fd) == -1) {
        perror("Archive Close");
        return;
    }
}

/*
 * Take the file to write to, the path name, verbose and strict flags
 * To write the header/contents if there are any
 */
void add_to_archive (int archive_fd, char *path, int verbose, int strict) {
    struct stat stat;
    USTARHeader header;
    unsigned int checksum = 0;
    struct passwd *password;
    struct group *group;
    char *padding;
    int i;
    int size_path = 0;
    int path_len = strlen(path);
    int strict_pass = 1, write_pass = 1;

    /* Set the USTARHeader to all nul */
    memset(&header, '\0', sizeof(USTARHeader));
    /* get the path lstat */
    if (lstat(path, &stat) == -1) {
        perror("Failed to stat file");
        return;
    }

    /* If path_len is larger then what can be stored, skip the file */
    if (path_len > 256) {
        return;
    }

    /* If path_len is larger than the name section
     * Find a convient '/' to break off of
     */
    if (path_len > ARCHIVE_NAME_SIZE) {
        for (i = path_len - 101; i < path_len; i++) {
            if (path[i] == '/') {
                size_path = i;
                break;
            }
        }
        /* Store in the prefix up to that '/' found */
        strncpy(header.prefix, path, size_path);

        /* Store the rest in the name */
        strncpy(header.name, path + size_path + 1, ARCHIVE_NAME_SIZE);
    } else {
        /* Else just store the path in the name */
        strncpy(header.name, path, ARCHIVE_NAME_SIZE);
    }

    /* Store the mode */
    snprintf(header.mode, ARCHIVE_MODE_SIZE, "%07o",
      (unsigned int)stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));

    /* Store the uid */
    snprintf(header.uid, ARCHIVE_UID_SIZE, "%o", stat.st_uid);
    /* Check if it's an invalid uid size */
    if (sizeof(header.uid) > ARCHIVE_UID_SIZE - 1) {
        /* Check strict and verbose */
        if (strict && verbose) {
            strict_pass = 0;
        }
        /* Check if we can write header */
        if (insert_special_int(header.uid,
             ARCHIVE_UID_SIZE, stat.st_uid) != 0) {
            if (strict && verbose) {
                write_pass = 0;
            }
        }
    } else {
        /* Else just store uid */
        snprintf(header.uid, ARCHIVE_UID_SIZE, "%07o", stat.st_uid);
    }

    /* Store gid */
    snprintf(header.gid, ARCHIVE_GID_SIZE, "%07o", stat.st_gid);

    /* Store mtime */
    snprintf(header.mtime, ARCHIVE_MTIME_SIZE,
             "%011o", (unsigned int)stat.st_mtime);

    /* Check typeflag and store */
    if (S_ISREG(stat.st_mode)) {
        header.typeflag = REGULAR_FILE;
        /*printf("Regular File\n");*/
        snprintf(header.size, ARCHIVE_SIZE_SIZE,
            "%011o", (unsigned int)stat.st_size);
    } else if (S_ISLNK(stat.st_mode)) {
        header.typeflag = SYMBOLIC_LINK;
        snprintf(header.size, ARCHIVE_SIZE_SIZE,
            "%011o", (unsigned int)stat.st_size);
        /*printf("Symbolic Link\n");*/
    } else if (S_ISDIR(stat.st_mode)) {
        header.typeflag = DIRECTORY;
        /*printf("Directory\n");*/
        /* update path name to have a / since its a dir */
        snprintf(header.size, ARCHIVE_SIZE_SIZE,
            "%011o", 0);
        /* Since it's a directory, append a '/' */
        header.name[strlen(header.name)] = '/';
    }
    
    /* Read link and store */
    readlink(path, header.linkname, ARCHIVE_LINKNAME_SIZE);

    /* store magic */
    snprintf(header.magic, ARCHIVE_MAGIC_SIZE + 1, "%s", ARCHIVE_MAGIC);

    /* Store version */
    snprintf(header.version, ARCHIVE_VERSION_SIZE + 1, "%s", ARCHIVE_VERSION);

    /* Get uname */
    password = getpwuid(stat.st_uid);
    /* Store uname */
    snprintf(header.uname, ARCHIVE_UNAME_SIZE, "%s", password->pw_name);

    /* Get gname */
    group = getgrgid(stat.st_gid);
    /* Store gname */
    snprintf(header.gname, ARCHIVE_GNAME_SIZE, "%s", group->gr_name);

    /* Calculate checksum, by summing all bytes */
    for (i = 0; i < sizeof(USTARHeader); i++) {
        checksum += ((unsigned char*)&header)[i];
    }

    /* Add values for checksum space location */
    for (i = 0; i < ARCHIVE_CHKSUM_SIZE; i++) {
        checksum += ' ';
    }
    
    /* Store checksum */
    snprintf(header.chksum, ARCHIVE_CHKSUM_SIZE, "%07o", checksum);



    /* Check If strict and it passes strict or
       if strict flag is not selected
       Then check if we passed the write check */
    if ((!strict || (strict && strict_pass)) && write_pass) {
        /* Write all header sections that we stored */
        if (write(archive_fd, header.name, ARCHIVE_NAME_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.mode, ARCHIVE_MODE_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.uid, ARCHIVE_UID_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.gid, ARCHIVE_GID_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.size, ARCHIVE_SIZE_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.mtime, ARCHIVE_MTIME_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.chksum, ARCHIVE_CHKSUM_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, &(header.typeflag),
             ARCHIVE_TYPEFLAG_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.linkname, ARCHIVE_LINKNAME_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.magic, ARCHIVE_MAGIC_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.version, ARCHIVE_VERSION_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.uname, ARCHIVE_UNAME_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.gname, ARCHIVE_GNAME_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.devmajor,
             ARCHIVE_DEVMAJOR_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.devminor,
             ARCHIVE_DEVMINOR_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        if (write(archive_fd, header.prefix, ARCHIVE_PREFIX_SIZE) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }

        /* Add the rest of the padding of 12
           bytes to finish the block of 512 */
        padding = (char *)calloc(sizeof(char), 12);
        if (padding == NULL) {
            perror("Memoery Allocation");
            exit(EXIT_FAILURE);
        }
        if (write(archive_fd, padding, 12) == -1) {
            perror("Write file");
            exit(EXIT_FAILURE);
        }        

    }

    /* Check verbose and print */
    if (verbose) {
        printf("%s%s\n", header.prefix, header.name);
    }

    /* Continue next file depending on what current file is
       If file, write blocks
       If Dir, open directory and try Adding to archive
       If Link, Check what link it is and repeat */
    if (S_ISREG(stat.st_mode)) {
        writeFileContent(archive_fd, path);
    } else if (S_ISDIR(stat.st_mode)) {
        checkDir(archive_fd, path, verbose, strict);
    } else if (S_ISLNK(stat.st_mode)) {
        checkLink(archive_fd, path, verbose, strict);
    }
}

/*
 * When called, will write the contents of the path provided
 * Will write padding to make sure blocks are of size 512
 */
void writeFileContent(int archive_fd, char* path) {
    struct stat stat;
    size_t file_size, padding_size;
    int infile;
    unsigned char *contents;
    char *padding;

    /* Stat the file */
    if (lstat(path, &stat) == -1) {
        perror("Stat file");
        exit(EXIT_FAILURE);
    }
    /* Grab file size */
    file_size = stat.st_size;

    /* Open file to read */
    if ((infile = open(path, O_RDONLY, 0)) == -1) {
        perror("Opening file");
        exit(EXIT_FAILURE);
    }

    /* Allocate buffer for bytes read */
    contents = (unsigned char *)calloc(file_size, sizeof(unsigned char));
    if (contents == NULL) {
        perror("Memory Allocation");
        close(infile);
        exit(EXIT_FAILURE);
    }

    /* Read file into buffer */
    if (read(infile, contents, file_size) == -1) {
        perror("Reading file");
        close(infile);
        free(contents);
        exit(EXIT_FAILURE);
    }

    /* Write buffer into archive */
    if (write(archive_fd, contents, file_size) == -1) {
        perror("Write file");
        close(infile);
        free(contents);
        exit(EXIT_FAILURE);
    }

    /* Calculate the padding to finish block size of 512 */
    padding_size = ARCHIVE_BLOCK_SIZE - (file_size % ARCHIVE_BLOCK_SIZE);
    /* If padding is 512, that means file
       content is already in blocks of 512 */
    if (padding_size == ARCHIVE_BLOCK_SIZE) {
        padding_size = 0;
    }

    /* If padding is valid, write the padding */
    if (padding_size > 0) {
        padding = (char *)calloc(padding_size, sizeof(char));
        if (padding == NULL) {
            perror("Calloc");
            close(infile);
            free(contents);
            exit(EXIT_FAILURE);
        }
        if (write(archive_fd, padding, padding_size) == -1) {
            perror("Write file");
            close(infile);
            free(contents);
            free(padding);
            exit(EXIT_FAILURE);
        }
        free(padding);
    }
    close(infile);
    free(contents);
}

/*
 * When called, will open up the directory and try adding each
 * element to the archive.
 */
void checkDir(int archive_fd, char* path, int verbose, int strict) {
    DIR *dir;
    struct dirent *entry;
    char *new_path;
    size_t new_path_len;

    /* Try opening directory */
    if ((dir = opendir(path)) == NULL) {
        perror("Directory Open"); 
        exit(EXIT_FAILURE);
    }

    /* Loop while elements still remain */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip self and parent */
        if (strcmp(entry->d_name, ".") == 0
             || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Calculate new path length based on path name
           and d_name + combining slash and nul terminating */
        new_path_len = strlen(path) + strlen(entry->d_name) + 2;
        /* Allocate memory for new_path */
        new_path = (char *)calloc(new_path_len, sizeof(char));
        if (new_path == NULL) {
            perror("Memory Allocation");
            closedir(dir);
            exit(EXIT_FAILURE);
        }
        /* Combine path and d_name with a slash inbetween */
        snprintf(new_path, new_path_len, "%s/%s", path, entry->d_name);
        /* Add this to archive recursively */
        add_to_archive(archive_fd, new_path, verbose, strict);
        free(new_path);
    }
    /* Close when all elements are added */
    closedir(dir);
}

/*
 * When called, will check where the link goes
 * Then will determine what to do
 */
void checkLink(int archive_fd, char* path, int verbose, int strict) {
    char linkname[PATH_MAX];
    struct stat linkstat;
    /* Read link */
    ssize_t r = readlink(path, linkname, sizeof(linkname) - 1);
    /* If valid stat link */
    if (r != -1) {
        linkname[r] = '\0';
        if (lstat(path, &linkstat) == -1) {
            perror("lstat");
            return;
        }
        /* If file, write contents */
        if (S_ISREG(linkstat.st_mode)) {
            writeFileContent(archive_fd, linkname);
        /* If directory, check to see if there are files there to add */
        } else if (S_ISDIR(linkstat.st_mode)) {
            checkDir(archive_fd, linkname, verbose, strict);
        /* If it is another link, recursively call checkLink */
        } else if (S_ISLNK(linkstat.st_mode)) {
            checkLink(archive_fd, linkname, verbose, strict);
        /* Else not supported and return */
        } else {
            return;
        }
    } else {
        perror("Read Link");
        return;
    }
}

/* Format the permissions based on the mode
 * Does not check typeflag for file type
 * That is handled somewhere else
 */
void format_permissions(mode_t mode, char *permissions) {
    /* AND mode and permission macro together */
    permissions[1] = mode & S_IRUSR ? 'r' : '-';
    permissions[2] = mode & S_IWUSR ? 'w' : '-';
    permissions[3] = mode & S_IXUSR ? 'x' : '-';

    permissions[4] = mode & S_IRGRP ? 'r' : '-';
    permissions[5] = mode & S_IWGRP ? 'w' : '-';
    permissions[6] = mode & S_IXGRP ? 'x' : '-';

    permissions[7] = mode & S_IROTH ? 'r' : '-';
    permissions[8] = mode & S_IWOTH ? 'w' : '-';
    permissions[9] = mode & S_IXOTH ? 'x' : '-';
    /* Nul terminate string */
    permissions[10] = '\0';
}

/* Format the time based on mtime */
void format_time(char *mtime, char *formatted_time) {
    time_t raw_time = (time_t)strtol(mtime, NULL, 8);
    struct tm *time_info = localtime(&raw_time);
    strftime(formatted_time, 17, "%Y-%m-%d %H:%M", time_info);
}
/*
 * Will open a tar file and list out the specified paths
 * If no paths specified, list out entire archive
 * Supports verbose listing
 */
void list_archive(char *name, char *paths[],
    int num_paths, int verbose, int strict) {
  
    USTARHeader *headers;
    unsigned char **contents;
    size_t header_count;
    int i;
    char nameprint[ARCHIVE_NAME_SIZE + 1];
    char prefixprint[ARCHIVE_PREFIX_SIZE + 1];
    char full_path[ARCHIVE_PREFIX_SIZE + ARCHIVE_NAME_SIZE + 1];
    char permissions[11];
    char formatted_time[17];
    char ugname[35];
    int archive_fd;
    mode_t mode;


    /* Try opening archive */
    archive_fd = open(name, O_RDONLY);
    if (archive_fd == -1) {
        perror("Archive Open");
        return;
    }


    /* Read headers and file contents
       into our *headers and **contents */
    read_contents(archive_fd, paths, num_paths, verbose,
            strict, &headers, &contents, &header_count);
   
    /* For each header read list them out */
    for (i = 0; i < header_count; i++) {
        /* Combine prefix and name properly */
        memcpy(nameprint, headers[i].name, ARCHIVE_NAME_SIZE);
        memcpy(prefixprint, headers[i].prefix, ARCHIVE_PREFIX_SIZE);
        /* Make sure they are nul terminated */
        nameprint[ARCHIVE_NAME_SIZE] = '\0';
        prefixprint[ARCHIVE_PREFIX_SIZE] = '\0';

        /* If prefix is valid */
        if (strlen(prefixprint) > 0) {
            /* Combine prefix with name */
            snprintf(full_path, sizeof(full_path),
                "%s/%s", prefixprint, nameprint);
        } else {
            /* Else only use name */
            snprintf(full_path, sizeof(full_path), "%s", nameprint);
        }

        /* If verbose is specified */ 
        if (verbose) {
            /* Grab mode and convert back into long from octal */
            mode = (mode_t)strtoul(headers[i].mode, NULL, 8);
            /* Format the permissions */
            format_permissions(mode, permissions);

            /* Check typeflag for type */
            if (headers[i].typeflag == '5') {
                permissions[0] = 'd';
            } else if (headers[i].typeflag == '2') {
                permissions[0] = 'l'; 
            } else {
                permissions[0] = '-';
            } 

            /* Format the times of the file */
            format_time(headers[i].mtime, formatted_time);

            /* Combine uname and gname together */
            snprintf(ugname, sizeof(ugname), "%s/%s",
                    headers[i].uname, headers[i].gname);

            /* Print out verbose information */
            printf("%s %-17s %8ld %s %s\n",
            permissions,
            ugname,
            strtol(headers[i].size, NULL, 8),
            formatted_time,
            full_path); 
        } else {
            /* If verbose not specified, print full path */
            printf("%s\n", full_path);
        }
    }
    
    /* Free every contents */
    for (i = 0; i < header_count; i++) {
        free(contents[i]);
    }

    /* Free pointer to contents */
    free(contents);
    free(headers);

    /* Close Archive*/
    if (close(archive_fd) == -1) {
        perror("Archive Close");
        return;
    }
}

/* Read the headers and file contents 
 * Also checks if it matches paths specified
 */
void read_contents(int archive_fd, char *paths[],
    int num_paths, int verbose, int strict, USTARHeader **out_headers,
    unsigned char ***out_contents, size_t *out_count) {
    int h;
    USTARHeader *headers = NULL;
    USTARHeader header;
    unsigned char **contents = NULL;
    size_t header_count = 0;
    off_t file_size, padding;
    char nameprint[ARCHIVE_NAME_SIZE + 1];
    char prefixprint[ARCHIVE_PREFIX_SIZE + 1];
    char full_path[ARCHIVE_PREFIX_SIZE + ARCHIVE_NAME_SIZE + 1];

    /* Keep looping until specified to break */
    while (1) {
        /* Read the header if archive read failed, lseek past it */
        if ((h = read_header(archive_fd, &header, strict)) == -1) {
            /* Calculate padding to skip */
            file_size = strtol(header.size, NULL, 8);
            padding = (file_size % ARCHIVE_BLOCK_SIZE)
            ? (ARCHIVE_BLOCK_SIZE - (file_size % ARCHIVE_BLOCK_SIZE)) : 0;
            /* lseek past header and contents */
            if (lseek(archive_fd, file_size + padding, SEEK_CUR) == -1) {
                perror("lseek");
                exit(EXIT_FAILURE);
            }
            continue;            
        }

        /* If read header is 1 that means end of archive */
        if (h == 1) {
            /* Break from reading headers and storing */
            break;
        }

        /* Copy name and prefix */
        memcpy(nameprint, header.name, ARCHIVE_NAME_SIZE);
        memcpy(prefixprint, header.prefix, ARCHIVE_PREFIX_SIZE);
        /* Make sure they are nul terminated */
        nameprint[ARCHIVE_NAME_SIZE] = '\0';
        prefixprint[ARCHIVE_PREFIX_SIZE] = '\0';

        /* If prefix is valid */
        if (strlen(prefixprint) > 0) {
            /* Combine prefix and name */
            snprintf(full_path, sizeof(full_path),
                "%s/%s", prefixprint, nameprint);
        } else {
            /* Else just use name */
            snprintf(full_path, sizeof(full_path), "%s", nameprint);
        }       

        /* Check if the header matches the paths specified */
        if (!matches_path(full_path, paths, num_paths)) {
            /* Skips header/contents if paths are not valid */
            file_size = strtol(header.size, NULL, 8);
            padding = (file_size % ARCHIVE_BLOCK_SIZE)
            ? (ARCHIVE_BLOCK_SIZE - (file_size % ARCHIVE_BLOCK_SIZE)) : 0;
            if (lseek(archive_fd, file_size + padding, SEEK_CUR) == -1) {
                perror("lseek");
                exit(EXIT_FAILURE);
            }
            continue;
        }

        /* Increase count */
        header_count++;
        /* Realloc to add more room for read header */
        headers = realloc(headers, header_count * sizeof(USTARHeader));
        contents = realloc(contents, header_count * sizeof(unsigned char *));
        if (!headers || !contents) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        /* Store header */
        headers[header_count - 1] = header;

        /* Check if there was any content */
        file_size = strtol(header.size, NULL, 8);
        if (file_size > 0) {
            /* If content exists, allocate memory for it */
            contents[header_count - 1] = malloc(file_size);
            if (!contents[header_count - 1]) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            /* Read the contents into the buffer */
            if (read(archive_fd, contents[header_count - 1],
                file_size) != file_size) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            /* Calculate rest of block padding and lseek apst it */
            padding = (file_size % ARCHIVE_BLOCK_SIZE)
             ? (ARCHIVE_BLOCK_SIZE - (file_size % ARCHIVE_BLOCK_SIZE)) : 0;
            if (lseek(archive_fd, padding, SEEK_CUR) == -1) {
                perror("lseek");
                exit(EXIT_FAILURE);
            }
        } else {
            /* Else if no content, set it to NULL */
            contents[header_count - 1] = NULL;
        }
    }

    /* Update headers/contents/count */
    *out_headers = headers;
    *out_contents = contents;
    *out_count = header_count;
}

/* Check if the name matches the paths specified */
int matches_path(const char *name, char *paths[], int num_paths) {
    int i;
    /* If no paths specified, return success */
    if (paths == NULL || num_paths == 0) {
        return 1;
    }
    /* Each path specified, compare current path up to that */
    for (i = 0; i < num_paths; i++) {
        /* If it is the same return success */
        if (strncmp(name, paths[i], strlen(paths[i])) == 0) {
            return 1;
        }
    }
    /* Else return failure */
    return 0;
}

/*
 * Reads a block of 512 and converts it into USTARHeader
 * Also checks if we reached end of archive
 */
int read_header(int archive_fd, USTARHeader *header, int strict) {
    unsigned char block[ARCHIVE_BLOCK_SIZE];
    ssize_t bytes_read;

    /* Reads a block of 512 */
    bytes_read = read(archive_fd, block, ARCHIVE_BLOCK_SIZE);
    if (bytes_read == -1) {
        perror("Read Error");
        return -1;
    } else if (bytes_read == 0) {
        return 0;
    } else if (bytes_read != ARCHIVE_BLOCK_SIZE) {
        fprintf(stderr, "Incomplete header read\n");
        return -1;
    }

    /* Checks if the block is completely empty */ 
    if (is_block_empty(block)) {
        /* If empty, read the next 512 block */
        if (read(archive_fd, block, ARCHIVE_BLOCK_SIZE)
            != ARCHIVE_BLOCK_SIZE) {
            perror("Failed to read second header block");
            exit(EXIT_FAILURE);
        }
        /* If this next 512 block is empty, we reached end of archive */
        if (is_block_empty(block)) {
            return 1;
        } else {
            /* Else we lseek back to the previous block
             * Not sure why we would though so I'm debating
             * on if I even need to check the 2nd block
             * Since if the block is empty, there is no valid
             * header. However, End of Archive is 2 512 empty blocks
             * So I will check it
             */
            lseek(archive_fd, -ARCHIVE_BLOCK_SIZE, SEEK_CUR);
        }
    }

    /* Copy the information into the header using the offsets */
    memcpy(header->name, block + 0, ARCHIVE_NAME_SIZE);
    memcpy(header->mode, block + 100, ARCHIVE_MODE_SIZE);
    memcpy(header->uid, block + 108, ARCHIVE_UID_SIZE);
    memcpy(header->gid, block + 116, ARCHIVE_GID_SIZE);
    memcpy(header->size, block + 124, ARCHIVE_SIZE_SIZE);
    memcpy(header->mtime, block + 136, ARCHIVE_MTIME_SIZE);
    memcpy(header->chksum, block + 148, ARCHIVE_CHKSUM_SIZE);
    header->typeflag = block[156];
    memcpy(header->linkname, block + 157, ARCHIVE_LINKNAME_SIZE);
    memcpy(header->magic, block + 257, ARCHIVE_MAGIC_SIZE);
    memcpy(header->version, block + 263, ARCHIVE_VERSION_SIZE);
    memcpy(header->uname, block + 265, ARCHIVE_UNAME_SIZE);
    memcpy(header->gname, block + 297, ARCHIVE_GNAME_SIZE);
    memcpy(header->devmajor, block + 329, ARCHIVE_DEVMAJOR_SIZE);
    memcpy(header->devminor, block + 337, ARCHIVE_DEVMINOR_SIZE);
    memcpy(header->prefix, block + 345, ARCHIVE_PREFIX_SIZE);

    /* If strict mode */
    if (strict) {
        /* Check if the magic is not the same as ARCHIVE_MAGIC */
        /* Also check if it is not nul terminated */
        if (strncmp(header->magic, ARCHIVE_MAGIC, ARCHIVE_MAGIC_SIZE) != 0 ||
            header->magic[ARCHIVE_MAGIC_SIZE] != '\0') {
            fprintf(stderr, "Invalid magic number in strict mode\n");
            return -1;
        }
        /* If magic passes, check if version is correct */
        if (strncmp(header->version, ARCHIVE_VERSION,
            ARCHIVE_VERSION_SIZE) != 0) {
            fprintf(stderr, "Invalid version number in strict mode\n");
            return -1;
        }
    /* Else just check if magic is the same */
    } else {
        /* Do not check if nul terminated */
        if (strncmp(header->magic, ARCHIVE_MAGIC,
            ARCHIVE_MAGIC_SIZE - 1) != 0) {
            fprintf(stderr, "Magic Number: %s\n", header->magic);
            fprintf(stderr, "Invalid magic number\n");
            return -1;
        }
    }
    /* If passes, return success */
    return 0;
}

/* Check if block is empty */
int is_block_empty(unsigned char *block) {
    int i;
    /* Iterate through block size */
    for (i = 0; i < ARCHIVE_BLOCK_SIZE; i++) {
        /* If at anypoint it is not '\0' return failure */
        if (block[i] != '\0') {
            return 0;
        }
    }
    /* Return success */
    return 1;
}

/* 
 * Creates all the directories up until the path provided
 * If Directory already exists, skip and move on
 */
int create_directories(char *path) {
    /* Find last '/' in path */
    char *sep = strrchr(path, '/');
    char tmp[PATH_MAX];
    struct stat st;

    /* If found a separation point */
    if (sep) {
        /* Copy up to that point */
        strncpy(tmp, path, sep - path);
        /* nul terminate it */
        tmp[sep - path] = '\0';

        /* Stat the path */
        if (stat(tmp, &st) == 0) {
            /* Check if it's a directory */
            if (S_ISDIR(st.st_mode)) {
                /* Return success */
                return 0;
            /* Else its not a directory */
            } else {
                /* Error message */
                fprintf(stderr, "%s exists and is not a directory\n", tmp);
                /* Return failure */
                return -1;
            }
        }

        /* Recursively call with tmp path */
        if (create_directories(tmp) == -1) {
            /* If failure, return failure */
            return -1;
        }

        /* Create a directory of tmp */
        if (mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            perror("mkdir");
            /* If failure to create, return failure */
            return -1;
        }
    }

    /* Return success */
    return 0;
}

/*
 * Extract From TAR file
 * Also specifies paths to extract specific paths
 * Has verbose and strict functionality
 */
void extract_archive (char *archive_name, char *paths[],
     int num_paths, int verbose, int strict) {

    USTARHeader *headers;
    unsigned char **contents;
    size_t header_count;
    char filename[ARCHIVE_PREFIX_SIZE + ARCHIVE_NAME_SIZE + 2];
    int i;
    int file_fd;
    struct utimbuf times;
    mode_t mode;
    size_t content_size;
    char nameprint[ARCHIVE_NAME_SIZE + 1];
    char prefixprint[ARCHIVE_PREFIX_SIZE + 1];

    /* Attempts to open archive */
    int archive_fd = open(archive_name, O_RDONLY);
    if (archive_fd == -1) {
        perror("Archive Open");
        return;
    }

    /* Clear char[] just in case */
    memset(&filename, '\0', sizeof(filename));
    memset(&nameprint, '\0', sizeof(nameprint));
    memset(&prefixprint, '\0', sizeof(prefixprint));
    /* Read headers/contents of archive and store them */
    read_contents(archive_fd, paths, num_paths, verbose,
            strict, &headers, &contents, &header_count);

    /* For each header */
    for (i = 0; i < header_count; i++) {
        /* Copy name */
        memcpy(nameprint, headers[i].name, ARCHIVE_NAME_SIZE);
        /* Copy prefix */
        memcpy(prefixprint, headers[i].prefix, ARCHIVE_PREFIX_SIZE);
        /* Make sure they are nul terminated */
        nameprint[ARCHIVE_NAME_SIZE] = '\0';
        prefixprint[ARCHIVE_PREFIX_SIZE] = '\0';
        
        /* If prefix is available */
        if (strlen(headers[i].prefix) > 0) {
            /* Combine prefix and name with a / */
            snprintf(filename, sizeof(filename), "%s/%s",
                    prefixprint, nameprint);
        } else {
            /* Else just use name */
            snprintf(filename, sizeof(filename), "%s", nameprint);
        }

        /* Check the header type */
        if (headers[i].typeflag == '0') {
            /* If it's a regular file */
            /* Make sure directories exist or create them */
            if (create_directories(filename) == -1) {
                /* If failure, skip file */
                continue;
            }

            /* Grab mode */
            mode = strtol(headers[i].mode, NULL, 8);
            /* Create new file or truncate old file using mode */
            file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
            if (file_fd == -1) {
                perror("Failed to create file");
                continue;
            }

            /* Grab time values */
            times.actime = time(NULL);
            times.modtime = strtol(headers[i].mtime, NULL, 8);
            /* Change file times */
            if (utime(filename, &times) == -1) {
                perror("utime");
            }

            /* If contents exist */
            if (contents[i] != NULL) {
                /* Find content size */
                content_size = strtol(headers[i].size, NULL, 8);
                /* Write content */
                if (write(file_fd, contents[i],
                    content_size) != content_size) {
                    perror("write");
                }
            }

            /* Close file */
            if (close(file_fd) == -1) {
                perror("close");
            }
        /* Else if file is directory */
        } else if (headers[i].typeflag == '5') {
            /* Make sure directories exist or create them */
            if (create_directories(filename) == -1) {
                /* If failure, skip directory */
                continue;
            }
            /* Set mode */
            mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; 
            /* Make directory with mode */
            if (mkdir(filename, mode) == -1) {
                perror("Failed to create directory");
                /* If failed, skip directory */
                continue;
            }
        /* Else if file is a link */
        } else if (headers[i].typeflag == '2') {
            /* Try to create a link */
            if (symlink(headers[i].linkname, filename) == -1) {
                perror("Failed to create symlink");
                continue;
            }
            /* Grab times */
            times.actime = time(NULL);
            times.modtime = strtol(headers[i].mtime, NULL, 8);
            /* Change file times */
            if (utime(filename, &times) == -1) {
                perror("Failed to set timestamp for symlink");
            }
        /* Else unsupported file type */
        } else {
            fprintf(stderr,
            "Unsupported file type for '%s', skipping extraction\n",
            filename);
            /* Skip file */
            continue;
        }

        /* Print out file name if verbose */
        if (verbose) {
            printf("Extracted: %s\n", filename);
        }
    }

    /* Free contents */
    for (i = 0; i < header_count; i++) {
        free(contents[i]);
    }
    /* Free pointer to contents */
    free(contents);
    free(headers);

    if (close(archive_fd) == -1) {
        perror("Archive Close");
    } 
}

uint32_t extract_special_int(char *where, int len) {
    /* For interoperability with GNU tar. GNU seems to
    * set the high–order bit of the first byte, then
    * treat the rest of the field as a binary integer
    * in network byte order.
    * I don’t know for sure if it’s a 32 or 64–bit int, but for
    * this version, we’ll only support 32. (well, 31)
    * returns the integer on success, –1 on failure.
    * In spite of the name of htonl(), it converts int32 t
    */
    int32_t val = -1;
    if ( (len >= sizeof(val)) && (where[0] & 0x80)) {
        /* the top bit is set and we have space
        * extract the last four bytes */
        val = *(int32_t *)(where + len - sizeof(val));
        val = ntohl(val); /* convert to host byte order */
    }
    return val;
}

int insert_special_int(char *where, size_t size, int32_t val) {
    /* For interoperability with GNU tar. GNU seems to
    * set the high–order bit of the first byte, then
    * treat the rest of the field as a binary integer
    * in network byte order.
    * Insert the given integer into the given field
    * using this technique. Returns 0 on success, nonzero
    * otherwise
    */
    int err = 0;
    if ( val < 0 || ( size < sizeof(val)) ) {
        /* if it’s negative, bit 31 is set and we can’t use the flag
        * if len is too small, we can’t write it. Either way, we’re
        * done.
        */
        err++;
    } else {
        /* game on....*/
        memset(where, 0, size); /* Clear out the buffer */
        /* place the int */
        *(int32_t *)(where + size - sizeof(val)) = htonl(val);
        *where |= 0x80; /* set that high–order bit */
    }
    return err;
}

