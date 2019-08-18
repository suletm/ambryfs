/*
  AmbryFS implementation libfuse
  Copyright (C) 2017  Tofig Suleymanov
  This program can be distributed under the terms of the GNU GPL.
*/

#define FUSE_USE_VERSION 30
#define BUFSIZE 2000

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <stddef.h>


struct MemoryPool {
    char *memory;
    size_t size;
    double len;
};


static struct options {
        const char *ambry_base_url;
        const char *ambry_port;
} options;


#define OPTION(t, p) { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
        OPTION("--ambry_base_url=%s", ambry_base_url),
        OPTION("--ambry_port=%i", ambry_port),
        FUSE_OPT_END
};

static int do_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *ffi)
{
    #ifdef FUSE_VERBOSE
        fprintf(stderr,"do_write() triggered with path: %s, size: %d and offset: %d", path, size, offset);
    #endif
    return -EROFS;
}

/* do_create() is called when open with O_CREAT is called on an inexistent file
 * It returns 0 on success and the new fd should be assigned to ffi->fh 
 */
static int do_create (const char *path, mode_t mode, struct fuse_file_info *ffi)
{
    #ifdef FUSE_VERBOSE
        fprintf(stderr,"do_create() is triggered on a read only filesystem with path: %s\n", path);
    #endif
    return -EROFS;
}

static int do_open(const char *path, struct fuse_file_info *ffi)
{
    #ifdef FUSE_VERBOSE
        fprintf(stderr,"do_open() is triggered on a read only filesystem with path: %s", path);
    #endif
    return 0;
}

static int do_unlink(const char *path)
{
    CURL *curl;
    CURLcode res;
    char url[BUFSIZE]; // max url size per https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers

    curl = curl_easy_init();

    #ifdef FUSE_VERBOSE
        fprintf(stderr,"unlink(): Blob name received in if_blob_exists: %s\n", path);
    #endif
    snprintf(url,BUFSIZE, "%s%s",options.ambry_base_url, path);
    #ifdef FUSE_VERBOSE
        fprintf(stderr,"Url constructed in unlink: %s\n", url);
    #endif

    if(curl) {
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, "x-ambry-service-id: ambryfs");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_PORT, options.ambry_port);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_easy_setopt(curl, CURLOPT_VERBOSE, VERBOSE);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                      curl_easy_strerror(res));
            return -1;
        }

        long int http_code;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo() failed: %s\n",
                      curl_easy_strerror(res));
            return -1;
        }

        if(http_code != 202) {
            #ifdef FUSE_VERBOSE
                fprintf(stderr, "Blob not found or invalid request. Http code returned: %d\n", http_code);
            #endif
            return -1;
        }
        fprintf(stderr, "Blob deleted: %s", path);
   }
    return 0;
}

size_t ambry_write_callback(char *ptr, size_t size, size_t nmemb, void *up)
{
    struct MemoryPool *mem = (struct MemoryPool *)up;

    #ifdef FUSE_VERBOSE
        fprintf(stderr, "curl_write_callback triggered. Size: %d. Nmemb: %d\n",size,nmemb);
    #endif
    mem->memory = realloc(mem->memory, mem->size + size * nmemb + 1);
    if(!mem->memory) {
        fprintf(stderr, "Realloc failed to allocate user data storage\n");
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), ptr, size * nmemb);
    mem->size = mem->size + size * nmemb;
    mem->memory[mem->size] = 0;
    return size * nmemb;
}

static int if_blob_exists(const char *blob_name)
{
    CURL *curl;
    CURLcode res;
    char url[BUFSIZE]; // max url size per https://stackoverflow.com/questions/417142/what-is-the-maximum-length-of-a-url-in-different-browsers
    double content_length;
    int status;

    #ifdef FUSE_VERBOSE
        fprintf(stderr,"Blob name received in if_blob_exists: %s\n", blob_name);
    #endif
    snprintf(url,BUFSIZE, "%s%s",options.ambry_base_url,blob_name);
    #ifdef FUSE_VERBOSE
        fprintf(stderr,"Url constructed in if_blob_exists: %s\n", url);
    #endif
    curl = curl_easy_init();
    if(curl) {
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, "x-ambry-service-id: ambryfs");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_PORT, options.ambry_port);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, VERBOSE);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                      curl_easy_strerror(res));
            return -1;
        }

        long int http_code;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo() failed: %s\n",
                      curl_easy_strerror(res));
            return -1;
        }

        #ifdef FUSE_VERBOSE
            fprintf(stderr, "Http code returned: %d for blob: %s\n", http_code, blob_name);
        #endif
        switch(http_code)
        {
            case 200:
                #ifdef FUSE_VERBOSE
                    fprintf(stderr, "Blob exists: %s\n", blob_name);
                #endif
                res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
                if(res != CURLE_OK) {
                    fprintf(stderr, "curl_easy_getinfo() CURLINFO_CONTENT_LENGTH_DOWNLOAD failed: %s\n",
                    curl_easy_strerror(res));
                    status = -EIO;
                } else {
                    status = content_length;
                }
                break;
            case 410:
            case 404:
                status = -ENOENT;
                break;
            case 400:
                status = -EINVAL;
                break;
            default:
                status = -EINVAL;
                break;
        }
    } else {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
        status = -EIO;
    }
    return status;
}


static int do_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *ffi)
{
    CURL *curl;
    CURLcode res;
    char url[BUFSIZE];
    double len;
    struct MemoryPool userdata;
    if (offset == 0)
        userdata.size = 0;

    #ifdef FUSE_VERBOSE
        fprintf(stderr, "do_read(): triggered for path: %s\n", path);
    #endif
    snprintf(url,BUFSIZE, "%s%s",options.ambry_base_url,path);
    #ifdef FUSE_VERBOSE
        fprintf(stderr, "do_read(): constructed Ambry URL: %s\n", url);
    #endif

    if (offset == 0) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        userdata.memory = malloc(1); // Allocate 1 byte. We will realloc it later
        if(curl) {
            struct curl_slist *chunk = NULL;
            chunk = curl_slist_append(chunk, "x-ambry-service-id: ambryfs");
            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_PORT, options.ambry_port);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ambry_write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &userdata);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, VERBOSE);

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
                return -EIO;
            }

            res = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &len);
            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_getinfo() CURLINFO_SIZE_DOWNLOAD failed: %s\n",
                    curl_easy_strerror(res));
                return -EIO;
            }
            userdata.len = len;
            curl_easy_cleanup(curl);
        } else {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
            return -EIO;
        }
    }

    if (offset < userdata.len) {
        #ifdef FUSE_VERBOSE
            fprintf(stderr,"Offset is within the length of the blob. Offset: %ld. Len: %lf\n", offset, userdata.len);
        #endif
        if (offset + size > userdata.len) {
            #ifdef FUSE_VERBOSE
                fprintf(stderr,"Offset + size spil over the file. Offset: %ld. Len: %lf. Size: %ld. New size: %lf\n", offset, userdata.len, size, userdata.len-offset);
            #endif
            size = (size_t) (userdata.len - offset);
            #ifdef FUSE_VERBOSE
                fprintf(stderr, "Copying %d (size) bytes from userdata + %d (offset)\n",size, offset);
            #endif
            memcpy(buf, userdata.memory + offset, size);
            free(userdata.memory); // We don't need this buffer anymore since we've read all the data
            return size;
        } else {
            #ifdef FUSE_VERBOSE
                fprintf(stderr,"Reading size bytes from offset is within the len of the blob. Offset: %ld. Size: %ld. Blob Len: %lf\n", offset, size, userdata.len);
            #endif
            memcpy(buf, (const char *) userdata.memory + offset, size); // We might return here, since there is data to be read. Leaving userdata around
            return size;
        }
    } else {
        free(userdata.memory); // We don't need this buffer anymore since we've read all the data
        return 0;
    }
}

static int do_getattr (const char *path, struct stat *st)
{
    double len;
    if ( strcmp( path, "/" ) == 0 )
    {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else
    {
        len = if_blob_exists(path);
        if(len >= 0 ) {
            #ifdef FUSE_VERBOSE
                fprintf(stderr, "Found blob in ambry: %s. Size: %f\n", path, len);
            #endif
            st->st_mode = S_IFREG | 0644;
            st->st_nlink = 1;
            st->st_size = len;
        } else {
            #ifdef FUSE_VERBOSE
                fprintf(stderr, "Blob not found in ambry: %s\n", path);
            #endif
            return len; // len will contain the errno status
        }
        
    }
    return 0;
}



static struct fuse_operations operations = {
    .getattr = do_getattr,
    .read	= do_read,
    .unlink = do_unlink,
    .open = do_open,
    .create = do_create,
    .write = do_write,
};

int main( int argc, char *argv[] )
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

	return fuse_main( args.argc, args.argv, &operations, NULL );
}


