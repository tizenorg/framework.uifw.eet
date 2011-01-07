#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else /* ifdef HAVE_ALLOCA_H */
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif /* ifdef  __cplusplus */
void *    alloca (size_t);
#endif /* ifdef HAVE_ALLOCA_H */

#ifdef _WIN32
# include <winsock2.h>
#endif /* ifdef _WIN32 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <fnmatch.h>
#include <fcntl.h>
#include <zlib.h>

#ifndef _MSC_VER
# include <unistd.h>
#endif /* ifndef _MSC_VER */

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif /* ifdef HAVE_NETINET_IN_H */

#ifdef HAVE_EVIL
# include <Evil.h>
#endif /* ifdef HAVE_EVIL */

#ifdef HAVE_GNUTLS
# include <gnutls/gnutls.h>
# include <gcrypt.h>
#endif /* ifdef HAVE_GNUTLS */

#ifdef HAVE_OPENSSL
# include <openssl/err.h>
# include <openssl/evp.h>
#endif /* ifdef HAVE_OPENSSL */

#ifdef EFL_HAVE_POSIX_THREADS
# include <pthread.h>
# ifdef HAVE_GNUTLS
GCRY_THREAD_OPTION_PTHREAD_IMPL;
# endif /* ifdef HAVE_GNUTLS */
#endif /* ifdef EFL_HAVE_POSIX_THREADS */

#include <Eina.h>

#include "Eet.h"
#include "Eet_private.h"

static Eet_Version _version = { VMAJ, VMIN, VMIC, VREV };
EAPI Eet_Version *eet_version = &_version;

#ifdef HAVE_REALPATH
# undef HAVE_REALPATH
#endif /* ifdef HAVE_REALPATH */

#define EET_MAGIC_FILE        0x1ee7ff00
#define EET_MAGIC_FILE_HEADER 0x1ee7ff01

#define EET_MAGIC_FILE2       0x1ee70f42

typedef struct _Eet_File_Header      Eet_File_Header;
typedef struct _Eet_File_Node        Eet_File_Node;
typedef struct _Eet_File_Directory   Eet_File_Directory;

struct _Eet_File
{
   char                *path;
   FILE                *readfp;
   Eet_File_Header     *header;
   Eet_Dictionary      *ed;
   Eet_Key             *key;
   const unsigned char *data;
   const void          *x509_der;
   const void          *signature;
   void                *sha1;

   Eet_File_Mode        mode;

   int                  magic;
   int                  references;

   int                  data_size;
   int                  x509_length;
   unsigned int         signature_length;
   int                  sha1_length;

   time_t               mtime;

#ifdef EFL_HAVE_THREADS
# ifdef EFL_HAVE_POSIX_THREADS
   pthread_mutex_t      file_lock;
# else /* ifdef EFL_HAVE_POSIX_THREADS */
   HANDLE               file_lock;
# endif /* ifdef EFL_HAVE_POSIX_THREADS */
#endif /* ifdef EFL_HAVE_THREADS */

   unsigned char        writes_pending : 1;
   unsigned char        delete_me_now : 1;
};

struct _Eet_File_Header
{
   int                 magic;
   Eet_File_Directory *directory;
};

struct _Eet_File_Directory
{
   int             size;
   Eet_File_Node **nodes;
};

struct _Eet_File_Node
{
   char          *name;
   void          *data;
   Eet_File_Node *next;  /* FIXME: make buckets linked lists */

   int            offset;
   int            dictionary_offset;
   int            name_offset;

   int            name_size;
   int            size;
   int            data_size;

   unsigned char  free_name : 1;
   unsigned char  compression : 1;
   unsigned char  ciphered : 1;
   unsigned char  alias : 1;
};

#if 0
/* Version 2 */
/* NB: all int's are stored in network byte order on disk */
/* file format: */
int magic; /* magic number ie 0x1ee7ff00 */
int num_directory_entries; /* number of directory entries to follow */
int bytes_directory_entries; /* bytes of directory entries to follow */
struct
{
   int  offset; /* bytes offset into file for data chunk */
   int  flags; /* flags - for now 0 = uncompressed and clear, 1 = compressed and clear, 2 = uncompressed and ciphered, 3 = compressed and ciphered */
   int  size; /* size of the data chunk */
   int  data_size; /* size of the (uncompressed) data chunk */
   int  name_size; /* length in bytes of the name field */
   char name[name_size]; /* name string (variable length) and \0 terminated */
} directory[num_directory_entries];
/* and now startes the data stream... */
#endif /* if 0 */

#if 0
/* Version 3 */
/* NB: all int's are stored in network byte order on disk */
/* file format: */
int magic; /* magic number ie 0x1ee70f42 */
int num_directory_entries; /* number of directory entries to follow */
int num_dictionary_entries; /* number of dictionary entries to follow */
struct
{
   int data_offset; /* bytes offset into file for data chunk */
   int size; /* size of the data chunk */
   int data_size; /* size of the (uncompressed) data chunk */
   int name_offset; /* bytes offset into file for name string */
   int name_size; /* length in bytes of the name field */
   int flags; /* bit flags - for now:
                 bit 0 => compresion on/off
                 bit 1 => ciphered on/off
                 bit 2 => alias
               */
} directory[num_directory_entries];
struct
{
   int hash;
   int offset;
   int size;
   int prev;
   int next;
} dictionary[num_dictionary_entries];
/* now start the string stream. */
/* and right after them the data stream. */
int magic_sign; /* Optional, only if the eet file is signed. */
int signature_length; /* Signature length. */
int x509_length; /* Public certificate that signed the file. */
char signature[signature_length]; /* The signature. */
char x509[x509_length]; /* The public certificate. */
#endif /* if 0 */

#define EET_FILE2_HEADER_COUNT           3
#define EET_FILE2_DIRECTORY_ENTRY_COUNT  6
#define EET_FILE2_DICTIONARY_ENTRY_COUNT 5

#define EET_FILE2_HEADER_SIZE            (sizeof(int) *\
                                          EET_FILE2_HEADER_COUNT)
#define EET_FILE2_DIRECTORY_ENTRY_SIZE   (sizeof(int) *\
                                          EET_FILE2_DIRECTORY_ENTRY_COUNT)
#define EET_FILE2_DICTIONARY_ENTRY_SIZE  (sizeof(int) *\
                                          EET_FILE2_DICTIONARY_ENTRY_COUNT)

/* prototypes of internal calls */
static Eet_File *         eet_cache_find(const char *path,
                                         Eet_File  **cache,
                                         int         cache_num);
static void               eet_cache_add(Eet_File   *ef,
                                        Eet_File ***cache,
                                        int        *cache_num,
                                        int        *cache_alloc);
static void               eet_cache_del(Eet_File   *ef,
                                        Eet_File ***cache,
                                        int        *cache_num,
                                        int        *cache_alloc);
static int                eet_string_match(const char *s1, const char *s2);
#if 0 /* Unused */
static Eet_Error          eet_flush(Eet_File *ef);
#endif /* if 0 */
static Eet_Error          eet_flush2(Eet_File *ef);
static Eet_File_Node *    find_node_by_name(Eet_File *ef, const char *name);
static int                read_data_from_disk(Eet_File      *ef,
                                              Eet_File_Node *efn,
                                              void          *buf,
                                              int            len);

static Eet_Error          eet_internal_close(Eet_File *ef, Eina_Bool locked);

#ifdef EFL_HAVE_THREADS

# ifdef EFL_HAVE_POSIX_THREADS

static pthread_mutex_t eet_cache_lock = PTHREAD_MUTEX_INITIALIZER;

#  define LOCK_CACHE   pthread_mutex_lock(&eet_cache_lock)
#  define UNLOCK_CACHE pthread_mutex_unlock(&eet_cache_lock)

#  define INIT_FILE(File)    pthread_mutex_init(&File->file_lock, NULL)
#  define LOCK_FILE(File)    pthread_mutex_lock(&File->file_lock)
#  define UNLOCK_FILE(File)  pthread_mutex_unlock(&File->file_lock)
#  define DESTROY_FILE(File) pthread_mutex_destroy(&File->file_lock)

# else /* EFL_HAVE_WIN32_THREADS */

static HANDLE eet_cache_lock = NULL;

#  define LOCK_CACHE   WaitForSingleObject(eet_cache_lock, INFINITE)
#  define UNLOCK_CACHE ReleaseMutex(eet_cache_lock)

#  define INIT_FILE(File)    File->file_lock = CreateMutex(NULL, FALSE, NULL)
#  define LOCK_FILE(File)    WaitForSingleObject(File->file_lock, INFINITE)
#  define UNLOCK_FILE(File)  ReleaseMutex(File->file_lock)
#  define DESTROY_FILE(File) CloseHandle(File->file_lock)

# endif /* EFL_HAVE_WIN32_THREADS */

#else /* ifdef EFL_HAVE_THREADS */

# define LOCK_CACHE   do {} while (0)
# define UNLOCK_CACHE do {} while (0)

# define INIT_FILE(File)    do {} while (0)
# define LOCK_FILE(File)    do {} while (0)
# define UNLOCK_FILE(File)  do {} while (0)
# define DESTROY_FILE(File) do {} while (0)

#endif /* EFL_HAVE_THREADS */

/* cache. i don't expect this to ever be large, so arrays will do */
static int eet_writers_num = 0;
static int eet_writers_alloc = 0;
static Eet_File **eet_writers = NULL;
static int eet_readers_num = 0;
static int eet_readers_alloc = 0;
static Eet_File **eet_readers = NULL;
static int eet_init_count = 0;

/* log domain variable */
int _eet_log_dom_global = -1;

/* Check to see its' an eet file pointer */
static inline int
eet_check_pointer(const Eet_File *ef)
{
   if ((!ef) || (ef->magic != EET_MAGIC_FILE))
      return 1;

   return 0;
} /* eet_check_pointer */

static inline int
eet_check_header(const Eet_File *ef)
{
   if (!ef->header)
      return 1;

   if (!ef->header->directory)
      return 1;

   return 0;
} /* eet_check_header */

static inline int
eet_test_close(int       test,
               Eet_File *ef)
{
   if (test)
     {
        ef->delete_me_now = 1;
        eet_internal_close(ef, EINA_TRUE);
     }

   return test;
} /* eet_test_close */

/* find an eet file in the currently in use cache */
static Eet_File *
eet_cache_find(const char *path,
               Eet_File  **cache,
               int         cache_num)
{
   int i;

   /* walk list */
   for (i = 0; i < cache_num; i++)
     {
        /* if matches real path - return it */
        if (eet_string_match(cache[i]->path, path))
           if (!cache[i]->delete_me_now)
              return cache[i];

     }

   /* not found */
   return NULL;
} /* eet_cache_find */

/* add to end of cache */
/* this should only be called when the cache lock is already held */
static void
eet_cache_add(Eet_File   *ef,
              Eet_File ***cache,
              int        *cache_num,
              int        *cache_alloc)
{
   Eet_File **new_cache;
   int new_cache_num;
   int new_cache_alloc;

   new_cache_num = *cache_num;
   if (new_cache_num >= 64) /* avoid fd overruns - limit to 128 (most recent) in the cache */
     {
        Eet_File *del_ef = NULL;
        int i;

        new_cache = *cache;
        for (i = 0; i < new_cache_num; i++)
          {
             if (new_cache[i]->references == 0)
               {
                  del_ef = new_cache[i];
                  break;
               }
          }

        if (del_ef)
          {
             del_ef->delete_me_now = 1;
             eet_internal_close(del_ef, EINA_TRUE);
          }
     }

   new_cache = *cache;
   new_cache_num = *cache_num;
   new_cache_alloc = *cache_alloc;
   new_cache_num++;
   if (new_cache_num > new_cache_alloc)
     {
        new_cache_alloc += 16;
        new_cache = realloc(new_cache, new_cache_alloc * sizeof(Eet_File *));
        if (!new_cache)
          {
             CRIT("BAD ERROR! Eet realloc of cache list failed. Abort");
             abort();
          }
     }

   new_cache[new_cache_num - 1] = ef;
   *cache = new_cache;
   *cache_num = new_cache_num;
   *cache_alloc = new_cache_alloc;
} /* eet_cache_add */

/* delete from cache */
/* this should only be called when the cache lock is already held */
static void
eet_cache_del(Eet_File   *ef,
              Eet_File ***cache,
              int        *cache_num,
              int        *cache_alloc)
{
   Eet_File **new_cache;
   int new_cache_num, new_cache_alloc;
   int i, j;

   new_cache = *cache;
   new_cache_num = *cache_num;
   new_cache_alloc = *cache_alloc;
   if (new_cache_num <= 0)
      return;

   for (i = 0; i < new_cache_num; i++)
     {
        if (new_cache[i] == ef)
           break;
     }

   if (i >= new_cache_num)
      return;

   new_cache_num--;
   for (j = i; j < new_cache_num; j++)
      new_cache[j] = new_cache[j + 1];

   if (new_cache_num <= (new_cache_alloc - 16))
     {
        new_cache_alloc -= 16;
        if (new_cache_num > 0)
          {
             new_cache = realloc(new_cache, new_cache_alloc * sizeof(Eet_File *));
             if (!new_cache)
               {
                  CRIT("BAD ERROR! Eet realloc of cache list failed. Abort");
                  abort();
               }
          }
        else
          {
             free(new_cache);
             new_cache = NULL;
          }
     }

   *cache = new_cache;
   *cache_num = new_cache_num;
   *cache_alloc = new_cache_alloc;
} /* eet_cache_del */

/* internal string match. null friendly, catches same ptr */
static int
eet_string_match(const char *s1,
                 const char *s2)
{
   /* both null- no match */
   if ((!s1) || (!s2))
      return 0;

   if (s1 == s2)
      return 1;

   return (!strcmp(s1, s2));
} /* eet_string_match */

/* flush out writes to a v2 eet file */
static Eet_Error
eet_flush2(Eet_File *ef)
{
   Eet_File_Node *efn;
   FILE *fp;
   Eet_Error error = EET_ERROR_NONE;
   int head[EET_FILE2_HEADER_COUNT];
   int num_directory_entries = 0;
   int num_dictionary_entries = 0;
   int bytes_directory_entries = 0;
   int bytes_dictionary_entries = 0;
   int bytes_strings = 0;
   int data_offset = 0;
   int strings_offset = 0;
   int num;
   int i;
   int j;

   if (eet_check_pointer(ef))
      return EET_ERROR_BAD_OBJECT;

   if (eet_check_header(ef))
      return EET_ERROR_EMPTY;

   if (!ef->writes_pending)
      return EET_ERROR_NONE;

   if ((ef->mode == EET_FILE_MODE_READ_WRITE)
       || (ef->mode == EET_FILE_MODE_WRITE))
     {
        int fd;

        /* opening for write - delete old copy of file right away */
        unlink(ef->path);
        fd = open(ef->path, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
        fp = fdopen(fd, "wb");
        if (!fp)
           return EET_ERROR_NOT_WRITABLE;

        fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
     }
   else
      return EET_ERROR_NOT_WRITABLE;

   /* calculate string base offset and data base offset */
   num = (1 << ef->header->directory->size);
   for (i = 0; i < num; ++i)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             num_directory_entries++;
             bytes_strings += strlen(efn->name) + 1;
          }
     }
   if (ef->ed)
     {
        num_dictionary_entries = ef->ed->count;

        for (i = 0; i < num_dictionary_entries; ++i)
           bytes_strings += ef->ed->all[i].len;
     }

   /* calculate section bytes size */
   bytes_directory_entries = EET_FILE2_DIRECTORY_ENTRY_SIZE *
      num_directory_entries + EET_FILE2_HEADER_SIZE;
   bytes_dictionary_entries = EET_FILE2_DICTIONARY_ENTRY_SIZE *
      num_dictionary_entries;

   /* calculate per entry offset */
   strings_offset = bytes_directory_entries + bytes_dictionary_entries;
   data_offset = bytes_directory_entries + bytes_dictionary_entries +
      bytes_strings;

   for (i = 0; i < num; ++i)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             efn->offset = data_offset;
             data_offset += efn->size;

             efn->name_offset = strings_offset;
             strings_offset += efn->name_size;
          }
     }

   /* calculate dictionary strings offset */
   if (ef->ed)
      ef->ed->offset = strings_offset;

   /* go thru and write the header */
   head[0] = (int)htonl((unsigned int)EET_MAGIC_FILE2);
   head[1] = (int)htonl((unsigned int)num_directory_entries);
   head[2] = (int)htonl((unsigned int)num_dictionary_entries);

   fseek(fp, 0, SEEK_SET);
   if (fwrite(head, sizeof (head), 1, fp) != 1)
      goto write_error;

   /* write directories entry */
   for (i = 0; i < num; i++)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             unsigned int flag;
             int ibuf[EET_FILE2_DIRECTORY_ENTRY_COUNT];

             flag = (efn->alias << 2) | (efn->ciphered << 1) | efn->compression;

             ibuf[0] = (int)htonl((unsigned int)efn->offset);
             ibuf[1] = (int)htonl((unsigned int)efn->size);
             ibuf[2] = (int)htonl((unsigned int)efn->data_size);
             ibuf[3] = (int)htonl((unsigned int)efn->name_offset);
             ibuf[4] = (int)htonl((unsigned int)efn->name_size);
             ibuf[5] = (int)htonl((unsigned int)flag);

             if (fwrite(ibuf, sizeof(ibuf), 1, fp) != 1)
                goto write_error;
          }
     }

   /* write dictionnary */
   if (ef->ed)
     {
        int offset = strings_offset;

        for (j = 0; j < ef->ed->count; ++j)
          {
             int sbuf[EET_FILE2_DICTIONARY_ENTRY_COUNT];

             sbuf[0] = (int)htonl((unsigned int)ef->ed->all[j].hash);
             sbuf[1] = (int)htonl((unsigned int)offset);
             sbuf[2] = (int)htonl((unsigned int)ef->ed->all[j].len);
             sbuf[3] = (int)htonl((unsigned int)ef->ed->all[j].prev);
             sbuf[4] = (int)htonl((unsigned int)ef->ed->all[j].next);

             offset += ef->ed->all[j].len;

             if (fwrite(sbuf, sizeof (sbuf), 1, fp) != 1)
                goto write_error;
          }
     }

   /* write directories name */
   for (i = 0; i < num; i++)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             if (fwrite(efn->name, efn->name_size, 1, fp) != 1)
                goto write_error;
          }
     }

   /* write strings */
   if (ef->ed)
      for (j = 0; j < ef->ed->count; ++j)
        {
           if (fwrite(ef->ed->all[j].str, ef->ed->all[j].len, 1, fp) != 1)
             goto write_error;
        }

   /* write data */
   for (i = 0; i < num; i++)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             if (fwrite(efn->data, efn->size, 1, fp) != 1)
                goto write_error;
          }
     }

   /* flush all write to the file. */
   fflush(fp);
// this is going to really cause trouble. if ANYTHING this needs to go into a
// thread spawned off - but even then...
// in this case... ext4 is "wrong". (yes we can jump up and down and point posix
// manual pages at eachother, but ext4 broke behavior that has been in place
// for decades and that 1000's of apps rely on daily - that is that one operation
// to disk is committed to disk BEFORE following operations, so the fs retains
// a consistent state
//   fsync(fileno(fp));

   /* append signature if required */
   if (ef->key)
     {
        error = eet_identity_sign(fp, ef->key);
        if (error != EET_ERROR_NONE)
           goto sign_error;
     }

   /* no more writes pending */
   ef->writes_pending = 0;

   fclose(fp);

   return EET_ERROR_NONE;

write_error:
   if (ferror(fp))
     {
        switch (errno)
          {
           case EFBIG: error = EET_ERROR_WRITE_ERROR_FILE_TOO_BIG; break;

           case EIO: error = EET_ERROR_WRITE_ERROR_IO_ERROR; break;

           case ENOSPC: error = EET_ERROR_WRITE_ERROR_OUT_OF_SPACE; break;

           case EPIPE: error = EET_ERROR_WRITE_ERROR_FILE_CLOSED; break;

           default: error = EET_ERROR_WRITE_ERROR; break;
          } /* switch */
     }

sign_error:
   fclose(fp);
   return error;
} /* eet_flush2 */

EAPI int
eet_init(void)
{
   if (++eet_init_count != 1)
      return eet_init_count;

   if (!eina_init())
     {
        fprintf(stderr, "Eet: Eina init failed");
        return --eet_init_count;
     }

   _eet_log_dom_global = eina_log_domain_register("eet", EET_DEFAULT_LOG_COLOR);
   if (_eet_log_dom_global < 0)
     {
        EINA_LOG_ERR("Eet Can not create a general log domain.");
        goto shutdown_eina;
     }

   if (!eet_node_init())
     {
        EINA_LOG_ERR("Eet: Eet_Node mempool creation failed");
        goto unregister_log_domain;
     }

#ifdef HAVE_GNUTLS
   /* Before the library can be used, it must initialize itself if needed. */
   if (gcry_control(GCRYCTL_ANY_INITIALIZATION_P) == 0)
     {
        gcry_check_version(NULL);
        /* Disable warning messages about problems with the secure memory subsystem.
           This command should be run right after gcry_check_version. */
        if (gcry_control(GCRYCTL_DISABLE_SECMEM_WARN))
           goto shutdown_eet;  /* This command is used to allocate a pool of secure memory and thus
                                  enabling the use of secure memory. It also drops all extra privileges the
                                  process has (i.e. if it is run as setuid (root)). If the argument nbytes
                                  is 0, secure memory will be disabled. The minimum amount of secure memory
                                  allocated is currently 16384 bytes; you may thus use a value of 1 to
                                  request that default size. */

        if (gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0))
           WRN(
              "BIG FAT WARNING: I AM UNABLE TO REQUEST SECMEM, Cryptographic operation are at risk !");
     }

# ifdef EFL_HAVE_POSIX_THREADS
   if (gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread))
      WRN(
         "YOU ARE USING PTHREADS, BUT I CANNOT INITIALIZE THREADSAFE GCRYPT OPERATIONS!");

# endif /* ifdef EFL_HAVE_POSIX_THREADS */
   if (gnutls_global_init())
      goto shutdown_eet;

#endif /* ifdef HAVE_GNUTLS */
#ifdef HAVE_OPENSSL
   ERR_load_crypto_strings();
   OpenSSL_add_all_algorithms();
#endif /* ifdef HAVE_OPENSSL */

   return eet_init_count;

#ifdef HAVE_GNUTLS
shutdown_eet:
#endif   
   eet_node_shutdown();
unregister_log_domain:
   eina_log_domain_unregister(_eet_log_dom_global);
   _eet_log_dom_global = -1;
shutdown_eina:
   eina_shutdown();
   return --eet_init_count;
} /* eet_init */

EAPI int
eet_shutdown(void)
{
   if (--eet_init_count != 0)
      return eet_init_count;

   eet_clearcache();
   eet_node_shutdown();
#ifdef HAVE_GNUTLS
   gnutls_global_deinit();
#endif /* ifdef HAVE_GNUTLS */
#ifdef HAVE_OPENSSL
   EVP_cleanup();
   ERR_free_strings();
#endif /* ifdef HAVE_OPENSSL */
   eina_log_domain_unregister(_eet_log_dom_global);
   _eet_log_dom_global = -1;
   eina_shutdown();

   return eet_init_count;
} /* eet_shutdown */

EAPI Eet_Error
eet_sync(Eet_File *ef)
{
   Eet_Error ret;

   if (eet_check_pointer(ef))
      return EET_ERROR_BAD_OBJECT;

   if ((ef->mode != EET_FILE_MODE_WRITE) &&
       (ef->mode != EET_FILE_MODE_READ_WRITE))
      return EET_ERROR_NOT_WRITABLE;

   if (!ef->writes_pending)
      return EET_ERROR_NONE;

   LOCK_FILE(ef);

   ret = eet_flush2(ef);

   UNLOCK_FILE(ef);
   return ret;
} /* eet_sync */

EAPI void
eet_clearcache(void)
{
   int num = 0;
   int i;

   /*
    * We need to compute the list of eet file to close separately from the cache,
    * due to eet_close removing them from the cache after each call.
    */
   LOCK_CACHE;
   for (i = 0; i < eet_writers_num; i++)
     {
        if (eet_writers[i]->references <= 0)
           num++;
     }

   for (i = 0; i < eet_readers_num; i++)
     {
        if (eet_readers[i]->references <= 0)
           num++;
     }

   if (num > 0)
     {
        Eet_File **closelist = NULL;

        closelist = alloca(num * sizeof(Eet_File *));
        num = 0;
        for (i = 0; i < eet_writers_num; i++)
          {
             if (eet_writers[i]->references <= 0)
               {
                  closelist[num] = eet_writers[i];
                  eet_writers[i]->delete_me_now = 1;
                  num++;
               }
          }

        for (i = 0; i < eet_readers_num; i++)
          {
             if (eet_readers[i]->references <= 0)
               {
                  closelist[num] = eet_readers[i];
                  eet_readers[i]->delete_me_now = 1;
                  num++;
               }
          }

        for (i = 0; i < num; i++)
          {
             eet_internal_close(closelist[i], EINA_TRUE);
          }
     }

   UNLOCK_CACHE;
} /* eet_clearcache */

/* FIXME: MMAP race condition in READ_WRITE_MODE */
static Eet_File *
eet_internal_read2(Eet_File *ef)
{
   const int *data = (const int *)ef->data;
   const char *start = (const char *)ef->data;
   int idx = 0;
   int num_directory_entries;
   int bytes_directory_entries;
   int num_dictionary_entries;
   int bytes_dictionary_entries;
   int signature_base_offset;
   int i;

   idx += sizeof(int);
   if (eet_test_close((int)ntohl(*data) != EET_MAGIC_FILE2, ef))
      return NULL;

   data++;

#define GET_INT(Value, Pointer, Index)\
   {\
      Value = ntohl(*Pointer);\
      Pointer++;\
      Index += sizeof(int);\
   }

   /* get entries count and byte count */
   GET_INT(num_directory_entries,  data, idx);
   /* get dictionary count and byte count */
   GET_INT(num_dictionary_entries, data, idx);

   bytes_directory_entries = EET_FILE2_DIRECTORY_ENTRY_SIZE *
      num_directory_entries + EET_FILE2_HEADER_SIZE;
   bytes_dictionary_entries = EET_FILE2_DICTIONARY_ENTRY_SIZE *
      num_dictionary_entries;

   /* we can't have <= 0 values here - invalid */
   if (eet_test_close((num_directory_entries <= 0), ef))
      return NULL;

   /* we can't have more bytes directory and bytes in dictionaries than the size of the file */
   if (eet_test_close((bytes_directory_entries + bytes_dictionary_entries) >
                      ef->data_size, ef))
      return NULL;

   /* allocate header */
   ef->header = calloc(1, sizeof(Eet_File_Header));
   if (eet_test_close(!ef->header, ef))
      return NULL;

   ef->header->magic = EET_MAGIC_FILE_HEADER;

   /* allocate directory block in ram */
   ef->header->directory = calloc(1, sizeof(Eet_File_Directory));
   if (eet_test_close(!ef->header->directory, ef))
      return NULL;

   /* 8 bit hash table (256 buckets) */
   ef->header->directory->size = 8;
   /* allocate base hash table */
   ef->header->directory->nodes =
      calloc(1, sizeof(Eet_File_Node *) * (1 << ef->header->directory->size));
   if (eet_test_close(!ef->header->directory->nodes, ef))
      return NULL;

   signature_base_offset = 0;

   /* actually read the directory block - all of it, into ram */
   for (i = 0; i < num_directory_entries; ++i)
     {
        const char *name;
        Eet_File_Node *efn;
        int name_offset;
        int name_size;
        int hash;
        int flag;

        /* out directory block is inconsistent - we have oveerun our */
        /* dynamic block buffer before we finished scanning dir entries */
        efn = malloc(sizeof(Eet_File_Node));
        if (eet_test_close(!efn, ef))
          {
             if (efn) free(efn); /* yes i know - we only get here if 
                                  * efn is null/0 -> trying to shut up
                                  * warning tools like cppcheck */
             return NULL;
          }

        /* get entrie header */
        GET_INT(efn->offset,    data, idx);
        GET_INT(efn->size,      data, idx);
        GET_INT(efn->data_size, data, idx);
        GET_INT(name_offset,    data, idx);
        GET_INT(name_size,      data, idx);
        GET_INT(flag,           data, idx);

        efn->compression = flag & 0x1 ? 1 : 0;
        efn->ciphered = flag & 0x2 ? 1 : 0;
        efn->alias = flag & 0x4 ? 1 : 0;

#define EFN_TEST(Test, Ef, Efn)\
   if (eet_test_close(Test, Ef))\
     {\
        free(Efn);\
        return NULL;\
     }

        /* check data pointer position */
        EFN_TEST(!((efn->size > 0)
                   && (efn->offset + efn->size <= ef->data_size)
                   && (efn->offset > bytes_dictionary_entries +
                       bytes_directory_entries)), ef, efn);

        /* check name position */
        EFN_TEST(!((name_size > 0)
                   && (name_offset + name_size < ef->data_size)
                   && (name_offset >= bytes_dictionary_entries +
                       bytes_directory_entries)), ef, efn);

        name = start + name_offset;

        /* check '\0' at the end of name string */
        EFN_TEST(name[name_size - 1] != '\0', ef, efn);

        efn->free_name = 0;
        efn->name = (char *)name;
        efn->name_size = name_size;

        hash = _eet_hash_gen(efn->name, ef->header->directory->size);
        efn->next = ef->header->directory->nodes[hash];
        ef->header->directory->nodes[hash] = efn;

        /* read-only mode, so currently we have no data loaded */
        if (ef->mode == EET_FILE_MODE_READ)
           efn->data = NULL;  /* read-write mode - read everything into ram */
        else
          {
             efn->data = malloc(efn->size);
             if (efn->data)
                memcpy(efn->data, ef->data + efn->offset, efn->size);
          }

        /* compute the possible position of a signature */
        if (signature_base_offset < efn->offset + efn->size)
           signature_base_offset = efn->offset + efn->size;
     }

   ef->ed = NULL;

   if (num_dictionary_entries)
     {
        const int *dico = (const int *)ef->data +
           EET_FILE2_DIRECTORY_ENTRY_COUNT * num_directory_entries +
           EET_FILE2_HEADER_COUNT;
        int j;

        if (eet_test_close((num_dictionary_entries *
                            (int)EET_FILE2_DICTIONARY_ENTRY_SIZE + idx) >
                           (bytes_dictionary_entries + bytes_directory_entries),
                           ef))
           return NULL;

        ef->ed = calloc(1, sizeof (Eet_Dictionary));
        if (eet_test_close(!ef->ed, ef))
           return NULL;

        ef->ed->all = calloc(num_dictionary_entries, sizeof (Eet_String));
        if (eet_test_close(!ef->ed->all, ef))
           return NULL;

        ef->ed->count = num_dictionary_entries;
        ef->ed->total = num_dictionary_entries;
        ef->ed->start = start + bytes_dictionary_entries +
           bytes_directory_entries;
        ef->ed->end = ef->ed->start;

        for (j = 0; j < ef->ed->count; ++j)
          {
             int hash;
             int offset;

             GET_INT(hash,                dico, idx);
             GET_INT(offset,              dico, idx);
             GET_INT(ef->ed->all[j].len,  dico, idx);
             GET_INT(ef->ed->all[j].prev, dico, idx);
             GET_INT(ef->ed->all[j].next, dico, idx);

             /* Hash value could be stored on 8bits data, but this will break alignment of all the others data.
                So stick to int and check the value. */
             if (eet_test_close(hash & 0xFFFFFF00, ef))
                return NULL;

             /* Check string position */
             if (eet_test_close(!((ef->ed->all[j].len > 0)
                                  && (offset >
                                      (bytes_dictionary_entries +
                                       bytes_directory_entries))
                                  && (offset + ef->ed->all[j].len <
                                      ef->data_size)), ef))
                return NULL;

             ef->ed->all[j].str = start + offset;

             if (ef->ed->all[j].str + ef->ed->all[j].len > ef->ed->end)
                ef->ed->end = ef->ed->all[j].str + ef->ed->all[j].len;

             /* Check '\0' at the end of the string */
             if (eet_test_close(ef->ed->all[j].str[ef->ed->all[j].len - 1] !=
                                '\0', ef))
                return NULL;

             ef->ed->all[j].hash = hash;
             if (ef->ed->all[j].prev == -1)
                ef->ed->hash[hash] = j;

             /* compute the possible position of a signature */
             if (signature_base_offset < offset + ef->ed->all[j].len)
                signature_base_offset = offset + ef->ed->all[j].len;
          }
     }

   /* Check if the file is signed */
   ef->x509_der = NULL;
   ef->x509_length = 0;
   ef->signature = NULL;
   ef->signature_length = 0;

   if (signature_base_offset < ef->data_size)
     {
#ifdef HAVE_SIGNATURE
        const unsigned char *buffer = ((const unsigned char *)ef->data) +
           signature_base_offset;
        ef->x509_der = eet_identity_check(ef->data,
                                          signature_base_offset,
                                          &ef->sha1,
                                          &ef->sha1_length,
                                          buffer,
                                          ef->data_size - signature_base_offset,
                                          &ef->signature,
                                          &ef->signature_length,
                                          &ef->x509_length);

        if (eet_test_close(!ef->x509_der, ef))
           return NULL;

#else /* ifdef HAVE_SIGNATURE */
        ERR(
           "This file could be signed but you didn't compile the necessary code to check the signature.");
#endif /* ifdef HAVE_SIGNATURE */
     }

   return ef;
} /* eet_internal_read2 */

#if EET_OLD_EET_FILE_FORMAT
static Eet_File *
eet_internal_read1(Eet_File *ef)
{
   const unsigned char *dyn_buf = NULL;
   const unsigned char *p = NULL;
   int idx = 0;
   int num_entries;
   int byte_entries;
   int i;

   WRN(
      "EET file format of '%s' is deprecated. You should just open it one time with mode == EET_FILE_MODE_READ_WRITE to solve this issue.",
      ef->path);

   /* build header table if read mode */
   /* geat header */
   idx += sizeof(int);
   if (eet_test_close((int)ntohl(*((int *)ef->data)) != EET_MAGIC_FILE, ef))
      return NULL;

#define EXTRACT_INT(Value, Pointer, Index)\
   {\
      int tmp;\
      memcpy(&tmp, Pointer + Index, sizeof(int));\
      Value = ntohl(tmp);\
      Index += sizeof(int);\
   }

   /* get entries count and byte count */
   EXTRACT_INT(num_entries,  ef->data, idx);
   EXTRACT_INT(byte_entries, ef->data, idx);

   /* we can't have <= 0 values here - invalid */
   if (eet_test_close((num_entries <= 0) || (byte_entries <= 0), ef))
      return NULL;

   /* we can't have more entires than minimum bytes for those! invalid! */
   if (eet_test_close((num_entries * 20) > byte_entries, ef))
      return NULL;

   /* check we will not outrun the file limit */
   if (eet_test_close(((byte_entries + (int)sizeof(int) * 3) > ef->data_size),
                      ef))
      return NULL;

   /* allocate header */
   ef->header = calloc(1, sizeof(Eet_File_Header));
   if (eet_test_close(!ef->header, ef))
      return NULL;

   ef->header->magic = EET_MAGIC_FILE_HEADER;

   /* allocate directory block in ram */
   ef->header->directory = calloc(1, sizeof(Eet_File_Directory));
   if (eet_test_close(!ef->header->directory, ef))
      return NULL;

   /* 8 bit hash table (256 buckets) */
   ef->header->directory->size = 8;
   /* allocate base hash table */
   ef->header->directory->nodes =
      calloc(1, sizeof(Eet_File_Node *) * (1 << ef->header->directory->size));
   if (eet_test_close(!ef->header->directory->nodes, ef))
      return NULL;

   /* actually read the directory block - all of it, into ram */
   dyn_buf = ef->data + idx;

   /* parse directory block */
   p = dyn_buf;

   for (i = 0; i < num_entries; i++)
     {
        Eet_File_Node *efn;
        void *data = NULL;
        int indexn = 0;
        int name_size;
        int hash;
        int k;

#define HEADER_SIZE (sizeof(int) * 5)

        /* out directory block is inconsistent - we have oveerun our */
        /* dynamic block buffer before we finished scanning dir entries */
        if (eet_test_close(p + HEADER_SIZE >= (dyn_buf + byte_entries), ef))
           return NULL;

        /* allocate all the ram needed for this stored node accounting */
        efn = malloc (sizeof(Eet_File_Node));
        if (eet_test_close(!efn, ef))
          {
             if (efn) free(efn); /* yes i know - we only get here if 
                                  * efn is null/0 -> trying to shut up
                                  * warning tools like cppcheck */
             return NULL;
          }

        /* get entrie header */
        EXTRACT_INT(efn->offset,      p, indexn);
        EXTRACT_INT(efn->compression, p, indexn);
        EXTRACT_INT(efn->size,        p, indexn);
        EXTRACT_INT(efn->data_size,   p, indexn);
        EXTRACT_INT(name_size,        p, indexn);

        efn->name_size = name_size;
        efn->ciphered = 0;
        efn->alias = 0;

        /* invalid size */
        if (eet_test_close(efn->size <= 0, ef))
          {
             free(efn);
             return NULL;
          }

        /* invalid name_size */
        if (eet_test_close(name_size <= 0, ef))
          {
             free(efn);
             return NULL;
          }

        /* reading name would mean falling off end of dyn_buf - invalid */
        if (eet_test_close((p + 16 + name_size) > (dyn_buf + byte_entries), ef))
          {
             free(efn);
             return NULL;
          }

        /* This code is useless if we dont want backward compatibility */
        for (k = name_size;
             k > 0 && ((unsigned char)*(p + HEADER_SIZE + k)) != 0; --k)
           ;

        efn->free_name = ((unsigned char)*(p + HEADER_SIZE + k)) != 0;

        if (efn->free_name)
          {
             efn->name = malloc(sizeof(char) * name_size + 1);
             if (eet_test_close(!efn->name, ef))
               {
                  free(efn);
                  return NULL;
               }

             strncpy(efn->name, (char *)p + HEADER_SIZE, name_size);
             efn->name[name_size] = 0;

             WRN(
                "File: %s is not up to date for key \"%s\" - needs rebuilding sometime",
                ef->path,
                efn->name);
          }
        else
           /* The only really useful peace of code for efn->name (no backward compatibility) */
           efn->name = (char *)((unsigned char *)(p + HEADER_SIZE));

        /* get hash bucket it should go in */
        hash = _eet_hash_gen(efn->name, ef->header->directory->size);
        efn->next = ef->header->directory->nodes[hash];
        ef->header->directory->nodes[hash] = efn;

        /* read-only mode, so currently we have no data loaded */
        if (ef->mode == EET_FILE_MODE_READ)
           efn->data = NULL;  /* read-write mode - read everything into ram */
        else
          {
             data = malloc(efn->size);
             if (data)
                memcpy(data, ef->data + efn->offset, efn->size);

             efn->data = data;
          }

        /* advance */
        p += HEADER_SIZE + name_size;
     }
   return ef;
} /* eet_internal_read1 */

#endif /* if EET_OLD_EET_FILE_FORMAT */

/*
 * this should only be called when the cache lock is already held
 * (We could drop this restriction if we add a parameter to eet_test_close
 * that indicates if the lock is held or not.  For now it is easiest
 * to just require that it is always held.)
 */
static Eet_File *
eet_internal_read(Eet_File *ef)
{
   const int *data = (const int *)ef->data;

   if (eet_test_close((ef->data == (void *)-1) || (!ef->data), ef))
      return NULL;

   if (eet_test_close(ef->data_size < (int)sizeof(int) * 3, ef))
      return NULL;

   switch (ntohl(*data))
     {
#if EET_OLD_EET_FILE_FORMAT
      case EET_MAGIC_FILE:
         return eet_internal_read1(ef);

#endif /* if EET_OLD_EET_FILE_FORMAT */
      case EET_MAGIC_FILE2:
         return eet_internal_read2(ef);

      default:
         ef->delete_me_now = 1;
         eet_internal_close(ef, EINA_TRUE);
         break;
     } /* switch */

   return NULL;
} /* eet_internal_read */

static Eet_Error
eet_internal_close(Eet_File *ef,
                   Eina_Bool locked)
{
   Eet_Error err;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return EET_ERROR_BAD_OBJECT;

   if (!locked)
      LOCK_CACHE;

   /* deref */
   ef->references--;
   /* if its still referenced - dont go any further */
   if (ef->references > 0)
      goto on_error;  /* flush any writes */

   err = eet_flush2(ef);

   eet_identity_unref(ef->key);
   ef->key = NULL;

   /* if not urgent to delete it - dont free it - leave it in cache */
   if ((!ef->delete_me_now) && (ef->mode == EET_FILE_MODE_READ))
      goto on_error;

   /* remove from cache */
   if (ef->mode == EET_FILE_MODE_READ)
      eet_cache_del(ef, &eet_readers, &eet_readers_num, &eet_readers_alloc);
   else if ((ef->mode == EET_FILE_MODE_WRITE) ||
            (
               ef
               ->
               mode == EET_FILE_MODE_READ_WRITE))
      eet_cache_del(ef, &eet_writers, &eet_writers_num, &eet_writers_alloc);

   /* we can unlock the cache now */
   if (!locked)
      UNLOCK_CACHE;

   DESTROY_FILE(ef);

   /* free up data */
   if (ef->header)
     {
        if (ef->header->directory)
          {
             if (ef->header->directory->nodes)
               {
                  int i, num;

                  num = (1 << ef->header->directory->size);
                  for (i = 0; i < num; i++)
                    {
                       Eet_File_Node *efn;

                       while ((efn = ef->header->directory->nodes[i]))
                         {
                            if (efn->data)
                               free(efn->data);

                            ef->header->directory->nodes[i] = efn->next;

                            if (efn->free_name)
                               free(efn->name);

                            free(efn);
                         }
                    }
                  free(ef->header->directory->nodes);
               }

             free(ef->header->directory);
          }

        free(ef->header);
     }

   eet_dictionary_free(ef->ed);

   if (ef->sha1)
      free(ef->sha1);

   if (ef->data)
      munmap((void *)ef->data, ef->data_size);

   if (ef->readfp)
      fclose(ef->readfp);

   /* zero out ram for struct - caution tactic against stale memory use */
   memset(ef, 0, sizeof(Eet_File));

   /* free it */
   free(ef);
   return err;

on_error:
   if (!locked)
      UNLOCK_CACHE;

   return EET_ERROR_NONE;
} /* eet_internal_close */

EAPI Eet_File *
eet_memopen_read(const void *data,
                 size_t      size)
{
   Eet_File *ef;

   if (!data || size == 0)
      return NULL;

   ef = malloc (sizeof (Eet_File));
   if (!ef)
      return NULL;

   INIT_FILE(ef);
   ef->ed = NULL;
   ef->path = NULL;
   ef->key = NULL;
   ef->magic = EET_MAGIC_FILE;
   ef->references = 1;
   ef->mode = EET_FILE_MODE_READ;
   ef->header = NULL;
   ef->mtime = 0;
   ef->delete_me_now = 1;
   ef->readfp = NULL;
   ef->data = data;
   ef->data_size = size;
   ef->sha1 = NULL;
   ef->sha1_length = 0;

   /* eet_internal_read expects the cache lock to be held when it is called */
   LOCK_CACHE;
   ef = eet_internal_read(ef);
   UNLOCK_CACHE;
   return ef;
} /* eet_memopen_read */

EAPI Eet_File *
eet_open(const char   *file,
         Eet_File_Mode mode)
{
   FILE *fp;
   Eet_File *ef;
   int file_len;
   struct stat file_stat;

   if (!file)
      return NULL;

   /* find the current file handle in cache*/
   ef = NULL;
   LOCK_CACHE;
   if (mode == EET_FILE_MODE_READ)
     {
        ef = eet_cache_find((char *)file, eet_writers, eet_writers_num);
        if (ef)
          {
             eet_sync(ef);
             ef->references++;
             ef->delete_me_now = 1;
             eet_internal_close(ef, EINA_TRUE);
          }

        ef = eet_cache_find((char *)file, eet_readers, eet_readers_num);
     }
   else if ((mode == EET_FILE_MODE_WRITE) ||
            (mode == EET_FILE_MODE_READ_WRITE))
     {
        ef = eet_cache_find((char *)file, eet_readers, eet_readers_num);
        if (ef)
          {
             ef->delete_me_now = 1;
             ef->references++;
             eet_internal_close(ef, EINA_TRUE);
          }

        ef = eet_cache_find((char *)file, eet_writers, eet_writers_num);
     }

   /* try open the file based on mode */
   if ((mode == EET_FILE_MODE_READ) || (mode == EET_FILE_MODE_READ_WRITE))
     {
        /* Prevent garbage in futur comparison. */
        file_stat.st_mtime = 0;

        fp = fopen(file, "rb");
        if (!fp)
           goto open_error;

        if (fstat(fileno(fp), &file_stat))
          {
             fclose(fp);
             fp = NULL;

             memset(&file_stat, 0, sizeof(file_stat));

             goto open_error;
          }

        if (file_stat.st_size < ((int)sizeof(int) * 3))
          {
             fclose(fp);
             fp = NULL;

             memset(&file_stat, 0, sizeof(file_stat));

             goto open_error;
          }

open_error:
        if (!fp && mode == EET_FILE_MODE_READ)
           goto on_error;
     }
   else
     {
        if (mode != EET_FILE_MODE_WRITE)
           return NULL;

        memset(&file_stat, 0, sizeof(file_stat));

        fp = NULL;
     }

   /* We found one */
   if (ef &&
       ((file_stat.st_mtime != ef->mtime) ||
        (file_stat.st_size != ef->data_size)))
     {
        ef->delete_me_now = 1;
        ef->references++;
        eet_internal_close(ef, EINA_TRUE);
        ef = NULL;
     }

   if (ef)
     {
        /* reference it up and return it */
        if (fp)
           fclose(fp);

        ef->references++;
        UNLOCK_CACHE;
        return ef;
     }

   file_len = strlen(file) + 1;

   /* Allocate struct for eet file and have it zero'd out */
   ef = malloc(sizeof(Eet_File) + file_len);
   if (!ef)
      goto on_error;

   /* fill some of the members */
   INIT_FILE(ef);
   ef->key = NULL;
   ef->readfp = fp;
   ef->path = ((char *)ef) + sizeof(Eet_File);
   memcpy(ef->path, file, file_len);
   ef->magic = EET_MAGIC_FILE;
   ef->references = 1;
   ef->mode = mode;
   ef->header = NULL;
   ef->mtime = file_stat.st_mtime;
   ef->writes_pending = 0;
   ef->delete_me_now = 0;
   ef->data = NULL;
   ef->data_size = 0;
   ef->sha1 = NULL;
   ef->sha1_length = 0;

   ef->ed = (mode == EET_FILE_MODE_WRITE)
      || (!ef->readfp && mode == EET_FILE_MODE_READ_WRITE) ?
      eet_dictionary_add() : NULL;

   if (!ef->readfp &&
       (mode == EET_FILE_MODE_READ_WRITE || mode == EET_FILE_MODE_WRITE))
      goto empty_file;

   /* if we can't open - bail out */
   if (eet_test_close(!ef->readfp, ef))
      goto on_error;

   fcntl(fileno(ef->readfp), F_SETFD, FD_CLOEXEC);
   /* if we opened for read or read-write */
   if ((mode == EET_FILE_MODE_READ) || (mode == EET_FILE_MODE_READ_WRITE))
     {
        ef->data_size = file_stat.st_size;
        ef->data = mmap(NULL, ef->data_size, PROT_READ,
                        MAP_SHARED, fileno(ef->readfp), 0);
        if (eet_test_close((ef->data == MAP_FAILED), ef))
           goto on_error;

        ef = eet_internal_read(ef);
        if (!ef)
           goto on_error;
     }

empty_file:
   /* add to cache */
   if (ef->references == 1)
     {
        if (ef->mode == EET_FILE_MODE_READ)
           eet_cache_add(ef, &eet_readers, &eet_readers_num, &eet_readers_alloc);
        else
        if ((ef->mode == EET_FILE_MODE_WRITE) ||
            (ef->mode == EET_FILE_MODE_READ_WRITE))
           eet_cache_add(ef, &eet_writers, &eet_writers_num, &eet_writers_alloc);
     }

   UNLOCK_CACHE;
   return ef;

on_error:
   UNLOCK_CACHE;
   return NULL;
} /* eet_open */

EAPI Eet_File_Mode
eet_mode_get(Eet_File *ef)
{
   /* check to see its' an eet file pointer */
   if ((!ef) || (ef->magic != EET_MAGIC_FILE))
      return EET_FILE_MODE_INVALID;
   else
      return ef->mode;
} /* eet_mode_get */

EAPI const void *
eet_identity_x509(Eet_File *ef,
                  int      *der_length)
{
   if (!ef->x509_der)
      return NULL;

   if (der_length)
      *der_length = ef->x509_length;

   return ef->x509_der;
} /* eet_identity_x509 */

EAPI const void *
eet_identity_signature(Eet_File *ef,
                       int      *signature_length)
{
   if (!ef->signature)
      return NULL;

   if (signature_length)
      *signature_length = ef->signature_length;

   return ef->signature;
} /* eet_identity_signature */

EAPI const void *
eet_identity_sha1(Eet_File *ef,
                  int      *sha1_length)
{
   if (!ef->sha1)
      ef->sha1 = eet_identity_compute_sha1(ef->data,
                                           ef->data_size,
                                           &ef->sha1_length);

   if (sha1_length)
      *sha1_length = ef->sha1_length;

   return ef->sha1;
} /* eet_identity_sha1 */

EAPI Eet_Error
eet_identity_set(Eet_File *ef,
                 Eet_Key  *key)
{
   Eet_Key *tmp = ef->key;

   if (!ef)
      return EET_ERROR_BAD_OBJECT;

   ef->key = key;
   eet_identity_ref(ef->key);
   eet_identity_unref(tmp);

   /* flags that writes are pending */
   ef->writes_pending = 1;

   return EET_ERROR_NONE;
} /* eet_identity_set */

EAPI Eet_Error
eet_close(Eet_File *ef)
{
   return eet_internal_close(ef, EINA_FALSE);
} /* eet_close */

EAPI void *
eet_read_cipher(Eet_File   *ef,
                const char *name,
                int        *size_ret,
                const char *cipher_key)
{
   Eet_File_Node *efn;
   char *data = NULL;
   int size = 0;

   if (size_ret)
      *size_ret = 0;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return NULL;

   if (!name)
      return NULL;

   if ((ef->mode != EET_FILE_MODE_READ) &&
       (ef->mode != EET_FILE_MODE_READ_WRITE))
      return NULL;

   /* no header, return NULL */
   if (eet_check_header(ef))
      return NULL;

   LOCK_FILE(ef);

   /* hunt hash bucket */
   efn = find_node_by_name(ef, name);
   if (!efn)
      goto on_error;

   /* get size (uncompressed, if compressed at all) */
   size = efn->data_size;

   /* allocate data */
   data = malloc(size);
   if (!data)
      goto on_error;

   /* uncompressed data */
   if (efn->compression == 0)
     {
        void *data_deciphered = NULL;
        unsigned int data_deciphered_sz = 0;
        /* if we already have the data in ram... copy that */

        if (efn->ciphered && efn->size > size)
          {
             size = efn->size;
             data = realloc(data, efn->size);
          }

        if (efn->data)
           memcpy(data, efn->data, size);
        else
          if (!read_data_from_disk(ef, efn, data, size))
            goto on_error;

        if (efn->ciphered && cipher_key)
          {
             if (eet_decipher(data, efn->size, cipher_key, strlen(cipher_key),
                              &data_deciphered, &data_deciphered_sz))
               {
                  if (data_deciphered)
                     free(data_deciphered);

                  goto on_error;
               }

             free(data);
             data = data_deciphered;
             size = data_deciphered_sz;
          }
     }
   /* compressed data */
   else
     {
        void *tmp_data = NULL;
        void *data_deciphered = NULL;
        unsigned int data_deciphered_sz = 0;
        int free_tmp = 0;
        int compr_size = efn->size;
        uLongf dlen;

        /* if we already have the data in ram... copy that */
        if (efn->data)
           tmp_data = efn->data;
        else
          {
             tmp_data = malloc(compr_size);
             if (!tmp_data)
                goto on_error;

             free_tmp = 1;

             if (!read_data_from_disk(ef, efn, tmp_data, compr_size))
               {
                  free(tmp_data);
                  goto on_error;
               }
          }

        if (efn->ciphered && cipher_key)
          {
             if (eet_decipher(tmp_data, compr_size, cipher_key,
                              strlen(cipher_key), &data_deciphered,
                              &data_deciphered_sz))
               {
                  if (free_tmp)
                     free(tmp_data);

                  if (data_deciphered)
                     free(data_deciphered);

                  goto on_error;
               }

             if (free_tmp)
                free(tmp_data);
             free_tmp = 1;
             tmp_data = data_deciphered;
             compr_size = data_deciphered_sz;
          }

        /* decompress it */
        dlen = size;
        if (uncompress((Bytef *)data, &dlen,
                       tmp_data, (uLongf)compr_size))
          {
             if (free_tmp)
                free(tmp_data);
             goto on_error;
          }

        if (free_tmp)
           free(tmp_data);
     }

   UNLOCK_FILE(ef);

   /* handle alias */
   if (efn->alias)
     {
        void *tmp;

        if (data[size - 1] != '\0')
           goto on_error;

        tmp = eet_read_cipher(ef, data, size_ret, cipher_key);

        free(data);

        data = tmp;
     }
   else
   /* fill in return values */
   if (size_ret)
      *size_ret = size;

   return data;

on_error:
   UNLOCK_FILE(ef);
   free(data);
   return NULL;
} /* eet_read_cipher */

EAPI void *
eet_read(Eet_File   *ef,
         const char *name,
         int        *size_ret)
{
   return eet_read_cipher(ef, name, size_ret, NULL);
} /* eet_read */

EAPI const void *
eet_read_direct(Eet_File   *ef,
                const char *name,
                int        *size_ret)
{
   Eet_File_Node *efn;
   const char *data = NULL;
   int size = 0;

   if (size_ret)
      *size_ret = 0;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return NULL;

   if (!name)
      return NULL;

   if ((ef->mode != EET_FILE_MODE_READ) &&
       (ef->mode != EET_FILE_MODE_READ_WRITE))
      return NULL;

   /* no header, return NULL */
   if (eet_check_header(ef))
      return NULL;

   LOCK_FILE(ef);

   /* hunt hash bucket */
   efn = find_node_by_name(ef, name);
   if (!efn)
      goto on_error;

   if (efn->offset < 0 && !efn->data)
      goto on_error;

   /* get size (uncompressed, if compressed at all) */
   size = efn->data_size;

   if (efn->alias)
     {
        data = efn->data ? efn->data : ef->data + efn->offset;

        /* handle alias case */
        if (efn->compression)
          {
             char *tmp;
             int compr_size = efn->size;
             uLongf dlen;

             tmp = alloca(sizeof (compr_size));
             dlen = size;

             if (uncompress((Bytef *)tmp, &dlen, (Bytef *)data,
                            (uLongf)compr_size))
                goto on_error;

             if (tmp[compr_size - 1] != '\0')
                goto on_error;

	     UNLOCK_FILE(ef);

             return eet_read_direct(ef, tmp, size_ret);
          }

        if (!data)
           goto on_error;

        if (data[size - 1] != '\0')
           goto on_error;

	UNLOCK_FILE(ef);

        return eet_read_direct(ef, data, size_ret);
     }
   else
   /* uncompressed data */
   if (efn->compression == 0
       && efn->ciphered == 0)
      data = efn->data ? efn->data : ef->data + efn->offset;  /* compressed data */
   else
      data = NULL;

   /* fill in return values */
   if (size_ret)
      *size_ret = size;

   UNLOCK_FILE(ef);

   return data;

on_error:
   UNLOCK_FILE(ef);
   return NULL;
} /* eet_read_direct */

EAPI Eina_Bool
eet_alias(Eet_File   *ef,
          const char *name,
          const char *destination,
          int         comp)
{
   Eet_File_Node *efn;
   void *data2;
   Eina_Bool exists_already = EINA_FALSE;
   int data_size;
   int hash;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return EINA_FALSE;

   if ((!name) || (!destination))
      return EINA_FALSE;

   if ((ef->mode != EET_FILE_MODE_WRITE) &&
       (ef->mode != EET_FILE_MODE_READ_WRITE))
      return EINA_FALSE;

   LOCK_FILE(ef);

   if (!ef->header)
     {
        /* allocate header */
        ef->header = calloc(1, sizeof(Eet_File_Header));
        if (!ef->header)
           goto on_error;

        ef->header->magic = EET_MAGIC_FILE_HEADER;
        /* allocate directory block in ram */
        ef->header->directory = calloc(1, sizeof(Eet_File_Directory));
        if (!ef->header->directory)
          {
             free(ef->header);
             ef->header = NULL;
             goto on_error;
          }

        /* 8 bit hash table (256 buckets) */
        ef->header->directory->size = 8;
        /* allocate base hash table */
        ef->header->directory->nodes =
           calloc(1, sizeof(Eet_File_Node *) *
                  (1 << ef->header->directory->size));
        if (!ef->header->directory->nodes)
          {
             free(ef->header->directory);
             ef->header = NULL;
             goto on_error;
          }
     }

   /* figure hash bucket */
   hash = _eet_hash_gen(name, ef->header->directory->size);

   data_size = comp ?
      12 + (((strlen(destination) + 1) * 101) / 100)
      : strlen(destination) + 1;

   data2 = malloc(data_size);
   if (!data2)
      goto on_error;

   /* if we want to compress */
   if (comp)
     {
        uLongf buflen;

        /* compress the data with max compression */
        buflen = (uLongf)data_size;
        if (compress2((Bytef *)data2, &buflen, (Bytef *)destination,
                      (uLong)strlen(destination) + 1,
                      Z_BEST_COMPRESSION) != Z_OK)
          {
             free(data2);
             goto on_error;
          }

        /* record compressed chunk size */
        data_size = (int)buflen;
        if (data_size < 0 || data_size >= (int)(strlen(destination) + 1))
          {
             comp = 0;
             data_size = strlen(destination) + 1;
          }
        else
          {
             void *data3;

             data3 = realloc(data2, data_size);
             if (data3)
                data2 = data3;
          }
     }

   if (!comp)
      memcpy(data2, destination, data_size);

   /* Does this node already exist? */
   for (efn = ef->header->directory->nodes[hash]; efn; efn = efn->next)
     {
        /* if it matches */
        if ((efn->name) && (eet_string_match(efn->name, name)))
          {
             free(efn->data);
             efn->alias = 1;
             efn->ciphered = 0;
             efn->compression = !!comp;
             efn->size = data_size;
             efn->data_size = strlen(destination) + 1;
             efn->data = data2;
             efn->offset = -1;
             exists_already = EINA_TRUE;
             break;
          }
     }
   if (!exists_already)
     {
        efn = malloc(sizeof(Eet_File_Node));
        if (!efn)
          {
             free(data2);
             goto on_error;
          }

        efn->name = strdup(name);
        efn->name_size = strlen(efn->name) + 1;
        efn->free_name = 1;

        efn->next = ef->header->directory->nodes[hash];
        ef->header->directory->nodes[hash] = efn;
        efn->offset = -1;
        efn->alias = 1;
        efn->ciphered = 0;
        efn->compression = !!comp;
        efn->size = data_size;
        efn->data_size = strlen(destination) + 1;
        efn->data = data2;
     }

   /* flags that writes are pending */
   ef->writes_pending = 1;

   UNLOCK_FILE(ef);
   return EINA_TRUE;

on_error:
   UNLOCK_FILE(ef);
   return EINA_FALSE;
} /* eet_alias */

EAPI int
eet_write_cipher(Eet_File   *ef,
                 const char *name,
                 const void *data,
                 int         size,
                 int         comp,
                 const char *cipher_key)
{
   Eet_File_Node *efn;
   void *data2 = NULL;
   int exists_already = 0;
   int data_size;
   int hash;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return 0;

   if ((!name) || (!data) || (size <= 0))
      return 0;

   if ((ef->mode != EET_FILE_MODE_WRITE) &&
       (ef->mode != EET_FILE_MODE_READ_WRITE))
      return 0;

   LOCK_FILE(ef);

   if (!ef->header)
     {
        /* allocate header */
        ef->header = calloc(1, sizeof(Eet_File_Header));
        if (!ef->header)
           goto on_error;

        ef->header->magic = EET_MAGIC_FILE_HEADER;
        /* allocate directory block in ram */
        ef->header->directory = calloc(1, sizeof(Eet_File_Directory));
        if (!ef->header->directory)
          {
             free(ef->header);
             ef->header = NULL;
             goto on_error;
          }

        /* 8 bit hash table (256 buckets) */
        ef->header->directory->size = 8;
        /* allocate base hash table */
        ef->header->directory->nodes =
           calloc(1, sizeof(Eet_File_Node *) *
                  (1 << ef->header->directory->size));
        if (!ef->header->directory->nodes)
          {
             free(ef->header->directory);
             ef->header = NULL;
             goto on_error;
          }
     }

   /* figure hash bucket */
   hash = _eet_hash_gen(name, ef->header->directory->size);

   data_size = comp ? 12 + ((size * 101) / 100) : size;

   if (comp || !cipher_key)
     {
        data2 = malloc(data_size);
        if (!data2)
           goto on_error;
     }

   /* if we want to compress */
   if (comp)
     {
        uLongf buflen;

        /* compress the data with max compression */
        buflen = (uLongf)data_size;
        if (compress2((Bytef *)data2, &buflen, (Bytef *)data,
                      (uLong)size, Z_BEST_COMPRESSION) != Z_OK)
          {
             free(data2);
             goto on_error;
          }

        /* record compressed chunk size */
        data_size = (int)buflen;
        if (data_size < 0 || data_size >= size)
          {
             comp = 0;
             data_size = size;
          }
        else
          {
             void *data3;

             data3 = realloc(data2, data_size);
             if (data3)
                data2 = data3;
          }
     }

   if (cipher_key)
     {
        void *data_ciphered = NULL;
        unsigned int data_ciphered_sz = 0;
        const void *tmp;

        tmp = comp ? data2 : data;
        if (!eet_cipher(tmp, data_size, cipher_key, strlen(cipher_key),
                        &data_ciphered, &data_ciphered_sz))
          {
             if (data2)
                free(data2);

             data2 = data_ciphered;
             data_size = data_ciphered_sz;
          }
        else
          {
             if (data_ciphered)
                free(data_ciphered);

             cipher_key = NULL;
          }
     }
   else
     if (!comp)
       memcpy(data2, data, size);

   /* Does this node already exist? */
   for (efn = ef->header->directory->nodes[hash]; efn; efn = efn->next)
     {
        /* if it matches */
        if ((efn->name) && (eet_string_match(efn->name, name)))
          {
             free(efn->data);
             efn->alias = 0;
             efn->ciphered = cipher_key ? 1 : 0;
             efn->compression = !!comp;
             efn->size = data_size;
             efn->data_size = size;
             efn->data = data2;
             efn->offset = -1;
             exists_already = 1;
             break;
          }
     }
   if (!exists_already)
     {
        efn = malloc(sizeof(Eet_File_Node));
        if (!efn)
          {
             free(data2);
             goto on_error;
          }

        efn->name = strdup(name);
        efn->name_size = strlen(efn->name) + 1;
        efn->free_name = 1;

        efn->next = ef->header->directory->nodes[hash];
        ef->header->directory->nodes[hash] = efn;
        efn->offset = -1;
        efn->alias = 0;
        efn->ciphered = cipher_key ? 1 : 0;
        efn->compression = !!comp;
        efn->size = data_size;
        efn->data_size = size;
        efn->data = data2;
     }

   /* flags that writes are pending */
   ef->writes_pending = 1;
   UNLOCK_FILE(ef);
   return data_size;

on_error:
   UNLOCK_FILE(ef);
   return 0;
} /* eet_write_cipher */

EAPI int
eet_write(Eet_File   *ef,
          const char *name,
          const void *data,
          int         size,
          int         comp)
{
   return eet_write_cipher(ef, name, data, size, comp, NULL);
} /* eet_write */

EAPI int
eet_delete(Eet_File   *ef,
           const char *name)
{
   Eet_File_Node *efn;
   Eet_File_Node *pefn;
   int hash;
   int exists_already = 0;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef))
      return 0;

   if (!name)
      return 0;

   /* deleting keys is only possible in RW or WRITE mode */
   if (ef->mode == EET_FILE_MODE_READ)
      return 0;

   if (eet_check_header(ef))
      return 0;

   LOCK_FILE(ef);

   /* figure hash bucket */
   hash = _eet_hash_gen(name, ef->header->directory->size);

   /* Does this node already exist? */
   for (pefn = NULL, efn = ef->header->directory->nodes[hash];
        efn;
        pefn = efn, efn = efn->next)
     {
        /* if it matches */
        if (eet_string_match(efn->name, name))
          {
             if (efn->data)
                free(efn->data);

             if (!pefn)
                ef->header->directory->nodes[hash] = efn->next;
             else
                pefn->next = efn->next;

             if (efn->free_name)
                free(efn->name);

             free(efn);
             exists_already = 1;
             break;
          }
     }
   /* flags that writes are pending */
   if (exists_already)
      ef->writes_pending = 1;

   UNLOCK_FILE(ef);

   /* update access time */
   return exists_already;
} /* eet_delete */

EAPI Eet_Dictionary *
eet_dictionary_get(Eet_File *ef)
{
   if (eet_check_pointer(ef))
      return NULL;

   return ef->ed;
} /* eet_dictionary_get */

EAPI char **
eet_list(Eet_File   *ef,
         const char *glob,
         int        *count_ret)
{
   Eet_File_Node *efn;
   char **list_ret = NULL;
   int list_count = 0;
   int list_count_alloc = 0;
   int i, num;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef) || eet_check_header(ef) ||
       (!glob) ||
       ((ef->mode != EET_FILE_MODE_READ) &&
        (ef->mode != EET_FILE_MODE_READ_WRITE)))
     {
        if (count_ret)
           *count_ret = 0;

        return NULL;
     }

   if (!strcmp(glob, "*"))
      glob = NULL;

   LOCK_FILE(ef);

   /* loop through all entries */
   num = (1 << ef->header->directory->size);
   for (i = 0; i < num; i++)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
          {
             /* if the entry matches the input glob
              * check for * explicitly, because on some systems, * isn't well
              * supported
              */
             if ((!glob) || !fnmatch(glob, efn->name, 0))
               {
                  /* add it to our list */
                  list_count++;

                  /* only realloc in 32 entry chunks */
                  if (list_count > list_count_alloc)
                    {
                       char **new_list = NULL;

                       list_count_alloc += 64;
                       new_list =
                          realloc(list_ret, list_count_alloc * (sizeof(char *)));
                       if (!new_list)
                         {
                            free(list_ret);

                            goto on_error;
                         }

                       list_ret = new_list;
                    }

                  /* put pointer of name string in */
                  list_ret[list_count - 1] = efn->name;
               }
          }
     }

   UNLOCK_FILE(ef);

   /* return count and list */
   if (count_ret)
      *count_ret = list_count;

   return list_ret;

on_error:
   UNLOCK_FILE(ef);

   if (count_ret)
      *count_ret = 0;

   return NULL;
} /* eet_list */

EAPI int
eet_num_entries(Eet_File *ef)
{
   int i, num, ret = 0;
   Eet_File_Node *efn;

   /* check to see its' an eet file pointer */
   if (eet_check_pointer(ef) || eet_check_header(ef) ||
       ((ef->mode != EET_FILE_MODE_READ) &&
        (ef->mode != EET_FILE_MODE_READ_WRITE)))
      return -1;

   LOCK_FILE(ef);

   /* loop through all entries */
   num = (1 << ef->header->directory->size);
   for (i = 0; i < num; i++)
     {
        for (efn = ef->header->directory->nodes[i]; efn; efn = efn->next)
           ret++;
     }

   UNLOCK_FILE(ef);

   return ret;
} /* eet_num_entries */

static Eet_File_Node *
find_node_by_name(Eet_File   *ef,
                  const char *name)
{
   Eet_File_Node *efn;
   int hash;

   /* get hash bucket this should be in */
   hash = _eet_hash_gen(name, ef->header->directory->size);

   for (efn = ef->header->directory->nodes[hash]; efn; efn = efn->next)
     {
        if (eet_string_match(efn->name, name))
           return efn;
     }

   return NULL;
} /* find_node_by_name */

static int
read_data_from_disk(Eet_File      *ef,
                    Eet_File_Node *efn,
                    void          *buf,
                    int            len)
{
   if (efn->offset < 0)
      return 0;

   if (ef->data)
     {
        if ((efn->offset + len) > ef->data_size)
           return 0;

        memcpy(buf, ef->data + efn->offset, len);
     }
   else
     {
        if (!ef->readfp)
           return 0;

        /* seek to data location */
        if (fseek(ef->readfp, efn->offset, SEEK_SET) < 0)
           return 0;

        /* read it */
        len = fread(buf, len, 1, ef->readfp);
     }

   return len;
} /* read_data_from_disk */

