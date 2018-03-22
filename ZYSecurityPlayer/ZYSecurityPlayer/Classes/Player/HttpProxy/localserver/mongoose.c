#define MG_DISABLE_FILESYSTEM
#define MG_DISABLE_POPEN
#define MG_DISABLE_CGI
#define MG_DISABLE_DIRECTORY_LISTING
#define MG_DISABLE_SOCKETPAIR
#define MG_DISABLE_PFS

#include "mongoose.h"

//SELECT 实现要求 fd必须小于FD_SETSIZE,这里加上保护, add by hualiangyan, 2016.10.17
static int MYSAFE_SOCKET_FOR_SELECT(int socket_family, int socket_type, int protocol)
{
    int socket_fd = socket(socket_family, socket_type, protocol);
    
    if(socket_fd  != INVALID_SOCKET)
    {
        if(socket_fd >= FD_SETSIZE)
        {
            TVHttpProxyLOG(LOG_ERROR, "[TVDownloadProxy_LocalProxy]local server create socket %d over FD_SETSIZE(%d) cause select error", socket_fd, FD_SETSIZE);
            
            close(socket_fd);
            
            socket_fd = INVALID_SOCKET;
        }
    }
    
    return socket_fd;
}

#ifdef NS_MODULE_LINES
#line 1 "src/internal.h"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MG_INTERNAL_HEADER_INCLUDED
#define MG_INTERNAL_HEADER_INCLUDED

#ifndef MG_MALLOC
#define MG_MALLOC malloc
#endif

#ifndef MG_CALLOC
#define MG_CALLOC calloc
#endif

#ifndef MG_REALLOC
#define MG_REALLOC realloc
#endif

#ifndef MG_FREE
#define MG_FREE free
#endif

#ifndef MBUF_REALLOC
#define MBUF_REALLOC MG_REALLOC
#endif

#ifndef MBUF_FREE
#define MBUF_FREE MG_FREE
#endif

#define MG_SET_PTRPTR(_ptr, _v) \
  do {                          \
    if (_ptr) *(_ptr) = _v;     \
  } while (0)

#ifndef MG_INTERNAL
#define MG_INTERNAL static
#endif

#if !defined(MG_MGR_EV_MGR) && defined(__linux__)
#define MG_MGR_EV_MGR 1 /* epoll() */
#endif
#if !defined(MG_MGR_EV_MGR)
#define MG_MGR_EV_MGR 0 /* select() */
#endif

#ifdef PICOTCP
#define NO_LIBC
#define MG_DISABLE_FILESYSTEM
#define MG_DISABLE_POPEN
#define MG_DISABLE_CGI
#define MG_DISABLE_DIRECTORY_LISTING
#define MG_DISABLE_SOCKETPAIR
#define MG_DISABLE_PFS
#endif

/* Amalgamated: #include "../mongoose.h" */

/* internals that need to be accessible in unit tests */
MG_INTERNAL struct mg_connection *mg_finish_connect(struct mg_connection *nc,
                                                    int proto,
                                                    union socket_address *sa,
                                                    struct mg_add_sock_opts);

MG_INTERNAL int mg_parse_address(const char *str, union socket_address *sa,
                                 int *proto, char *host, size_t host_len);
MG_INTERNAL void mg_call(struct mg_connection *, int ev, void *ev_data);
MG_INTERNAL void mg_forward(struct mg_connection *, struct mg_connection *);
MG_INTERNAL void mg_add_conn(struct mg_mgr *mgr, struct mg_connection *c);
MG_INTERNAL void mg_remove_conn(struct mg_connection *c);

#ifndef MG_DISABLE_FILESYSTEM
MG_INTERNAL int find_index_file(char *, size_t, const char *, cs_stat_t *);
#endif

#ifdef _WIN32
void to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len);
#endif

/*
 * Reassemble the content of the buffer (buf, blen) which should be
 * in the HTTP chunked encoding, by collapsing data chunks to the
 * beginning of the buffer.
 *
 * If chunks get reassembled, modify hm->body to point to the reassembled
 * body and fire MG_EV_HTTP_CHUNK event. If handler sets MG_F_DELETE_CHUNK
 * in nc->flags, delete reassembled body from the mbuf.
 *
 * Return reassembled body size.
 */
MG_INTERNAL size_t mg_handle_chunked(struct mg_connection *nc,
                                     struct http_message *hm, char *buf,
                                     size_t blen);

/* Forward declarations for testing. */
extern void *(*test_malloc)(size_t);
extern void *(*test_calloc)(size_t, size_t);

#endif /* MG_INTERNAL_HEADER_INCLUDED */
#ifdef NS_MODULE_LINES
#line 1 "src/../../common/mbuf.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

#include <assert.h>
#include <string.h>
/* Amalgamated: #include "mbuf.h" */

#ifndef MBUF_REALLOC
#define MBUF_REALLOC realloc
#endif

#ifndef MBUF_FREE
#define MBUF_FREE free
#endif

#ifdef MG_ENABLE_DEBUG
static const char * ipv4ToStr(const struct in_addr *in, char *buf, socklen_t buflen)
{
    struct in_addr ia = *in;
    ia.s_addr = htonl(ia.s_addr);
    return inet_ntop(AF_INET, (void *)&ia, buf, buflen);
}
#endif

void mbuf_init(struct mbuf *mbuf, size_t initial_size) {
  mbuf->len = mbuf->size = 0;
  mbuf->buf = NULL;
  mbuf_resize(mbuf, initial_size);
}

void mbuf_free(struct mbuf *mbuf) {
  if (mbuf->buf != NULL) {
    MBUF_FREE(mbuf->buf);
    mbuf_init(mbuf, 0);
  }
}

void mbuf_resize(struct mbuf *a, size_t new_size) {
  char *p;
  if ((new_size > a->size || (new_size < a->size && new_size >= a->len)) &&
      (p = (char *) MBUF_REALLOC(a->buf, new_size)) != NULL) {
    a->size = new_size;
    a->buf = p;
  }
}

void mbuf_trim(struct mbuf *mbuf) {
  mbuf_resize(mbuf, mbuf->len);
}

size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t len) {
  char *p = NULL;

  assert(a != NULL);
  assert(a->len <= a->size);
  assert(off <= a->len);

  /* check overflow */
  if (~(size_t) 0 - (size_t) a->buf < len) return 0;

  if (a->len + len <= a->size) {
    memmove(a->buf + off + len, a->buf + off, a->len - off);
    if (buf != NULL) {
      memcpy(a->buf + off, buf, len);
    }
    a->len += len;
  } else if ((p = (char *) MBUF_REALLOC(
                  a->buf, (a->len + len) * MBUF_SIZE_MULTIPLIER)) != NULL) {
    a->buf = p;
    memmove(a->buf + off + len, a->buf + off, a->len - off);
    if (buf != NULL) {
      memcpy(a->buf + off, buf, len);
    }
    a->len += len;
    a->size = a->len * MBUF_SIZE_MULTIPLIER;
  } else {
    len = 0;
  }

  return len;
}

size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) {
  return mbuf_insert(a, a->len, buf, len);
}

void mbuf_remove(struct mbuf *mb, size_t n) {
  if (n > 0 && n <= mb->len) {
    memmove(mb->buf, mb->buf + n, mb->len - n);
    mb->len -= n;
  }
}

#endif /* EXCLUDE_COMMON */
#ifdef NS_MODULE_LINES
#line 1 "src/../../common/sha1.c"
/**/
#endif
/* Copyright(c) By Steve Reid <steve@edmweb.com> */
/* 100% Public Domain */

#ifdef NS_MODULE_LINES
#line 1 "src/../../common/md5.c"
/**/
#endif
/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#ifdef NS_MODULE_LINES
#line 1 "src/../../common/base64.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#if 0
#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "base64.h" */
#include <string.h>

/* ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ */

#define NUM_UPPERCASES ('Z' - 'A' + 1)
#define NUM_LETTERS (NUM_UPPERCASES * 2)
#define NUM_DIGITS ('9' - '0' + 1)

/*
 * Emit a base64 code char.
 *
 * Doesn't use memory, thus it's safe to use to safely dump memory in crashdumps
 */
static void cs_base64_emit_code(struct cs_base64_ctx *ctx, int v) {
  if (v < NUM_UPPERCASES) {
    ctx->b64_putc(v + 'A', ctx->user_data);
  } else if (v < (NUM_LETTERS)) {
    ctx->b64_putc(v - NUM_UPPERCASES + 'a', ctx->user_data);
  } else if (v < (NUM_LETTERS + NUM_DIGITS)) {
    ctx->b64_putc(v - NUM_LETTERS + '0', ctx->user_data);
  } else {
    ctx->b64_putc(v - NUM_LETTERS - NUM_DIGITS == 0 ? '+' : '/',
                  ctx->user_data);
  }
}

static void cs_base64_emit_chunk(struct cs_base64_ctx *ctx) {
  int a, b, c;

  a = ctx->chunk[0];
  b = ctx->chunk[1];
  c = ctx->chunk[2];

  cs_base64_emit_code(ctx, a >> 2);
  cs_base64_emit_code(ctx, ((a & 3) << 4) | (b >> 4));
  if (ctx->chunk_size > 1) {
    cs_base64_emit_code(ctx, (b & 15) << 2 | (c >> 6));
  }
  if (ctx->chunk_size > 2) {
    cs_base64_emit_code(ctx, c & 63);
  }
}

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t b64_putc,
                    void *user_data) {
  ctx->chunk_size = 0;
  ctx->b64_putc = b64_putc;
  ctx->user_data = user_data;
}

void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len) {
  const unsigned char *src = (const unsigned char *) str;
  size_t i;
  for (i = 0; i < len; i++) {
    ctx->chunk[ctx->chunk_size++] = src[i];
    if (ctx->chunk_size == 3) {
      cs_base64_emit_chunk(ctx);
      ctx->chunk_size = 0;
    }
  }
}

void cs_base64_finish(struct cs_base64_ctx *ctx) {
  if (ctx->chunk_size > 0) {
    int i;
    memset(&ctx->chunk[ctx->chunk_size], 0, 3 - ctx->chunk_size);
    cs_base64_emit_chunk(ctx);
    for (i = 0; i < (3 - ctx->chunk_size); i++) {
      ctx->b64_putc('=', ctx->user_data);
    }
  }
}

#define BASE64_ENCODE_BODY                                                \
  static const char *b64 =                                                \
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; \
  int i, j, a, b, c;                                                      \
                                                                          \
  for (i = j = 0; i < src_len; i += 3) {                                  \
    a = src[i];                                                           \
    b = i + 1 >= src_len ? 0 : src[i + 1];                                \
    c = i + 2 >= src_len ? 0 : src[i + 2];                                \
                                                                          \
    BASE64_OUT(b64[a >> 2]);                                              \
    BASE64_OUT(b64[((a & 3) << 4) | (b >> 4)]);                           \
    if (i + 1 < src_len) {                                                \
      BASE64_OUT(b64[(b & 15) << 2 | (c >> 6)]);                          \
    }                                                                     \
    if (i + 2 < src_len) {                                                \
      BASE64_OUT(b64[c & 63]);                                            \
    }                                                                     \
  }                                                                       \
                                                                          \
  while (j % 4 != 0) {                                                    \
    BASE64_OUT('=');                                                      \
  }                                                                       \
  BASE64_FLUSH()

#define BASE64_OUT(ch) \
  do {                 \
    dst[j++] = (ch);   \
  } while (0)

#define BASE64_FLUSH() \
  do {                 \
    dst[j++] = '\0';   \
  } while (0)

void cs_base64_encode(const unsigned char *src, int src_len, char *dst) {
  BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH

#define BASE64_OUT(ch)      \
  do {                      \
    fprintf(f, "%c", (ch)); \
    j++;                    \
  } while (0)

#define BASE64_FLUSH()

void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len) {
  BASE64_ENCODE_BODY;
}

#undef BASE64_OUT
#undef BASE64_FLUSH

/* Convert one byte of encoded base64 input stream to 6-bit chunk */
static unsigned char from_b64(unsigned char ch) {
  /* Inverse lookup map */
  static const unsigned char tab[128] = {
      255, 255, 255, 255,
      255, 255, 255, 255, /*  0 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  8 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  16 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  24 */
      255, 255, 255, 255,
      255, 255, 255, 255, /*  32 */
      255, 255, 255, 62,
      255, 255, 255, 63, /*  40 */
      52,  53,  54,  55,
      56,  57,  58,  59, /*  48 */
      60,  61,  255, 255,
      255, 200, 255, 255, /*  56   '=' is 200, on index 61 */
      255, 0,   1,   2,
      3,   4,   5,   6, /*  64 */
      7,   8,   9,   10,
      11,  12,  13,  14, /*  72 */
      15,  16,  17,  18,
      19,  20,  21,  22, /*  80 */
      23,  24,  25,  255,
      255, 255, 255, 255, /*  88 */
      255, 26,  27,  28,
      29,  30,  31,  32, /*  96 */
      33,  34,  35,  36,
      37,  38,  39,  40, /*  104 */
      41,  42,  43,  44,
      45,  46,  47,  48, /*  112 */
      49,  50,  51,  255,
      255, 255, 255, 255, /*  120 */
  };
  return tab[ch & 127];
}

int cs_base64_decode(const unsigned char *s, int len, char *dst) {
  unsigned char a, b, c, d;
  int orig_len = len;
  while (len >= 4 && (a = from_b64(s[0])) != 255 &&
         (b = from_b64(s[1])) != 255 && (c = from_b64(s[2])) != 255 &&
         (d = from_b64(s[3])) != 255) {
    s += 4;
    len -= 4;
    if (a == 200 || b == 200) break; /* '=' can't be there */
    *dst++ = a << 2 | b >> 4;
    if (c == 200) break;
    *dst++ = b << 4 | c >> 2;
    if (d == 200) break;
    *dst++ = c << 6 | d;
  }
  *dst = 0;
  return orig_len - len;
}

#endif /* EXCLUDE_COMMON */
#endif
#ifdef NS_MODULE_LINES
#line 1 "src/../../common/str_util.c"
/**/
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "osdep.h" */
/* Amalgamated: #include "str_util.h" */

#if !(_XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L) &&    \
        !(__DARWIN_C_LEVEL >= 200809L) && !defined(RTOS_SDK) || \
    defined(_WIN32)
#ifndef __ANDROID__
int strnlen(const char *s, size_t maxlen) {
  size_t l = 0;
  for (; l < maxlen && s[l] != '\0'; l++) {
  }
  return (int)l;
}
#endif
#endif

#define C_SNPRINTF_APPEND_CHAR(ch)       \
  do {                                   \
    if (i < (int) buf_size) buf[i] = ch; \
    i++;                                 \
  } while (0)

#define C_SNPRINTF_FLAG_ZERO 1

#ifdef C_DISABLE_BUILTIN_SNPRINTF
int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  return vsnprintf(buf, buf_size, fmt, ap);
}
#else
static int c_itoa(char *buf, size_t buf_size, int64_t num, int base, int flags,
                  int field_width) {
  char tmp[40];
  int i = 0, k = 0, neg = 0;

  if (num < 0) {
    neg++;
    num = -num;
  }

  /* Print into temporary buffer - in reverse order */
  do {
    int rem = num % base;
    if (rem < 10) {
      tmp[k++] = '0' + rem;
    } else {
      tmp[k++] = 'a' + (rem - 10);
    }
    num /= base;
  } while (num > 0);

  /* Zero padding */
  if (flags && C_SNPRINTF_FLAG_ZERO) {
    while (k < field_width && k < (int) sizeof(tmp) - 1) {
      tmp[k++] = '0';
    }
  }

  /* And sign */
  if (neg) {
    tmp[k++] = '-';
  }

  /* Now output */
  while (--k >= 0) {
    C_SNPRINTF_APPEND_CHAR(tmp[k]);
  }

  return i;
}

int c_vsnprintf(char *buf, size_t buf_size, const char *fmt, va_list ap) {
  int ch, i = 0, len_mod, flags, precision, field_width;

  while ((ch = *fmt++) != '\0') {
    if (ch != '%') {
      C_SNPRINTF_APPEND_CHAR(ch);
    } else {
      /*
       * Conversion specification:
       *   zero or more flags (one of: # 0 - <space> + ')
       *   an optional minimum  field  width (digits)
       *   an  optional precision (. followed by digits, or *)
       *   an optional length modifier (one of: hh h l ll L q j z t)
       *   conversion specifier (one of: d i o u x X e E f F g G a A c s p n)
       */
      flags = field_width = precision = len_mod = 0;

      /* Flags. only zero-pad flag is supported. */
      if (*fmt == '0') {
        flags |= C_SNPRINTF_FLAG_ZERO;
      }

      /* Field width */
      while (*fmt >= '0' && *fmt <= '9') {
        field_width *= 10;
        field_width += *fmt++ - '0';
      }
      /* Dynamic field width */
      if (*fmt == '*') {
        field_width = va_arg(ap, int);
        fmt++;
      }

      /* Precision */
      if (*fmt == '.') {
        fmt++;
        if (*fmt == '*') {
          precision = va_arg(ap, int);
          fmt++;
        } else {
          while (*fmt >= '0' && *fmt <= '9') {
            precision *= 10;
            precision += *fmt++ - '0';
          }
        }
      }

      /* Length modifier */
      switch (*fmt) {
        case 'h':
        case 'l':
        case 'L':
        case 'I':
        case 'q':
        case 'j':
        case 'z':
        case 't':
          len_mod = *fmt++;
          if (*fmt == 'h') {
            len_mod = 'H';
            fmt++;
          }
          if (*fmt == 'l') {
            len_mod = 'q';
            fmt++;
          }
          break;
      }

      ch = *fmt++;
      if (ch == 's') {
        const char *s = va_arg(ap, const char *); /* Always fetch parameter */
        int j;
        int pad = field_width - (precision >= 0 ? strnlen(s, precision) : 0);
        for (j = 0; j < pad; j++) {
          C_SNPRINTF_APPEND_CHAR(' ');
        }

        /* Ignore negative and 0 precisions */
        for (j = 0; (precision <= 0 || j < precision) && s[j] != '\0'; j++) {
          C_SNPRINTF_APPEND_CHAR(s[j]);
        }
      } else if (ch == 'c') {
        ch = va_arg(ap, int); /* Always fetch parameter */
        C_SNPRINTF_APPEND_CHAR(ch);
      } else if (ch == 'd' && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, int), 10, flags,
                    field_width);
      } else if (ch == 'd' && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, long), 10, flags,
                    field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 0) {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if ((ch == 'x' || ch == 'u') && len_mod == 'l') {
        i += c_itoa(buf + i, buf_size - i, va_arg(ap, unsigned long),
                    ch == 'x' ? 16 : 10, flags, field_width);
      } else if (ch == 'p') {
        unsigned long num = (unsigned long) va_arg(ap, void *);
        C_SNPRINTF_APPEND_CHAR('0');
        C_SNPRINTF_APPEND_CHAR('x');
        i += c_itoa(buf + i, buf_size - i, num, 16, flags, 0);
      } else {
#ifndef NO_LIBC
        /*
         * TODO(lsm): abort is not nice in a library, remove it
         * Also, ESP8266 SDK doesn't have it
         */
        abort();
#endif
      }
    }
  }

  /* Zero-terminate the result */
  if (buf_size > 0) {
    buf[i < (int) buf_size ? i : (int) buf_size - 1] = '\0';
  }

  return i;
}
#endif

int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
  result = c_vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
  return result;
}

#ifdef _WIN32
void to_wchar(const char *path, wchar_t *wbuf, size_t wbuf_len) {
  char buf[MAX_PATH * 2], buf2[MAX_PATH * 2], *p;

  strncpy(buf, path, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  /* Trim trailing slashes. Leave backslash for paths like "X:\" */
  p = buf + strlen(buf) - 1;
  while (p > buf && p[-1] != ':' && (p[0] == '\\' || p[0] == '/')) *p-- = '\0';

  /*
   * Convert to Unicode and back. If doubly-converted string does not
   * match the original, something is fishy, reject.
   */
  memset(wbuf, 0, wbuf_len * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, (int) wbuf_len);
  WideCharToMultiByte(CP_UTF8, 0, wbuf, (int) wbuf_len, buf2, sizeof(buf2),
                      NULL, NULL);
  if (strcmp(buf, buf2) != 0) {
    wbuf[0] = L'\0';
  }
}
#endif /* _WIN32 */

#endif /* EXCLUDE_COMMON */
#ifdef NS_MODULE_LINES
#line 1 "src/../../common/dirent.c"
/**/
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef EXCLUDE_COMMON

/* Amalgamated: #include "osdep.h" */

/*
 * This file contains POSIX opendir/closedir/readdir API implementation
 * for systems which do not natively support it (e.g. Windows).
 */

#ifndef MG_FREE
#define MG_FREE free
#endif

#ifndef MG_MALLOC
#define MG_MALLOC malloc
#endif

#ifdef _WIN32
DIR *opendir(const char *name) {
  DIR *dir = NULL;
  wchar_t wpath[MAX_PATH];
  DWORD attrs;

  if (name == NULL) {
    SetLastError(ERROR_BAD_ARGUMENTS);
  } else if ((dir = (DIR *) MG_MALLOC(sizeof(*dir))) == NULL) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
  } else {
    to_wchar(name, wpath, ARRAY_SIZE(wpath));
    attrs = GetFileAttributesW(wpath);
    if (attrs != 0xFFFFFFFF && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
      (void) wcscat_s(wpath, MAX_PATH, L"\\*");
      dir->handle = FindFirstFileW(wpath, &dir->info);
      dir->result.d_name[0] = '\0';
    } else {
      MG_FREE(dir);
      dir = NULL;
    }
  }

  return dir;
}

int closedir(DIR *dir) {
  int result = 0;

  if (dir != NULL) {
    if (dir->handle != INVALID_HANDLE_VALUE)
      result = FindClose(dir->handle) ? 0 : -1;
    MG_FREE(dir);
  } else {
    result = -1;
    SetLastError(ERROR_BAD_ARGUMENTS);
  }

  return result;
}

struct dirent *readdir(DIR *dir) {
  struct dirent *result = 0;

  if (dir) {
    if (dir->handle != INVALID_HANDLE_VALUE) {
      result = &dir->result;
      (void) WideCharToMultiByte(CP_UTF8, 0, dir->info.cFileName, -1,
                                 result->d_name, sizeof(result->d_name), NULL,
                                 NULL);

      if (!FindNextFileW(dir->handle, &dir->info)) {
        (void) FindClose(dir->handle);
        dir->handle = INVALID_HANDLE_VALUE;
      }

    } else {
      SetLastError(ERROR_FILE_NOT_FOUND);
    }
  } else {
    SetLastError(ERROR_BAD_ARGUMENTS);
  }
  return result;
}
#endif

#endif /* EXCLUDE_COMMON */
#ifndef FROZEN_HEADER_INCLUDED
#ifdef NS_MODULE_LINES
#line 1 "src/../deps/frozen/frozen.c"
/**/
#endif
/*
 * Copyright (c) 2004-2013 Sergey Lyubka <valenok@gmail.com>
 * Copyright (c) 2013 Cesanta Software Limited
 * All rights reserved
 *
 * This library is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http: *www.gnu.org/licenses/>.
 *
 * You are free to use this library under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this library under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#define _CRT_SECURE_NO_WARNINGS /* Disable deprecation warning in VS2005+ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* Amalgamated: #include "frozen.h" */

#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifndef FROZEN_REALLOC
#define FROZEN_REALLOC realloc
#endif

#ifndef FROZEN_FREE
#define FROZEN_FREE free
#endif

struct frozen {
  const char *end;
  const char *cur;
  struct json_token *tokens;
  int max_tokens;
  int num_tokens;
  int do_realloc;
};

static int parse_object(struct frozen *f);
static int parse_value(struct frozen *f);

#define EXPECT(cond, err_code) do { if (!(cond)) return (err_code); } while (0)
#define TRY(expr) do { int _n = expr; if (_n < 0) return _n; } while (0)
#define END_OF_STRING (-1)

static int left(const struct frozen *f) {
  return f->end - f->cur;
}

static int is_space(int ch) {
  return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void skip_whitespaces(struct frozen *f) {
  while (f->cur < f->end && is_space(*f->cur)) f->cur++;
}

static int cur(struct frozen *f) {
  skip_whitespaces(f);
  return f->cur >= f->end ? END_OF_STRING : * (unsigned char *) f->cur;
}

static int test_and_skip(struct frozen *f, int expected) {
  int ch = cur(f);
  if (ch == expected) { f->cur++; return 0; }
  return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
}

static int is_alpha(int ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static int is_digit(int ch) {
  return ch >= '0' && ch <= '9';
}

static int is_hex_digit(int ch) {
  return is_digit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static int get_escape_len(const char *s, int len) {
  switch (*s) {
    case 'u':
      return len < 6 ? JSON_STRING_INCOMPLETE :
        is_hex_digit(s[1]) && is_hex_digit(s[2]) &&
        is_hex_digit(s[3]) && is_hex_digit(s[4]) ? 5 : JSON_STRING_INVALID;
    case '"': case '\\': case '/': case 'b':
    case 'f': case 'n': case 'r': case 't':
      return len < 2 ? JSON_STRING_INCOMPLETE : 1;
    default:
      return JSON_STRING_INVALID;
  }
}

static int capture_ptr(struct frozen *f, const char *ptr, enum json_type type) {
  if (f->do_realloc && f->num_tokens >= f->max_tokens) {
    int new_size = f->max_tokens == 0 ? 100 : f->max_tokens * 2;
    void *p = FROZEN_REALLOC(f->tokens, new_size * sizeof(f->tokens[0]));
    if (p == NULL) return JSON_TOKEN_ARRAY_TOO_SMALL;
    f->max_tokens = new_size;
    f->tokens = (struct json_token *) p;
  }
  if (f->tokens == NULL || f->max_tokens == 0) return 0;
  if (f->num_tokens >= f->max_tokens) return JSON_TOKEN_ARRAY_TOO_SMALL;
  f->tokens[f->num_tokens].ptr = ptr;
  f->tokens[f->num_tokens].type = type;
  f->num_tokens++;
  return 0;
}

static int capture_len(struct frozen *f, int token_index, const char *ptr) {
  if (f->tokens == 0 || f->max_tokens == 0) return 0;
  EXPECT(token_index >= 0 && token_index < f->max_tokens, JSON_STRING_INVALID);
  f->tokens[token_index].len = ptr - f->tokens[token_index].ptr;
  f->tokens[token_index].num_desc = (f->num_tokens - 1) - token_index;
  return 0;
}

/* identifier = letter { letter | digit | '_' } */
static int parse_identifier(struct frozen *f) {
  EXPECT(is_alpha(cur(f)), JSON_STRING_INVALID);
  TRY(capture_ptr(f, f->cur, JSON_TYPE_STRING));
  while (f->cur < f->end &&
         (*f->cur == '_' || is_alpha(*f->cur) || is_digit(*f->cur))) {
    f->cur++;
  }
  capture_len(f, f->num_tokens - 1, f->cur);
  return 0;
}

static int get_utf8_char_len(unsigned char ch) {
  if ((ch & 0x80) == 0) return 1;
  switch (ch & 0xf0) {
    case 0xf0: return 4;
    case 0xe0: return 3;
    default: return 2;
  }
}

/* string = '"' { quoted_printable_chars } '"' */
static int parse_string(struct frozen *f) {
  int n, ch = 0, len = 0;
  TRY(test_and_skip(f, '"'));
  TRY(capture_ptr(f, f->cur, JSON_TYPE_STRING));
  for (; f->cur < f->end; f->cur += len) {
    ch = * (unsigned char *) f->cur;
    len = get_utf8_char_len((unsigned char) ch);
    EXPECT(ch >= 32 && len > 0, JSON_STRING_INVALID);  /* No control chars */
    EXPECT(len < left(f), JSON_STRING_INCOMPLETE);
    if (ch == '\\') {
      EXPECT((n = get_escape_len(f->cur + 1, left(f))) > 0, n);
      len += n;
    } else if (ch == '"') {
      capture_len(f, f->num_tokens - 1, f->cur);
      f->cur++;
      break;
    };
  }
  return ch == '"' ? 0 : JSON_STRING_INCOMPLETE;
}

/* number = [ '-' ] digit+ [ '.' digit+ ] [ ['e'|'E'] ['+'|'-'] digit+ ] */
static int parse_number(struct frozen *f) {
  int ch = cur(f);
  TRY(capture_ptr(f, f->cur, JSON_TYPE_NUMBER));
  if (ch == '-') f->cur++;
  EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
  EXPECT(is_digit(f->cur[0]), JSON_STRING_INVALID);
  while (f->cur < f->end && is_digit(f->cur[0])) f->cur++;
  if (f->cur < f->end && f->cur[0] == '.') {
    f->cur++;
    EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
    EXPECT(is_digit(f->cur[0]), JSON_STRING_INVALID);
    while (f->cur < f->end && is_digit(f->cur[0])) f->cur++;
  }
  if (f->cur < f->end && (f->cur[0] == 'e' || f->cur[0] == 'E')) {
    f->cur++;
    EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
    if ((f->cur[0] == '+' || f->cur[0] == '-')) f->cur++;
    EXPECT(f->cur < f->end, JSON_STRING_INCOMPLETE);
    EXPECT(is_digit(f->cur[0]), JSON_STRING_INVALID);
    while (f->cur < f->end && is_digit(f->cur[0])) f->cur++;
  }
  capture_len(f, f->num_tokens - 1, f->cur);
  return 0;
}

/* array = '[' [ value { ',' value } ] ']' */
static int parse_array(struct frozen *f) {
  int ind;
  TRY(test_and_skip(f, '['));
  TRY(capture_ptr(f, f->cur - 1, JSON_TYPE_ARRAY));
  ind = f->num_tokens - 1;
  while (cur(f) != ']') {
    TRY(parse_value(f));
    if (cur(f) == ',') f->cur++;
  }
  TRY(test_and_skip(f, ']'));
  capture_len(f, ind, f->cur);
  return 0;
}

static int compare(const char *s, const char *str, int len) {
  int i = 0;
  while (i < len && s[i] == str[i]) i++;
  return i == len ? 1 : 0;
}

static int expect(struct frozen *f, const char *s, int len, enum json_type t) {
  int i, n = left(f);

  TRY(capture_ptr(f, f->cur, t));
  for (i = 0; i < len; i++) {
    if (i >= n) return JSON_STRING_INCOMPLETE;
    if (f->cur[i] != s[i]) return JSON_STRING_INVALID;
  }
  f->cur += len;
  TRY(capture_len(f, f->num_tokens - 1, f->cur));

  return 0;
}

/* value = 'null' | 'true' | 'false' | number | string | array | object */
static int parse_value(struct frozen *f) {
  int ch = cur(f);

  switch (ch) {
    case '"': TRY(parse_string(f)); break;
    case '{': TRY(parse_object(f)); break;
    case '[': TRY(parse_array(f)); break;
    case 'n': TRY(expect(f, "null", 4, JSON_TYPE_NULL)); break;
    case 't': TRY(expect(f, "true", 4, JSON_TYPE_TRUE)); break;
    case 'f': TRY(expect(f, "false", 5, JSON_TYPE_FALSE)); break;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      TRY(parse_number(f));
      break;
    default:
      return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
  }

  return 0;
}

/* key = identifier | string */
static int parse_key(struct frozen *f) {
  int ch = cur(f);
#if 0
  printf("%s 1 [%.*s]\n", __func__, (int) (f->end - f->cur), f->cur);
#endif
  if (is_alpha(ch)) {
    TRY(parse_identifier(f));
  } else if (ch == '"') {
    TRY(parse_string(f));
  } else {
    return ch == END_OF_STRING ? JSON_STRING_INCOMPLETE : JSON_STRING_INVALID;
  }
  return 0;
}

/* pair = key ':' value */
static int parse_pair(struct frozen *f) {
  TRY(parse_key(f));
  TRY(test_and_skip(f, ':'));
  TRY(parse_value(f));
  return 0;
}

/* object = '{' pair { ',' pair } '}' */
static int parse_object(struct frozen *f) {
  int ind;
  TRY(test_and_skip(f, '{'));
  TRY(capture_ptr(f, f->cur - 1, JSON_TYPE_OBJECT));
  ind = f->num_tokens - 1;
  while (cur(f) != '}') {
    TRY(parse_pair(f));
    if (cur(f) == ',') f->cur++;
  }
  TRY(test_and_skip(f, '}'));
  capture_len(f, ind, f->cur);
  return 0;
}

static int doit(struct frozen *f) {
  if (f->cur == 0 || f->end < f->cur) return JSON_STRING_INVALID;
  if (f->end == f->cur) return JSON_STRING_INCOMPLETE;
  TRY(parse_object(f));
  TRY(capture_ptr(f, f->cur, JSON_TYPE_EOF));
  capture_len(f, f->num_tokens, f->cur);
  return 0;
}

/* json = object */
int parse_json(const char *s, int s_len, struct json_token *arr, int arr_len) {
  struct frozen frozen;

  memset(&frozen, 0, sizeof(frozen));
  frozen.end = s + s_len;
  frozen.cur = s;
  frozen.tokens = arr;
  frozen.max_tokens = arr_len;

  TRY(doit(&frozen));

  return frozen.cur - s;
}

struct json_token *parse_json2(const char *s, int s_len) {
  struct frozen frozen;

  memset(&frozen, 0, sizeof(frozen));
  frozen.end = s + s_len;
  frozen.cur = s;
  frozen.do_realloc = 1;

  if (doit(&frozen) < 0) {
    FROZEN_FREE((void *) frozen.tokens);
    frozen.tokens = NULL;
  }
  return frozen.tokens;
}

static int path_part_len(const char *p) {
  int i = 0;
  while (p[i] != '\0' && p[i] != '[' && p[i] != '.') i++;
  return i;
}

struct json_token *find_json_token(struct json_token *toks, const char *path) {
  while (path != 0 && path[0] != '\0') {
    int i, ind2 = 0, ind = -1, skip = 2, n = path_part_len(path);
    if (path[0] == '[') {
      if (toks->type != JSON_TYPE_ARRAY || !is_digit(path[1])) return 0;
      for (ind = 0, n = 1; path[n] != ']' && path[n] != '\0'; n++) {
        if (!is_digit(path[n])) return 0;
        ind *= 10;
        ind += path[n] - '0';
      }
      if (path[n++] != ']') return 0;
      skip = 1;  /* In objects, we skip 2 elems while iterating, in arrays 1. */
    } else if (toks->type != JSON_TYPE_OBJECT) return 0;
    toks++;
    for (i = 0; i < toks[-1].num_desc; i += skip, ind2++) {
      /* ind == -1 indicated that we're iterating an array, not object */
      if (ind == -1 && toks[i].type != JSON_TYPE_STRING) return 0;
      if (ind2 == ind ||
          (ind == -1 && toks[i].len == n && compare(path, toks[i].ptr, n))) {
        i += skip - 1;
        break;
      };
      if (toks[i - 1 + skip].type == JSON_TYPE_ARRAY ||
          toks[i - 1 + skip].type == JSON_TYPE_OBJECT) {
        i += toks[i - 1 + skip].num_desc;
      }
    }
    if (i == toks[-1].num_desc) return 0;
    path += n;
    if (path[0] == '.') path++;
    if (path[0] == '\0') return &toks[i];
    toks += i;
  }
  return 0;
}

int json_emit_long(char *buf, int buf_len, long int value) {
  char tmp[20];
  int n = snprintf(tmp, sizeof(tmp), "%ld", value);
  strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
  return n;
}

int json_emit_double(char *buf, int buf_len, double value) {
  char tmp[20];
  int n = snprintf(tmp, sizeof(tmp), "%g", value);
  strncpy(buf, tmp, buf_len > 0 ? buf_len : 0);
  return n;
}

int json_emit_quoted_str(char *s, int s_len, const char *str, int len) {
  const char *begin = s, *end = s + s_len, *str_end = str + len;
  char ch;

#define EMIT(x) do { if (s < end) *s = x; s++; } while (0)

  EMIT('"');
  while (str < str_end) {
    ch = *str++;
    switch (ch) {
      case '"':  EMIT('\\'); EMIT('"'); break;
      case '\\': EMIT('\\'); EMIT('\\'); break;
      case '\b': EMIT('\\'); EMIT('b'); break;
      case '\f': EMIT('\\'); EMIT('f'); break;
      case '\n': EMIT('\\'); EMIT('n'); break;
      case '\r': EMIT('\\'); EMIT('r'); break;
      case '\t': EMIT('\\'); EMIT('t'); break;
      default: EMIT(ch);
    }
  }
  EMIT('"');
  if (s < end) {
    *s = '\0';
  }

  return s - begin;
}

int json_emit_unquoted_str(char *buf, int buf_len, const char *str, int len) {
  if (buf_len > 0 && len > 0) {
    int n = len < buf_len ? len : buf_len;
    memcpy(buf, str, n);
    if (n < buf_len) {
      buf[n] = '\0';
    }
  }
  return len;
}

int json_emit_va(char *s, int s_len, const char *fmt, va_list ap) {
  const char *end = s + s_len, *str, *orig = s;
  size_t len;

  while (*fmt != '\0') {
    switch (*fmt) {
      case '[': case ']': case '{': case '}': case ',': case ':':
      case ' ': case '\r': case '\n': case '\t':
        if (s < end) {
          *s = *fmt;
        }
        s++;
        break;
      case 'i':
        s += json_emit_long(s, end - s, va_arg(ap, long));
        break;
      case 'f':
        s += json_emit_double(s, end - s, va_arg(ap, double));
        break;
      case 'v':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_quoted_str(s, end - s, str, len);
        break;
      case 'V':
        str = va_arg(ap, char *);
        len = va_arg(ap, size_t);
        s += json_emit_unquoted_str(s, end - s, str, len);
        break;
      case 's':
        str = va_arg(ap, char *);
        s += json_emit_quoted_str(s, end - s, str, strlen(str));
        break;
      case 'S':
        str = va_arg(ap, char *);
        s += json_emit_unquoted_str(s, end - s, str, strlen(str));
        break;
      case 'T':
        s += json_emit_unquoted_str(s, end - s, "true", 4);
        break;
      case 'F':
        s += json_emit_unquoted_str(s, end - s, "false", 5);
        break;
      case 'N':
        s += json_emit_unquoted_str(s, end - s, "null", 4);
        break;
      default:
        return 0;
    }
    fmt++;
  }

  /* Best-effort to 0-terminate generated string */
  if (s < end) {
    *s = '\0';
  }

  return s - orig;
}

int json_emit(char *buf, int buf_len, const char *fmt, ...) {
  int len;
  va_list ap;

  va_start(ap, fmt);
  len = json_emit_va(buf, buf_len, fmt, ap);
  va_end(ap);

  return len;
}
#endif /*FROZEN_HEADER_INCLUDED*/
#ifdef NS_MODULE_LINES
#line 1 "src/net.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/* Amalgamated: #include "internal.h" */

#if MG_MGR_EV_MGR == 1 /* epoll() */
#include <sys/epoll.h>
#endif

#define MG_CTL_MSG_MESSAGE_SIZE 8192
#define MG_READ_BUFFER_SIZE 1024
#define MG_UDP_RECEIVE_BUFFER_SIZE 1500
#define MG_VPRINTF_BUFFER_SIZE 100
#define MG_MAX_HOST_LEN 200

#define MG_COPY_COMMON_CONNECTION_OPTIONS(dst, src) \
  memcpy(dst, src, sizeof(*dst));

/* Which flags can be pre-set by the user at connection creation time. */
#define _MG_ALLOWED_CONNECT_FLAGS_MASK                                   \
  (MG_F_USER_1 | MG_F_USER_2 | MG_F_USER_3 | MG_F_USER_4 | MG_F_USER_5 | \
   MG_F_USER_6 | MG_F_WEBSOCKET_NO_DEFRAG)
/* Which flags should be modifiable by user's callbacks. */
#define _MG_CALLBACK_MODIFIABLE_FLAGS_MASK                               \
  (MG_F_USER_1 | MG_F_USER_2 | MG_F_USER_3 | MG_F_USER_4 | MG_F_USER_5 | \
   MG_F_USER_6 | MG_F_WEBSOCKET_NO_DEFRAG | MG_F_SEND_AND_CLOSE |        \
   MG_F_DONT_SEND | MG_F_CLOSE_IMMEDIATELY | MG_F_IS_WEBSOCKET)

#ifndef intptr_t
#define intptr_t long
#endif

struct ctl_msg {
  mg_event_handler_t callback;
  char message[MG_CTL_MSG_MESSAGE_SIZE];
};

static void mg_ev_mgr_init(struct mg_mgr *mgr);
static void mg_ev_mgr_free(struct mg_mgr *mgr);
static void mg_ev_mgr_add_conn(struct mg_connection *nc);
static void mg_ev_mgr_remove_conn(struct mg_connection *nc);

MG_INTERNAL void mg_add_conn(struct mg_mgr *mgr, struct mg_connection *c) {
  c->mgr = mgr;
  c->next = mgr->active_connections;
  mgr->active_connections = c;
  c->prev = NULL;
  if (c->next != NULL) c->next->prev = c;
  mg_ev_mgr_add_conn(c);
}

MG_INTERNAL void mg_remove_conn(struct mg_connection *conn) {
  if (conn->prev == NULL) conn->mgr->active_connections = conn->next;
  if (conn->prev) conn->prev->next = conn->next;
  if (conn->next) conn->next->prev = conn->prev;
  mg_ev_mgr_remove_conn(conn);
}

MG_INTERNAL void mg_call(struct mg_connection *nc, int ev, void *ev_data) {
  unsigned long flags_before;
  mg_event_handler_t ev_handler;

  DBG(("%p flags=%lu ev=%d ev_data=%p rmbl=%d", nc, nc->flags, ev, ev_data,
       (int) nc->recv_mbuf.len));

#ifndef MG_DISABLE_FILESYSTEM
  /* LCOV_EXCL_START */
  if (nc->mgr->hexdump_file != NULL && ev != MG_EV_POLL &&
      ev != MG_EV_SEND /* handled separately */) {
    int len = (ev == MG_EV_RECV ? *(int *) ev_data : 0);
    mg_hexdump_connection(nc, nc->mgr->hexdump_file, len, ev);
  }
/* LCOV_EXCL_STOP */
#endif

  /*
   * If protocol handler is specified, call it. Otherwise, call user-specified
   * event handler.
   */
  ev_handler = nc->proto_handler ? nc->proto_handler : nc->handler;
  if (ev_handler != NULL) {
    flags_before = nc->flags;
    ev_handler(nc, ev, ev_data);
    if (nc->flags != flags_before) {
      nc->flags = (flags_before & ~_MG_CALLBACK_MODIFIABLE_FLAGS_MASK) |
                  (nc->flags & _MG_CALLBACK_MODIFIABLE_FLAGS_MASK);
    }
  }
  DBG(("call done, flags %d", (int) nc->flags));
}

static size_t mg_out(struct mg_connection *nc, const void *buf, size_t len) {
  if (nc->flags & MG_F_UDP) {
    ssize_t n = sendto(nc->sock, buf, len, 0, &nc->sa.sa, sizeof(nc->sa.sin));
#ifdef MG_ENABLE_DEBUG
    char buf[32] = {0};
    DBG(("%p %d %d %d %s:%hu", nc, nc->sock, n, errno,
         ipv4ToStr(&nc->sa.sin.sin_addr, buf, sizeof(buf)-1), ntohs(nc->sa.sin.sin_port)));
#endif
    return n < 0 ? 0 : n;
  } else {
    return mbuf_append(&nc->send_mbuf, buf, len);
  }
}

static void mg_destroy_conn(struct mg_connection *conn) {
  if (conn->sock != INVALID_SOCKET) {
    closesocket(conn->sock);
    /*
     * avoid users accidentally double close a socket
     * because it can lead to difficult to debug situations.
     * It would happen only if reusing a destroyed mg_connection
     * but it's not always possible to run the code through an
     * address sanitizer.
     */
    conn->sock = INVALID_SOCKET;
  }
  mbuf_free(&conn->recv_mbuf);
  mbuf_free(&conn->send_mbuf);
  MG_FREE(conn);
}

static void mg_close_conn(struct mg_connection *conn) {
  DBG(("%p %lu", conn, conn->flags));
  if (!(conn->flags & MG_F_CONNECTING)) {
    mg_call(conn, MG_EV_CLOSE, NULL);
  }
  mg_remove_conn(conn);
  mg_destroy_conn(conn);
}

void mg_mgr_init(struct mg_mgr *s, void *user_data) {
  memset(s, 0, sizeof(*s));
  s->ctl[0] = s->ctl[1] = INVALID_SOCKET;
  s->user_data = user_data;

#ifdef _WIN32
  {
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
  }
#elif !defined(AVR_LIBC) && !defined(MG_ESP8266)
  /* Ignore SIGPIPE signal, so if client cancels the request, it
   * won't kill the whole process. */
  signal(SIGPIPE, SIG_IGN);
#endif

  mg_ev_mgr_init(s);
  DBG(("=================================="));
  DBG(("init mgr=%p", s));
}

void mg_mgr_free(struct mg_mgr *s) {
  struct mg_connection *conn, *tmp_conn;

  DBG(("%p", s));
  if (s == NULL) return;
  /* Do one last poll, see https://github.com/cesanta/mongoose/issues/286 */
  mg_mgr_poll(s, 0);

  if (s->ctl[0] != INVALID_SOCKET) closesocket(s->ctl[0]);
  if (s->ctl[1] != INVALID_SOCKET) closesocket(s->ctl[1]);
  s->ctl[0] = s->ctl[1] = INVALID_SOCKET;

  for (conn = s->active_connections; conn != NULL; conn = tmp_conn) {
    tmp_conn = conn->next;
    mg_close_conn(conn);
  }

  mg_ev_mgr_free(s);
}

int mg_vprintf(struct mg_connection *nc, const char *fmt, va_list ap) {
  char mem[MG_VPRINTF_BUFFER_SIZE], *buf = mem;
  int len;

  if ((len = mg_avprintf(&buf, sizeof(mem), fmt, ap)) > 0) {
    mg_out(nc, buf, len);
  }
  if (buf != mem && buf != NULL) {
    MG_FREE(buf); /* LCOV_EXCL_LINE */
  }               /* LCOV_EXCL_LINE */

  return len;
}

int mg_printf(struct mg_connection *conn, const char *fmt, ...) {
  int len;
  va_list ap;
  va_start(ap, fmt);
  len = mg_vprintf(conn, fmt, ap);
  va_end(ap);
  return len;
}

static void mg_set_non_blocking_mode(sock_t sock) {
#ifdef _WIN32
  unsigned long on = 1;
  ioctlsocket(sock, FIONBIO, &on);
#elif defined(MG_CC3200)
  cc3200_set_non_blocking_mode(sock);
#else
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

#if 0
/* TODO(lsm): use non-blocking resolver */
static int mg_resolve2(const char *host, struct in_addr *ina) {
  struct hostent *he;
  if ((he = gethostbyname(host)) == NULL) {
    DBG(("gethostbyname(%s) failed: %s", host, strerror(errno)));
  } else {
    memcpy(ina, he->h_addr_list[0], sizeof(*ina));
    return 1;
  }
  return 0;
}

int mg_resolve(const char *host, char *buf, size_t n) {
  struct in_addr ad;
  return mg_resolve2(host, &ad) ? snprintf(buf, n, "%s", inet_ntoa(ad)) : 0;
}
#endif

MG_INTERNAL struct mg_connection *mg_create_connection(
    struct mg_mgr *mgr, mg_event_handler_t callback,
    struct mg_add_sock_opts opts) {
  struct mg_connection *conn;

  if ((conn = (struct mg_connection *) MG_MALLOC(sizeof(*conn))) != NULL) {
    memset(conn, 0, sizeof(*conn));
    conn->sock = INVALID_SOCKET;
    conn->handler = callback;
    conn->mgr = mgr;
    conn->last_io_time = time(NULL);
    conn->flags = opts.flags & _MG_ALLOWED_CONNECT_FLAGS_MASK;
    conn->user_data = opts.user_data;
    /*
     * SIZE_MAX is defined as a long long constant in
     * system headers on some platforms and so it
     * doesn't compile with pedantic ansi flags.
     */
    conn->recv_mbuf_limit = ~0;
  }

  return conn;
}

/* Associate a socket to a connection and and add to the manager. */
MG_INTERNAL void mg_set_sock(struct mg_connection *nc, sock_t sock) {
#if !defined(MG_CC3200) && !defined(MG_ESP8266)
  /* Can't get non-blocking connect to work.
   * TODO(rojer): Figure out why it fails where blocking succeeds.
   */
  mg_set_non_blocking_mode(sock);
#endif
  mg_set_close_on_exec(sock);
  nc->sock = sock;
  mg_add_conn(nc->mgr, nc);

  DBG(("%p %d", nc, sock));
}

/*
 * Address format: [PROTO://][HOST]:PORT
 *
 * HOST could be IPv4/IPv6 address or a host name.
 * `host` is a destination buffer to hold parsed HOST part. Shoud be at least
 * MG_MAX_HOST_LEN bytes long.
 * `proto` is a returned socket type, either SOCK_STREAM or SOCK_DGRAM
 *
 * Return:
 *   -1   on parse error
 *    0   if HOST needs DNS lookup
 *   >0   length of the address string
 */
MG_INTERNAL int mg_parse_address(const char *str, union socket_address *sa,
                                 int *proto, char *host, size_t host_len) {
  unsigned int a, b, c, d, port = 0;
  int len = 0;

  /*
   * MacOS needs that. If we do not zero it, subsequent bind() will fail.
   * Also, all-zeroes in the socket address means binding to all addresses
   * for both IPv4 and IPv6 (INADDR_ANY and IN6ADDR_ANY_INIT).
   */
  memset(sa, 0, sizeof(*sa));
  sa->sin.sin_family = AF_INET;

  *proto = SOCK_STREAM;

  if (strncmp(str, "udp://", 6) == 0) {
    str += 6;
    *proto = SOCK_DGRAM;
  } else if (strncmp(str, "tcp://", 6) == 0) {
    str += 6;
  }

  if (sscanf(str, "%u.%u.%u.%u:%u%n", &a, &b, &c, &d, &port, &len) == 5) {
    /* Bind to a specific IPv4 address, e.g. 192.168.1.5:8080 */
    sa->sin.sin_addr.s_addr =
        htonl(((uint32_t) a << 24) | ((uint32_t) b << 16) | c << 8 | d);
    sa->sin.sin_port = htons((uint16_t) port);
  } else if (sscanf(str, ":%u%n", &port, &len) == 1 ||
             sscanf(str, "%u%n", &port, &len) == 1) {
    /* If only port is specified, bind to IPv4, INADDR_ANY */
    sa->sin.sin_port = htons((uint16_t) port);
  } else {
    return -1;
  }

  return port < 0xffffUL && str[len] == '\0' ? len : -1;
}

/* 'sa' must be an initialized address to bind to */
static sock_t mg_open_listening_socket(union socket_address *sa, int proto) {
  socklen_t sa_len =
      (sa->sa.sa_family == AF_INET) ? sizeof(sa->sin) : sizeof(sa->sin6);
  sock_t sock = INVALID_SOCKET;
#ifndef MG_CC3200
  int on = 1;
#endif

  if ((sock = MYSAFE_SOCKET_FOR_SELECT(sa->sa.sa_family, proto, 0)) != INVALID_SOCKET &&
#ifndef MG_CC3200 /* CC3200 doesn't support either */
#if defined(_WIN32) && defined(SO_EXCLUSIVEADDRUSE)
      /* "Using SO_REUSEADDR and SO_EXCLUSIVEADDRUSE" http://goo.gl/RmrFTm */
      !setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (void *) &on,
                  sizeof(on)) &&
#endif

#if !defined(_WIN32) || !defined(SO_EXCLUSIVEADDRUSE)
      /*
       * SO_RESUSEADDR is not enabled on Windows because the semantics of
       * SO_REUSEADDR on UNIX and Windows is different. On Windows,
       * SO_REUSEADDR allows to bind a socket to a port without error even if
       * the port is already open by another program. This is not the behavior
       * SO_REUSEADDR was designed for, and leads to hard-to-track failure
       * scenarios. Therefore, SO_REUSEADDR was disabled on Windows unless
       * SO_EXCLUSIVEADDRUSE is supported and set on a socket.
       */
      !setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on)) &&
#endif
#endif /* !MG_CC3200 */

      !bind(sock, &sa->sa, sa_len) &&
      (proto == SOCK_DGRAM || listen(sock, SOMAXCONN) == 0)) {
#ifndef MG_CC3200 /* TODO(rojer): Fix this. */
    mg_set_non_blocking_mode(sock);
    /* In case port was set to 0, get the real port number */
    (void) getsockname(sock, &sa->sa, &sa_len);
#endif
  } else if (sock != INVALID_SOCKET) {
    closesocket(sock);
    sock = INVALID_SOCKET;
  }

  return sock;
}

static struct mg_connection *accept_conn(struct mg_connection *ls) {
  struct mg_connection *c = NULL;
  union socket_address sa;
  socklen_t len = sizeof(sa);
  sock_t sock = INVALID_SOCKET;

  /* NOTE(lsm): on Windows, sock is always > FD_SETSIZE */
  if ((sock = accept(ls->sock, &sa.sa, &len)) == INVALID_SOCKET) {
      DBG(("accept failed,errno:%d", errno));
  } else if ((c = mg_add_sock(ls->mgr, sock, ls->handler)) == NULL) {
      DBG(("mg_add_sock failed"));
    closesocket(sock);
  } else {
    c->listener = ls;
    c->proto_data = ls->proto_data;
    c->proto_handler = ls->proto_handler;
    c->user_data = ls->user_data;
    c->recv_mbuf_limit = ls->recv_mbuf_limit;
    if (c->ssl == NULL) { /* SSL connections need to perform handshake. */
      mg_call(c, MG_EV_ACCEPT, &sa);
    }
    DBG(("%p %d %p %p", c, c->sock, c->ssl_ctx, c->ssl));
  }

  return c;
}

static int mg_is_error(int n) {
#ifdef MG_CC3200
  DBG(("n = %d, errno = %d", n, errno));
  if (n < 0) errno = n;
#endif
  return n == 0 || (n < 0 && errno != EINTR && errno != EINPROGRESS &&
                    errno != EAGAIN && errno != EWOULDBLOCK
#ifdef MG_CC3200
                    && errno != SL_EALREADY
#endif
#ifdef _WIN32
                    && WSAGetLastError() != WSAEINTR &&
                    WSAGetLastError() != WSAEWOULDBLOCK
#endif
                    );
}

static size_t recv_avail_size(struct mg_connection *conn, size_t max) {
  size_t avail;
  if (conn->recv_mbuf_limit < conn->recv_mbuf.len) return 0;
  avail = conn->recv_mbuf_limit - conn->recv_mbuf.len;
  return avail > max ? max : avail;
}

static void mg_read_from_socket(struct mg_connection *conn) {
  char buf[MG_READ_BUFFER_SIZE];
  int n = 0;

  if (conn->flags & MG_F_CONNECTING) {
    int ok = 1, ret;
#if !defined(MG_CC3200) && !defined(MG_ESP8266)
    socklen_t len = sizeof(ok);
#endif

    (void) ret;
#if defined(MG_CC3200) || defined(MG_ESP8266)
    /* On CC3200 and ESP8266 we use blocking connect. If we got as far as this,
     * this means connect() was successful.
     * TODO(rojer): Figure out why it fails where blocking succeeds.
     */
    mg_set_non_blocking_mode(conn->sock);
    ret = ok = 0;
#else
    ret = getsockopt(conn->sock, SOL_SOCKET, SO_ERROR, (char *) &ok, &len);
#endif
    DBG(("%p connect ok=%d", conn, ok));
    if (ok != 0) {
      conn->flags |= MG_F_CLOSE_IMMEDIATELY;
    } else {
      conn->flags &= ~MG_F_CONNECTING;
    }
    mg_call(conn, MG_EV_CONNECT, &ok);
    return;
  }

    while ((n = (int) MG_EV_RECV_FUNC(
                conn->sock, buf, recv_avail_size(conn, sizeof(buf)), 0)) > 0) {
      DBG(("%p %d bytes (PLAIN) <- %d", conn, n, conn->sock));
      mbuf_append(&conn->recv_mbuf, buf, n);
      mg_call(conn, MG_EV_RECV, &n);
      if (conn->flags & MG_F_CLOSE_IMMEDIATELY) break;
    }
  DBG(("recv returns %d", n));

  if (mg_is_error(n)) {
      DBG(("recv error:%p",conn));
    conn->flags |= MG_F_CLOSE_IMMEDIATELY;
  }
}

static void mg_write_to_socket(struct mg_connection *conn) {
  struct mbuf *io = &conn->send_mbuf;
  int n = 0;

  assert(io->len > 0);

  {
    n = (int) MG_EV_SEND_FUNC(conn->sock, io->buf, io->len, 0);
  }

  DBG(("%p %d bytes -> %d", conn, n, conn->sock));

  if (mg_is_error(n)) {
    conn->flags |= MG_F_CLOSE_IMMEDIATELY;
  } else if (n > 0) {
#ifndef MG_DISABLE_FILESYSTEM
    /* LCOV_EXCL_START */
    if (conn->mgr->hexdump_file != NULL) {
      mg_hexdump_connection(conn, conn->mgr->hexdump_file, n, MG_EV_SEND);
    }
/* LCOV_EXCL_STOP */
#endif
    mbuf_remove(io, n);
  }
  mg_call(conn, MG_EV_SEND, &n);
}

int mg_send(struct mg_connection *conn, const void *buf, int len) {
  return (int) mg_out(conn, buf, len);
}

static void mg_handle_udp(struct mg_connection *ls) {
  struct mg_connection nc;
  char buf[MG_UDP_RECEIVE_BUFFER_SIZE];
  int n;
  socklen_t s_len = sizeof(nc.sa);

  memset(&nc, 0, sizeof(nc));
  n = (int)recvfrom(ls->sock, buf, sizeof(buf), 0, &nc.sa.sa, &s_len);
  if (n <= 0) {
    DBG(("%p recvfrom: %s", ls, strerror(errno)));
  } else {
    union socket_address sa = nc.sa;
    /* Copy all attributes, preserving sender address */
    nc = *ls;

    /* Then override some */
    nc.sa = sa;
    nc.recv_mbuf.buf = buf;
    nc.recv_mbuf.len = nc.recv_mbuf.size = n;
    nc.listener = ls;
    nc.flags = MG_F_UDP;

    /* Call MG_EV_RECV handler */
    DBG(("%p %d bytes received", ls, n));
    mg_call(&nc, MG_EV_RECV, &n);

    /*
     * See https://github.com/cesanta/mongoose/issues/207
     * mg_call migth set flags. They need to be synced back to ls.
     */
    ls->flags = nc.flags;
  }
}

#define _MG_F_FD_CAN_READ 1
#define _MG_F_FD_CAN_WRITE 1 << 1
#define _MG_F_FD_ERROR 1 << 2

//定义严重错误，当发生严重错误时，表示本地代理已经不可用
#define _MG_HARD_ERROR 500

static int mg_mgr_handle_connection(struct mg_connection *nc, int fd_flags,
                                     time_t now) {
  DBG(("%p fd=%d fd_flags=%d nc_flags=%lu rmbl=%d smbl=%d", nc, nc->sock,
       fd_flags, nc->flags, (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
  if (fd_flags != 0) nc->last_io_time = now;

  if (nc->flags & MG_F_CONNECTING) {
    if (fd_flags != 0) {
      mg_read_from_socket(nc);
    }
    return 0;
  }

  if (nc->flags & MG_F_LISTENING) {
    /*
     * We're not looping here, and accepting just one connection at
     * a time. The reason is that eCos does not respect non-blocking
     * flag on a listening socket and hangs in a loop.
     */
    if (fd_flags & _MG_F_FD_CAN_READ)
    {
        //本地代理的监听socket连续10次accept失败，置严重错误标识
        if(NULL == accept_conn(nc))
        {
            ++(nc->error_counter);
            if(nc->error_counter > 10)
            {
                closesocket(nc->sock);
                nc->sock = INVALID_SOCKET;
                return _MG_HARD_ERROR;
            }
        }
        else
        {
            nc->error_counter = 0;
        }
    }
      
    return 0;
  }

  if (fd_flags & _MG_F_FD_CAN_READ) {
    if (nc->flags & MG_F_UDP) {
      mg_handle_udp(nc);
    } else {
      mg_read_from_socket(nc);
    }
    if (nc->flags & MG_F_CLOSE_IMMEDIATELY) return 0;
  }

  if ((fd_flags & _MG_F_FD_CAN_WRITE) && !(nc->flags & MG_F_DONT_SEND) &&
      !(nc->flags & MG_F_UDP)) { /* Writes to UDP sockets are not buffered. */
    mg_write_to_socket(nc);
  }

  if ( (fd_flags & _MG_F_FD_ERROR) ) {
    int err = 0;
    socklen_t errlen = sizeof(err);
    if (getsockopt(nc->sock, SOL_SOCKET, SO_ERROR, &err, &errlen) != 0) {
      err = errno;
    }
    mg_call(nc, MG_EV_ERROR, &err);
    nc->flags |= MG_F_CLOSE_IMMEDIATELY; // if error occur, close immediately 
  }

  if (!(fd_flags & (_MG_F_FD_CAN_READ | _MG_F_FD_CAN_WRITE | _MG_F_FD_ERROR))) {
    mg_call(nc, MG_EV_POLL, &now);
  }

  DBG(("%p after fd=%d nc_flags=%lu rmbl=%d smbl=%d", nc, nc->sock, nc->flags,
       (int) nc->recv_mbuf.len, (int) nc->send_mbuf.len));
    
    return 0;
}

#if MG_MGR_EV_MGR == 1 /* epoll() */

#ifndef MG_EPOLL_MAX_EVENTS
#define MG_EPOLL_MAX_EVENTS 100
#endif

#define _MG_EPF_EV_EPOLLIN (1 << 0)
#define _MG_EPF_EV_EPOLLOUT (1 << 1)
#define _MG_EPF_NO_POLL (1 << 2)

static uint32_t mg_epf_to_evflags(unsigned int epf) {
  uint32_t result = 0;
  if (epf & _MG_EPF_EV_EPOLLIN) result |= EPOLLIN;
  if (epf & _MG_EPF_EV_EPOLLOUT) result |= EPOLLOUT;
  return result;
}

static void mg_ev_mgr_epoll_set_flags(const struct mg_connection *nc,
                                      struct epoll_event *ev) {
  /* NOTE: EPOLLERR and EPOLLHUP are always enabled. */
  ev->events = 0;
  if (nc->recv_mbuf.len < nc->recv_mbuf_limit) {
    ev->events |= EPOLLIN;
  }
  if ((nc->flags & MG_F_CONNECTING) ||
      (nc->send_mbuf.len > 0 && !(nc->flags & MG_F_DONT_SEND))) {
    ev->events |= EPOLLOUT;
  }
}

static void mg_ev_mgr_epoll_ctl(struct mg_connection *nc, int op) {
  int epoll_fd = (intptr_t) nc->mgr->mgr_data;
  struct epoll_event ev;
  assert(op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD || EPOLL_CTL_DEL);
  if (op != EPOLL_CTL_DEL) {
    mg_ev_mgr_epoll_set_flags(nc, &ev);
    if (op == EPOLL_CTL_MOD) {
      uint32_t old_ev_flags = mg_epf_to_evflags((intptr_t) nc->mgr_data);
      if (ev.events == old_ev_flags) return;
    }
    ev.data.ptr = nc;
  }
  if (epoll_ctl(epoll_fd, op, nc->sock, &ev) != 0) {
    perror("epoll_ctl");
    abort();
  }
}

static void mg_ev_mgr_init(struct mg_mgr *mgr) {
  int epoll_fd;
  DBG(("%p using epoll()", mgr));
  epoll_fd = epoll_create(MG_EPOLL_MAX_EVENTS /* unused but required */);
  if (epoll_fd < 0) {
    perror("epoll_ctl");
    abort();
  }
  mgr->mgr_data = (void *) ((intptr_t) epoll_fd);
  if (mgr->ctl[1] != INVALID_SOCKET) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mgr->ctl[1], &ev) != 0) {
      perror("epoll_ctl");
      abort();
    }
  }
}

static void mg_ev_mgr_free(struct mg_mgr *mgr) {
  int epoll_fd = (intptr_t) mgr->mgr_data;
  close(epoll_fd);
}

static void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  mg_ev_mgr_epoll_ctl(nc, EPOLL_CTL_ADD);
}

static void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  mg_ev_mgr_epoll_ctl(nc, EPOLL_CTL_DEL);
}

time_t mg_mgr_poll(struct mg_mgr *mgr, int timeout_ms) {
  int epoll_fd = (intptr_t) mgr->mgr_data;
  struct epoll_event events[MG_EPOLL_MAX_EVENTS];
  struct mg_connection *nc, *next;
  int num_ev, fd_flags;
  time_t now;

  num_ev = epoll_wait(epoll_fd, events, MG_EPOLL_MAX_EVENTS, timeout_ms);
  now = time(NULL);
  DBG(("epoll_wait @ %ld num_ev=%d", (long) now, num_ev));

  while (num_ev-- > 0) {
    intptr_t epf;
    struct epoll_event *ev = events + num_ev;
    nc = (struct mg_connection *) ev->data.ptr;
    if (nc == NULL) {
      mg_mgr_handle_ctl_sock(mgr);
      continue;
    }
    fd_flags = ((ev->events & (EPOLLIN | EPOLLHUP)) ? _MG_F_FD_CAN_READ : 0) |
               ((ev->events & (EPOLLOUT)) ? _MG_F_FD_CAN_WRITE : 0) |
               ((ev->events & (EPOLLERR)) ? _MG_F_FD_ERROR : 0);
    mg_mgr_handle_connection(nc, fd_flags, now);
    epf = (intptr_t) nc->mgr_data;
    epf ^= _MG_EPF_NO_POLL;
    nc->mgr_data = (void *) epf;
  }

  for (nc = mgr->active_connections; nc != NULL; nc = next) {
    next = nc->next;
    if (!(((intptr_t) nc->mgr_data) & _MG_EPF_NO_POLL)) {
      mg_mgr_handle_connection(nc, 0, now);
    } else {
      intptr_t epf = (intptr_t) nc->mgr_data;
      epf ^= _MG_EPF_NO_POLL;
      nc->mgr_data = (void *) epf;
    }
    if ((nc->flags & MG_F_CLOSE_IMMEDIATELY) ||
        (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE))) {
      mg_close_conn(nc);
    } else {
      mg_ev_mgr_epoll_ctl(nc, EPOLL_CTL_MOD);
    }
  }

  return now;
}

#else /* select() */

static void mg_ev_mgr_init(struct mg_mgr *mgr) {
  (void) mgr;
  DBG(("%p using select()", mgr));
}

static void mg_ev_mgr_free(struct mg_mgr *mgr) {
  (void) mgr;
}

static void mg_ev_mgr_add_conn(struct mg_connection *nc) {
  (void) nc;
}

static void mg_ev_mgr_remove_conn(struct mg_connection *nc) {
  (void) nc;
}

static void mg_add_to_set(sock_t sock, fd_set *set, sock_t *max_fd) {
  if (sock != INVALID_SOCKET) {
    FD_SET(sock, set);
    if (*max_fd == INVALID_SOCKET || sock > *max_fd) {
      *max_fd = sock;
    }
  }
}

time_t mg_mgr_poll(struct mg_mgr *mgr, int milli) {
  time_t now;
  struct mg_connection *nc, *tmp;
  struct timeval tv;
  fd_set read_set, write_set, err_set;
  sock_t max_fd = INVALID_SOCKET;
  int num_selected;
#ifdef MG_ENABLE_DEBUG
  char buf[32] = {0};
#endif

  FD_ZERO(&read_set);
  FD_ZERO(&write_set);
  FD_ZERO(&err_set);
  mg_add_to_set(mgr->ctl[1], &read_set, &max_fd);

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    tmp = nc->next;

    if (!(nc->flags & MG_F_WANT_WRITE) &&
        nc->recv_mbuf.len < nc->recv_mbuf_limit) {
      mg_add_to_set(nc->sock, &read_set, &max_fd);
    }

    if (((nc->flags & MG_F_CONNECTING) && !(nc->flags & MG_F_WANT_READ)) ||
        (nc->send_mbuf.len > 0 && !(nc->flags & MG_F_CONNECTING) &&
         !(nc->flags & MG_F_DONT_SEND))) {
      mg_add_to_set(nc->sock, &write_set, &max_fd);
      mg_add_to_set(nc->sock, &err_set, &max_fd);
    }
  }

  tv.tv_sec = milli / 1000;
  tv.tv_usec = (milli % 1000) * 1000;

  num_selected = select((int) max_fd + 1, &read_set, &write_set, &err_set, &tv);
  now = time(NULL);
  DBG(("select @ %ld num_ev=%d, errno:%d", (long) now, num_selected, errno));

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    int fd_flags = 0;
    if (num_selected > 0) {
      fd_flags = (FD_ISSET(nc->sock, &read_set) ? _MG_F_FD_CAN_READ : 0) |
                 (FD_ISSET(nc->sock, &write_set) ? _MG_F_FD_CAN_WRITE : 0) |
                 (FD_ISSET(nc->sock, &err_set) ? _MG_F_FD_ERROR : 0);
    }
#ifdef MG_CC3200
    // CC3200 does not report UDP sockets as writeable.
    if (nc->flags & MG_F_UDP &&
        (nc->send_mbuf.len > 0 || nc->flags & MG_F_CONNECTING)) {
      fd_flags |= _MG_F_FD_CAN_WRITE;
    }
#endif
    tmp = nc->next;
    
#ifdef MG_ENABLE_DEBUG
    DBG(("[TVDownloadProxy_LocalProxy]select socket %d(address:%s:%d) status:%d(1:read,2:write,4:error)", nc->sock, ipv4ToStr(&nc->sa.sin.sin_addr, buf, sizeof(buf)-1), (int)ntohs(nc->sa.sin.sin_port), fd_flags));
#endif
      
    if(mg_mgr_handle_connection(nc, fd_flags, now) == _MG_HARD_ERROR)
    {
        mgr->hard_error_flag = 1;
    }
  }

  for (nc = mgr->active_connections; nc != NULL; nc = tmp) {
    tmp = nc->next;
    if ((nc->flags & MG_F_CLOSE_IMMEDIATELY) ||
        (nc->send_mbuf.len == 0 && (nc->flags & MG_F_SEND_AND_CLOSE))) {
      mg_close_conn(nc);
    }
  }

  return now;
}

#endif

/*
 * Schedules an async connect for a resolved address and proto.
 * Called from two places: `mg_connect_opt()` and from async resolver.
 * When called from the async resolver, it must trigger `MG_EV_CONNECT` event
 * with a failure flag to indicate connection failure.
 */
MG_INTERNAL struct mg_connection *mg_finish_connect(struct mg_connection *nc,
                                                    int proto,
                                                    union socket_address *sa,
                                                    struct mg_add_sock_opts o) {
  sock_t sock = INVALID_SOCKET;
  int rc;

#ifdef MG_ENABLE_DEBUG
  char buf[32] = {0};
  DBG(("%p %s://%s:%hu", nc, proto == SOCK_DGRAM ? "udp" : "tcp",
       ipv4ToStr(&nc->sa.sin.sin_addr, buf, sizeof(buf)-1), ntohs(nc->sa.sin.sin_port)));
#endif

  if ((sock = MYSAFE_SOCKET_FOR_SELECT(AF_INET, proto, 0)) == INVALID_SOCKET) {
    int failure = errno;
    MG_SET_PTRPTR(o.error_string, "cannot create socket");
    if (nc->flags & MG_F_CONNECTING) {
      mg_call(nc, MG_EV_CONNECT, &failure);
    }
    mg_destroy_conn(nc);
    return NULL;
  }

#if !defined(MG_CC3200) && !defined(MG_ESP8266)
  mg_set_non_blocking_mode(sock);
#endif
  rc = (proto == SOCK_DGRAM) ? 0 : connect(sock, &sa->sa, sizeof(sa->sin));

  if (rc != 0 && mg_is_error(rc)) {
    MG_SET_PTRPTR(o.error_string, "cannot connect to socket");
    if (nc->flags & MG_F_CONNECTING) {
      mg_call(nc, MG_EV_CONNECT, &rc);
    }
    mg_destroy_conn(nc);
    close(sock);
    return NULL;
  }

  /* Fire MG_EV_CONNECT on next poll. */
  nc->flags |= MG_F_CONNECTING;

  /* No mg_destroy_conn() call after this! */
  mg_set_sock(nc, sock);

  return nc;
}

struct mg_connection *mg_connect(struct mg_mgr *mgr, const char *address,
                                 mg_event_handler_t callback) {
  static struct mg_connect_opts opts;
  return mg_connect_opt(mgr, address, callback, opts);
}

struct mg_connection *mg_connect_opt(struct mg_mgr *mgr, const char *address,
                                     mg_event_handler_t callback,
                                     struct mg_connect_opts opts) {
  struct mg_connection *nc = NULL;
  int proto, rc;
  struct mg_add_sock_opts add_sock_opts;
  char host[MG_MAX_HOST_LEN];

  MG_COPY_COMMON_CONNECTION_OPTIONS(&add_sock_opts, &opts);

  if ((nc = mg_create_connection(mgr, callback, add_sock_opts)) == NULL) {
    return NULL;
  } else if ((rc = mg_parse_address(address, &nc->sa, &proto, host,
                                    sizeof(host))) < 0) {
    /* Address is malformed */
    MG_SET_PTRPTR(opts.error_string, "cannot parse address");
    mg_destroy_conn(nc);
    return NULL;
  }
  nc->flags |= opts.flags & _MG_ALLOWED_CONNECT_FLAGS_MASK;
  nc->flags |= (proto == SOCK_DGRAM) ? MG_F_UDP : 0;
  nc->user_data = opts.user_data;

  if (rc == 0) {
    MG_SET_PTRPTR(opts.error_string, "Resolver is disabled");
    mg_destroy_conn(nc);
    return NULL;
  } else {
    /* Address is parsed and resolved to IP. proceed with connect() */
    return mg_finish_connect(nc, proto, &nc->sa, add_sock_opts);
  }
}

struct mg_connection *mg_bind(struct mg_mgr *srv, const char *address,
                              mg_event_handler_t event_handler) {
  static struct mg_bind_opts opts;
  return mg_bind_opt(srv, address, event_handler, opts);
}

struct mg_connection *mg_bind_opt(struct mg_mgr *mgr, const char *address,
                                  mg_event_handler_t callback,
                                  struct mg_bind_opts opts) {
  union socket_address sa;
  struct mg_connection *nc = NULL;
  int proto;
  sock_t sock;
  struct mg_add_sock_opts add_sock_opts;
  char host[MG_MAX_HOST_LEN];

  MG_COPY_COMMON_CONNECTION_OPTIONS(&add_sock_opts, &opts);

  if (mg_parse_address(address, &sa, &proto, host, sizeof(host)) <= 0) {
    MG_SET_PTRPTR(opts.error_string, "cannot parse address");
  } else if ((sock = mg_open_listening_socket(&sa, proto)) == INVALID_SOCKET) {
    DBG(("Failed to open listener: %d", errno));
    MG_SET_PTRPTR(opts.error_string, "failed to open listener");
  } else if ((nc = mg_add_sock_opt(mgr, sock, callback, add_sock_opts)) ==
             NULL) {
    /* opts.error_string set by mg_add_sock_opt */
    DBG(("Failed to mg_add_sock"));
    closesocket(sock);
  } else {
    nc->sa = sa;
    nc->handler = callback;

    if (proto == SOCK_DGRAM) {
      nc->flags |= MG_F_UDP;
    } else {
      nc->flags |= MG_F_LISTENING;
    }

    DBG(("%p sock %d/%d", nc, sock, proto));
  }

  return nc;
}

struct mg_connection *mg_add_sock(struct mg_mgr *s, sock_t sock,
                                  mg_event_handler_t callback) {
  static struct mg_add_sock_opts opts;
  return mg_add_sock_opt(s, sock, callback, opts);
}

struct mg_connection *mg_add_sock_opt(struct mg_mgr *s, sock_t sock,
                                      mg_event_handler_t callback,
                                      struct mg_add_sock_opts opts) {
  struct mg_connection *nc = mg_create_connection(s, callback, opts);
  if (nc != NULL) {
    mg_set_sock(nc, sock);
  }
  return nc;
}

struct mg_connection *mg_next(struct mg_mgr *s, struct mg_connection *conn) {
  return conn == NULL ? s->active_connections : conn->next;
}

void mg_broadcast(struct mg_mgr *mgr, mg_event_handler_t cb, void *data,
                  size_t len) {
  struct ctl_msg ctl_msg;

  /*
   * Mongoose manager has a socketpair, `struct mg_mgr::ctl`,
   * where `mg_broadcast()` pushes the message.
   * `mg_mgr_poll()` wakes up, reads a message from the socket pair, and calls
   * specified callback for each connection. Thus the callback function executes
   * in event manager thread.
   */
  if (mgr->ctl[0] != INVALID_SOCKET && data != NULL &&
      len < sizeof(ctl_msg.message)) {
    ctl_msg.callback = cb;
    memcpy(ctl_msg.message, data, len);
    MG_EV_SEND_FUNC(mgr->ctl[0], (char *) &ctl_msg,
                    offsetof(struct ctl_msg, message) + len, 0);
    MG_EV_RECV_FUNC(mgr->ctl[0], (char *) &len, 1, 0);
  }
}

static int isbyte(int n) {
  return n >= 0 && n <= 255;
}

static int parse_net(const char *spec, uint32_t *net, uint32_t *mask) {
  int n, a, b, c, d, slash = 32, len = 0;

  if ((sscanf(spec, "%d.%d.%d.%d/%d%n", &a, &b, &c, &d, &slash, &n) == 5 ||
       sscanf(spec, "%d.%d.%d.%d%n", &a, &b, &c, &d, &n) == 4) &&
      isbyte(a) && isbyte(b) && isbyte(c) && isbyte(d) && slash >= 0 &&
      slash < 33) {
    len = n;
    *net =
        ((uint32_t) a << 24) | ((uint32_t) b << 16) | ((uint32_t) c << 8) | d;
    *mask = slash ? 0xffffffffU << (32 - slash) : 0;
  }

  return len;
}

int mg_check_ip_acl(const char *acl, uint32_t remote_ip) {
  int allowed, flag;
  uint32_t net, mask;
  struct mg_str vec;

  /* If any ACL is set, deny by default */
  allowed = (acl == NULL || *acl == '\0') ? '+' : '-';

  while ((acl = mg_next_comma_list_entry(acl, &vec, NULL)) != NULL) {
    flag = vec.p[0];
    if ((flag != '+' && flag != '-') ||
        parse_net(&vec.p[1], &net, &mask) == 0) {
      return -1;
    }

    if (net == (remote_ip & mask)) {
      allowed = flag;
    }
  }

  return allowed == '+';
}

/* Move data from one connection to another */
void mg_forward(struct mg_connection *from, struct mg_connection *to) {
  mg_send(to, from->recv_mbuf.buf, (int)from->recv_mbuf.len);
  mbuf_remove(&from->recv_mbuf, from->recv_mbuf.len);
}
#ifdef NS_MODULE_LINES
#line 1 "src/multithreading.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "internal.h" */

#ifdef NS_MODULE_LINES
#line 1 "src/http.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MG_DISABLE_HTTP

/* Amalgamated: #include "internal.h" */

enum http_proto_data_type { DATA_NONE, DATA_FILE, DATA_PUT, DATA_CGI };

struct proto_data_http {
  FILE *fp;         /* Opened file. */
  int64_t cl;       /* Content-Length. How many bytes to send. */
  int64_t sent;     /* How many bytes have been already sent. */
  int64_t body_len; /* How many bytes of chunked body was reassembled. */
  struct mg_connection *cgi_nc;
  enum http_proto_data_type type;
};

/*
 * This structure helps to create an environment for the spawned CGI program.
 * Environment is an array of "VARIABLE=VALUE\0" ASCIIZ strings,
 * last element must be NULL.
 * However, on Windows there is a requirement that all these VARIABLE=VALUE\0
 * strings must reside in a contiguous buffer. The end of the buffer is
 * marked by two '\0' characters.
 * We satisfy both worlds: we create an envp array (which is vars), all
 * entries are actually pointers inside buf.
 */
struct cgi_env_block {
  struct mg_connection *nc;
  char buf[MG_CGI_ENVIRONMENT_SIZE];       /* Environment buffer */
  const char *vars[MG_MAX_CGI_ENVIR_VARS]; /* char *envp[] */
  int len;                                 /* Space taken */
  int nvars;                               /* Number of variables in envp[] */
};

#if 0
#define MIME_ENTRY(_ext, _type) \
  { _ext, sizeof(_ext) - 1, _type }
static const struct {
  const char *extension;
  size_t ext_len;
  const char *mime_type;
}
static_builtin_mime_types[] = {
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("htm", "text/html"),
    MIME_ENTRY("shtm", "text/html"),
    MIME_ENTRY("shtml", "text/html"),
    MIME_ENTRY("css", "text/css"),
    MIME_ENTRY("js", "application/x-javascript"),
    MIME_ENTRY("ico", "image/x-icon"),
    MIME_ENTRY("gif", "image/gif"),
    MIME_ENTRY("jpg", "image/jpeg"),
    MIME_ENTRY("jpeg", "image/jpeg"),
    MIME_ENTRY("png", "image/png"),
    MIME_ENTRY("svg", "image/svg+xml"),
    MIME_ENTRY("txt", "text/plain"),
    MIME_ENTRY("torrent", "application/x-bittorrent"),
    MIME_ENTRY("wav", "audio/x-wav"),
    MIME_ENTRY("mp3", "audio/x-mp3"),
    MIME_ENTRY("mid", "audio/mid"),
    MIME_ENTRY("m3u", "audio/x-mpegurl"),
    MIME_ENTRY("ogg", "application/ogg"),
    MIME_ENTRY("ram", "audio/x-pn-realaudio"),
    MIME_ENTRY("xml", "text/xml"),
    MIME_ENTRY("ttf", "application/x-font-ttf"),
    MIME_ENTRY("json", "application/json"),
    MIME_ENTRY("xslt", "application/xml"),
    MIME_ENTRY("xsl", "application/xml"),
    MIME_ENTRY("ra", "audio/x-pn-realaudio"),
    MIME_ENTRY("doc", "application/msword"),
    MIME_ENTRY("exe", "application/octet-stream"),
    MIME_ENTRY("zip", "application/x-zip-compressed"),
    MIME_ENTRY("xls", "application/excel"),
    MIME_ENTRY("tgz", "application/x-tar-gz"),
    MIME_ENTRY("tar", "application/x-tar"),
    MIME_ENTRY("gz", "application/x-gunzip"),
    MIME_ENTRY("arj", "application/x-arj-compressed"),
    MIME_ENTRY("rar", "application/x-rar-compressed"),
    MIME_ENTRY("rtf", "application/rtf"),
    MIME_ENTRY("pdf", "application/pdf"),
    MIME_ENTRY("swf", "application/x-shockwave-flash"),
    MIME_ENTRY("mpg", "video/mpeg"),
    MIME_ENTRY("webm", "video/webm"),
    MIME_ENTRY("mpeg", "video/mpeg"),
    MIME_ENTRY("mov", "video/quicktime"),
    MIME_ENTRY("mp4", "video/mp4"),
    MIME_ENTRY("m4v", "video/x-m4v"),
    MIME_ENTRY("asf", "video/x-ms-asf"),
    MIME_ENTRY("avi", "video/x-msvideo"),
    MIME_ENTRY("bmp", "image/bmp"),
    {NULL, 0, NULL}};
#endif

#ifndef MG_DISABLE_FILESYSTEM

static int mg_mkdir(const char *path, uint32_t mode) {
#ifndef _WIN32
  return mkdir(path, mode);
#else
  (void) mode;
  return _mkdir(path);
#endif
}

static struct mg_str get_mime_type(const char *path, const char *dflt,
                                   const struct mg_serve_http_opts *opts) {
  const char *ext, *overrides;
  size_t i, path_len;
  struct mg_str r, k, v;

  path_len = strlen(path);

  overrides = opts->custom_mime_types;
  while ((overrides = mg_next_comma_list_entry(overrides, &k, &v)) != NULL) {
    ext = path + (path_len - k.len);
    if (path_len > k.len && mg_vcasecmp(&k, ext) == 0) {
      return v;
    }
  }

  for (i = 0; static_builtin_mime_types[i].extension != NULL; i++) {
    ext = path + (path_len - static_builtin_mime_types[i].ext_len);
    if (path_len > static_builtin_mime_types[i].ext_len && ext[-1] == '.' &&
        mg_casecmp(ext, static_builtin_mime_types[i].extension) == 0) {
      r.p = static_builtin_mime_types[i].mime_type;
      r.len = strlen(r.p);
      return r;
    }
  }

  r.p = dflt;
  r.len = strlen(r.p);
  return r;
}
#endif

/*
 * Check whether full request is buffered. Return:
 *   -1  if request is malformed
 *    0  if request is not yet fully buffered
 *   >0  actual request length, including last \r\n\r\n
 */
static int get_request_len(const char *s, int buf_len) {
  const unsigned char *buf = (unsigned char *) s;
  int i;

  for (i = 0; i < buf_len; i++) {
    if (!isprint(buf[i]) && buf[i] != '\r' && buf[i] != '\n' && buf[i] < 128) {
      return -1;
    } else if (buf[i] == '\n' && i + 1 < buf_len && buf[i + 1] == '\n') {
      return i + 2;
    } else if (buf[i] == '\n' && i + 2 < buf_len && buf[i + 1] == '\r' &&
               buf[i + 2] == '\n') {
      return i + 3;
    }
  }

  return 0;
}

static const char *parse_http_headers(const char *s, const char *end, int len,
                                      struct http_message *req) {
  int i;
  for (i = 0; i < (int) ARRAY_SIZE(req->header_names); i++) {
    struct mg_str *k = &req->header_names[i], *v = &req->header_values[i];

    s = mg_skip(s, end, ": ", k);
    s = mg_skip(s, end, "\r\n", v);

    while (v->len > 0 && v->p[v->len - 1] == ' ') {
      v->len--; /* Trim trailing spaces in header value */
    }

//    if (k->len == 0 || v->len == 0) {
    if (k->len == 0) {
      k->p = v->p = NULL;
      k->len = v->len = 0;
      break;
    }

    if (!mg_ncasecmp(k->p, "Content-Length", 14)) {
      req->body.len = (size_t)to64(v->p);
      req->message.len = len + req->body.len;
    }
  }

  return s;
}

int mg_parse_http(const char *s, int n, struct http_message *hm, int is_req) {
  const char *end, *qs;
  int len = get_request_len(s, n);

  if (len <= 0) return len;

  memset(hm, 0, sizeof(*hm));
  hm->message.p = s;
  hm->body.p = s + len;
  hm->message.len = hm->body.len = (size_t) ~0;
  end = s + len;

  /* Request is fully buffered. Skip leading whitespaces. */
  while (s < end && isspace(*(unsigned char *) s)) s++;

  if (is_req) {
    /* Parse request line: method, URI, proto */
    s = mg_skip(s, end, " ", &hm->method);
    s = mg_skip(s, end, " ", &hm->uri);
    s = mg_skip(s, end, "\r\n", &hm->proto);
    if (hm->uri.p <= hm->method.p || hm->proto.p <= hm->uri.p) return -1;

    /* If URI contains '?' character, initialize query_string */
    if ((qs = (char *) memchr(hm->uri.p, '?', hm->uri.len)) != NULL) {
      hm->query_string.p = qs + 1;
      hm->query_string.len = &hm->uri.p[hm->uri.len] - (qs + 1);
      hm->uri.len = qs - hm->uri.p;
    }
  } else {
    s = mg_skip(s, end, " ", &hm->proto);
    if (end - s < 4 || s[3] != ' ') return -1;
    hm->resp_code = atoi(s);
    if (hm->resp_code < 100 || hm->resp_code >= 600) return -1;
    s += 4;
    s = mg_skip(s, end, "\r\n", &hm->resp_status_msg);
  }

  s = parse_http_headers(s, end, len, hm);

  /*
   * mg_parse_http() is used to parse both HTTP requests and HTTP
   * responses. If HTTP response does not have Content-Length set, then
   * body is read until socket is closed, i.e. body.len is infinite (~0).
   *
   * For HTTP requests though, according to
   * http://tools.ietf.org/html/rfc7231#section-8.1.3,
   * only POST and PUT methods have defined body semantics.
   * Therefore, if Content-Length is not specified and methods are
   * not one of PUT or POST, set body length to 0.
   *
   * So,
   * if it is HTTP request, and Content-Length is not set,
   * and method is not (PUT or POST) then reset body length to zero.
   */
  if (hm->body.len == (size_t) ~0 && is_req &&
      mg_vcasecmp(&hm->method, "PUT") != 0 &&
      mg_vcasecmp(&hm->method, "POST") != 0) {
    hm->body.len = 0;
    hm->message.len = len;
  }

  return len;
}

struct mg_str *mg_get_http_header(struct http_message *hm, const char *name) {
  size_t i, len = strlen(name);

  for (i = 0; i < ARRAY_SIZE(hm->header_names); i++) {
    struct mg_str *h = &hm->header_names[i], *v = &hm->header_values[i];
    if (h->p != NULL && h->len == len && !mg_ncasecmp(h->p, name, len))
      return v;
  }

  return NULL;
}

static void free_http_proto_data(struct mg_connection *nc) {
  struct proto_data_http *dp = (struct proto_data_http *) nc->proto_data;
  if (dp != NULL) {
    if (dp->fp != NULL) {
      fclose(dp->fp);
    }
    if (dp->cgi_nc != NULL) {
      dp->cgi_nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    }
    MG_FREE(dp);
    nc->proto_data = NULL;
  }
}

static void transfer_file_data(struct mg_connection *nc) {
  struct proto_data_http *dp = (struct proto_data_http *) nc->proto_data;
  char buf[MG_MAX_HTTP_SEND_IOBUF];
  int64_t left = dp->cl - dp->sent;
  size_t n = 0, to_read = 0;

  if (dp->type == DATA_FILE) {
    struct mbuf *io = &nc->send_mbuf;
    if (io->len < sizeof(buf)) {
      to_read = sizeof(buf) - io->len;
    }

    if (left > 0 && to_read > (size_t) left) {
      to_read = (size_t)left;
    }

    if (to_read == 0) {
      /* Rate limiting. send_mbuf is too full, wait until it's drained. */
    } else if (dp->sent < dp->cl && (n = fread(buf, 1, to_read, dp->fp)) > 0) {
      mg_send(nc, buf, (int)n);
      dp->sent += n;
    } else {
      free_http_proto_data(nc);
    }
  } else if (dp->type == DATA_PUT) {
    struct mbuf *io = &nc->recv_mbuf;
    size_t to_write =
        left <= 0 ? 0 : left < (int64_t) io->len ? (size_t) left : io->len;
    size_t n = fwrite(io->buf, 1, to_write, dp->fp);
    if (n > 0) {
      mbuf_remove(io, n);
      dp->sent += n;
    }
    if (n == 0 || dp->sent >= dp->cl) {
      free_http_proto_data(nc);
    }
  } else if (dp->type == DATA_CGI) {
    /* This is POST data that needs to be forwarded to the CGI process */
    if (dp->cgi_nc != NULL) {
      mg_forward(nc, dp->cgi_nc);
    } else {
      nc->flags |= MG_F_SEND_AND_CLOSE;
    }
  }
}

/*
 * Parse chunked-encoded buffer. Return 0 if the buffer is not encoded, or
 * if it's incomplete. If the chunk is fully buffered, return total number of
 * bytes in a chunk, and store data in `data`, `data_len`.
 */
static size_t parse_chunk(char *buf, size_t len, char **chunk_data,
                          size_t *chunk_len) {
  unsigned char *s = (unsigned char *) buf;
  size_t n = 0; /* scanned chunk length */
  size_t i = 0; /* index in s */

  /* Scan chunk length. That should be a hexadecimal number. */
  while (i < len && isxdigit(s[i])) {
    n *= 16;
    n += (s[i] >= '0' && s[i] <= '9') ? s[i] - '0' : tolower(s[i]) - 'a' + 10;
    i++;
  }

  /* Skip new line */
  if (i == 0 || i + 2 > len || s[i] != '\r' || s[i + 1] != '\n') {
    return 0;
  }
  i += 2;

  /* Record where the data is */
  *chunk_data = (char *) s + i;
  *chunk_len = n;

  /* Skip data */
  i += n;

  /* Skip new line */
  if (i == 0 || i + 2 > len || s[i] != '\r' || s[i + 1] != '\n') {
    return 0;
  }
  return i + 2;
}

MG_INTERNAL size_t mg_handle_chunked(struct mg_connection *nc,
                                     struct http_message *hm, char *buf,
                                     size_t blen) {
  struct proto_data_http *dp;
  char *data;
  size_t i, n, data_len, body_len, zero_chunk_received = 0;

  /* If not allocated, allocate proto_data to hold reassembled offset */
  if (nc->proto_data == NULL &&
      (nc->proto_data = MG_CALLOC(1, sizeof(*dp))) == NULL) {
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    return 0;
  }

  /* Find out piece of received data that is not yet reassembled */
  dp = (struct proto_data_http *) nc->proto_data;
  body_len = (size_t)dp->body_len;
  assert(blen >= body_len);

  /* Traverse all fully buffered chunks */
  for (i = body_len; (n = parse_chunk(buf + i, blen - i, &data, &data_len)) > 0;
       i += n) {
    /* Collapse chunk data to the rest of HTTP body */
    memmove(buf + body_len, data, data_len);
    body_len += data_len;
    hm->body.len = body_len;

    if (data_len == 0) {
      zero_chunk_received = 1;
      i += n;
      break;
    }
  }

  if (i > body_len) {
    /* Shift unparsed content to the parsed body */
    assert(i <= blen);
    memmove(buf + body_len, buf + i, blen - i);
    memset(buf + body_len + blen - i, 0, i - body_len);
    nc->recv_mbuf.len -= i - body_len;
    dp->body_len = body_len;

    /* Send MG_EV_HTTP_CHUNK event */
    nc->flags &= ~MG_F_DELETE_CHUNK;
    nc->handler(nc, MG_EV_HTTP_CHUNK, hm);

    /* Delete processed data if user set MG_F_DELETE_CHUNK flag */
    if (nc->flags & MG_F_DELETE_CHUNK) {
      memset(buf, 0, body_len);
      memmove(buf, buf + body_len, blen - i);
      nc->recv_mbuf.len -= body_len;
      hm->body.len = dp->body_len = 0;
    }

    if (zero_chunk_received) {
      hm->message.len = (size_t)(dp->body_len + blen - i);
    }
  }

  return body_len;
}

static void http_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct mbuf *io = &nc->recv_mbuf;
  struct http_message hm;
  int req_len;
  const int is_req = (nc->listener != NULL);
  /*
   * For HTTP messages without Content-Length, always send HTTP message
   * before MG_EV_CLOSE message.
   */
  if (ev == MG_EV_CLOSE && io->len > 0 &&
      mg_parse_http(io->buf, (int)io->len, &hm, is_req) > 0) {
    hm.message.len = io->len;
    hm.body.len = io->buf + io->len - hm.body.p;
    nc->handler(nc, is_req ? MG_EV_HTTP_REQUEST : MG_EV_HTTP_REPLY, &hm);
    free_http_proto_data(nc);
  }

  if (nc->proto_data != NULL) {
    transfer_file_data(nc);
  }

  nc->handler(nc, ev, ev_data);

  if (ev == MG_EV_RECV) {
    struct mg_str *s;
    req_len = mg_parse_http(io->buf, (int)io->len, &hm, is_req);

    if (req_len > 0 &&
        (s = mg_get_http_header(&hm, "Transfer-Encoding")) != NULL &&
        mg_vcasecmp(s, "chunked") == 0) {
      mg_handle_chunked(nc, &hm, io->buf + req_len, io->len - req_len);
    }

    if (req_len < 0 || (req_len == 0 && io->len >= MG_MAX_HTTP_REQUEST_SIZE)) {
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
    } else if (req_len == 0) {
      /* Do nothing, request is not yet fully buffered */
    }
    else if (hm.message.len <= io->len) {
      /* Whole HTTP message is fully buffered, call event handler */
      nc->handler(nc, nc->listener ? MG_EV_HTTP_REQUEST : MG_EV_HTTP_REPLY,
                  &hm);
      mbuf_remove(io, hm.message.len);
    }
  }
}

void mg_set_protocol_http_websocket(struct mg_connection *nc) {
  nc->proto_handler = http_handler;
}

#ifndef MG_DISABLE_FILESYSTEM
static void send_http_error(struct mg_connection *nc, int code,
                            const char *reason) {
  if (reason == NULL) {
    reason = "";
  }
  mg_printf(nc, "HTTP/1.1 %d %s\r\nContent-Length: 0\r\n\r\n", code, reason);
}

static void handle_ssi_request(struct mg_connection *nc, const char *path,
                               const struct mg_serve_http_opts *opts) {
  (void) path;
  (void) opts;
  send_http_error(nc, 500, "SSI disabled");
}

static void construct_etag(char *buf, size_t buf_len, const cs_stat_t *st) {
  snprintf(buf, buf_len, "\"%lx.%" INT64_FMT "\"", (unsigned long) st->st_mtime,
           (int64_t) st->st_size);
}
static void gmt_time_string(char *buf, size_t buf_len, time_t *t) {
  strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S GMT", gmtime(t));
}

static int parse_range_header(const struct mg_str *header, int64_t *a,
                              int64_t *b) {
  /*
   * There is no snscanf. Headers are not guaranteed to be NUL-terminated,
   * so we have this. Ugh.
   */
  int result;
  char *p = (char *) MG_MALLOC(header->len + 1);
  if (p == NULL) return 0;
  memcpy(p, header->p, header->len);
  p[header->len] = '\0';
  result = sscanf(p, "bytes=%" INT64_FMT "-%" INT64_FMT, a, b);
  MG_FREE(p);
  return result;
}

static void mg_send_http_file2(struct mg_connection *nc, const char *path,
                               cs_stat_t *st, struct http_message *hm,
                               struct mg_serve_http_opts *opts) {
  struct proto_data_http *dp;
  struct mg_str mime_type;

  free_http_proto_data(nc);
  if ((dp = (struct proto_data_http *) MG_CALLOC(1, sizeof(*dp))) == NULL) {
    send_http_error(nc, 500, "Server Error"); /* LCOV_EXCL_LINE */
  } else if ((dp->fp = fopen(path, "rb")) == NULL) {
    MG_FREE(dp);
    nc->proto_data = NULL;
    send_http_error(nc, 500, "Server Error");
  } else if (mg_match_prefix(opts->ssi_pattern, strlen(opts->ssi_pattern),
                             path) > 0) {
    handle_ssi_request(nc, path, opts);
  } else {
    char etag[50], current_time[50], last_modified[50], range[50];
    time_t t = time(NULL);
    int64_t r1 = 0, r2 = 0, cl = st->st_size;
    struct mg_str *range_hdr = mg_get_http_header(hm, "Range");
    int n, status_code = 200;
    const char *status_message = "OK";

    /* Handle Range header */
    range[0] = '\0';
    if (range_hdr != NULL &&
        (n = parse_range_header(range_hdr, &r1, &r2)) > 0 && r1 >= 0 &&
        r2 >= 0) {
      /* If range is specified like "400-", set second limit to content len */
      if (n == 1) {
        r2 = cl - 1;
      }
      if (r1 > r2 || r2 >= cl) {
        status_code = 416;
        status_message = "Requested range not satisfiable";
        cl = 0;
        snprintf(range, sizeof(range),
                 "Content-Range: bytes */%" INT64_FMT "\r\n",
                 (int64_t) st->st_size);
      } else {
        status_code = 206;
        status_message = "Partial Content";
        cl = r2 - r1 + 1;
        snprintf(range, sizeof(range), "Content-Range: bytes %" INT64_FMT
                                       "-%" INT64_FMT "/%" INT64_FMT "\r\n",
                 r1, r1 + cl - 1, (int64_t) st->st_size);
        fseeko(dp->fp, r1, SEEK_SET);
      }
    }

    construct_etag(etag, sizeof(etag), st);
    gmt_time_string(current_time, sizeof(current_time), &t);
    gmt_time_string(last_modified, sizeof(last_modified), &st->st_mtime);
    mime_type = get_mime_type(path, "text/plain", opts);
    mg_printf(nc,
              "HTTP/1.1 %d %s\r\n"
              "Date: %s\r\n"
              "Last-Modified: %s\r\n"
              "Accept-Ranges: bytes\r\n"
              "Content-Type: %.*s\r\n"
              "Content-Length: %" INT64_FMT
              "\r\n"
              "%s"
              "Etag: %s\r\n"
              "\r\n",
              status_code, status_message, current_time, last_modified,
              (int) mime_type.len, mime_type.p, cl, range, etag);
    nc->proto_data = (void *) dp;
    dp->cl = cl;
    dp->type = DATA_FILE;
    transfer_file_data(nc);
  }
}

static void remove_double_dots(char *s) {
  char *p = s;

  while (*s != '\0') {
    *p++ = *s++;
    if (s[-1] == '/' || s[-1] == '\\') {
      while (s[0] != '\0') {
        if (s[0] == '/' || s[0] == '\\') {
          s++;
        } else if (s[0] == '.' && s[1] == '.') {
          s += 2;
        } else {
          break;
        }
      }
    }
  }
  *p = '\0';
}

#endif

static int mg_url_decode(const char *src, int src_len, char *dst, int dst_len,
                         int is_form_url_encoded) {
  int i, j, a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

  for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
    if (src[i] == '%') {
      if (i < src_len - 2 && isxdigit(*(const unsigned char *) (src + i + 1)) &&
          isxdigit(*(const unsigned char *) (src + i + 2))) {
        a = tolower(*(const unsigned char *) (src + i + 1));
        b = tolower(*(const unsigned char *) (src + i + 2));
        dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
        i += 2;
      } else {
        return -1;
      }
    } else if (is_form_url_encoded && src[i] == '+') {
      dst[j] = ' ';
    } else {
      dst[j] = src[i];
    }
  }

  dst[j] = '\0'; /* Null-terminate the destination */

  return i >= src_len ? j : -1;
}

int mg_get_http_var(const struct mg_str *buf, const char *name, char *dst,
                    size_t dst_len) {
  const char *p, *e, *s;
  size_t name_len;
  int len;

  if (dst == NULL || dst_len == 0) {
    len = -2;
  } else if (buf->p == NULL || name == NULL || buf->len == 0) {
    len = -1;
    dst[0] = '\0';
  } else {
    name_len = strlen(name);
    e = buf->p + buf->len;
    len = -1;
    dst[0] = '\0';

    for (p = buf->p; p + name_len < e; p++) {
      if ((p == buf->p || p[-1] == '&') && p[name_len] == '=' &&
          !mg_ncasecmp(name, p, name_len)) {
        p += name_len + 1;
        s = (const char *) memchr(p, '&', (size_t)(e - p));
        if (s == NULL) {
          s = e;
        }
        len = mg_url_decode(p, (int)(size_t)(s - p), dst, (int)dst_len, 1);
        if (len == -1) {
          len = -2;
        }
        break;
      }
    }
  }

  return len;
}

void mg_send_http_chunk(struct mg_connection *nc, const char *buf, size_t len) {
  char chunk_size[50];
  int n;

  n = snprintf(chunk_size, sizeof(chunk_size), "%lX\r\n", (unsigned long) len);
  mg_send(nc, chunk_size, n);
  mg_send(nc, buf, (int)len);
  mg_send(nc, "\r\n", 2);
}

void mg_printf_http_chunk(struct mg_connection *nc, const char *fmt, ...) {
  char mem[500], *buf = mem;
  int len;
  va_list ap;

  va_start(ap, fmt);
  len = mg_avprintf(&buf, sizeof(mem), fmt, ap);
  va_end(ap);

  if (len >= 0) {
    mg_send_http_chunk(nc, buf, len);
  }

  /* LCOV_EXCL_START */
  if (buf != mem && buf != NULL) {
    MG_FREE(buf);
  }
  /* LCOV_EXCL_STOP */
}

void mg_printf_html_escape(struct mg_connection *nc, const char *fmt, ...) {
  char mem[500], *buf = mem;
  int i, j, len;
  va_list ap;

  va_start(ap, fmt);
  len = mg_avprintf(&buf, sizeof(mem), fmt, ap);
  va_end(ap);

  if (len >= 0) {
    for (i = j = 0; i < len; i++) {
      if (buf[i] == '<' || buf[i] == '>') {
        mg_send(nc, buf + j, i - j);
        mg_send(nc, buf[i] == '<' ? "&lt;" : "&gt;", 4);
        j = i + 1;
      }
    }
    mg_send(nc, buf + j, i - j);
  }

  /* LCOV_EXCL_START */
  if (buf != mem && buf != NULL) {
    MG_FREE(buf);
  }
  /* LCOV_EXCL_STOP */
}

int mg_http_parse_header(struct mg_str *hdr, const char *var_name, char *buf,
                         size_t buf_size) {
  int ch = ' ', ch1 = ',', len = 0, n = (int)strlen(var_name);
  const char *p, *end = hdr->p + hdr->len, *s = NULL;

  if (buf != NULL && buf_size > 0) buf[0] = '\0';

  /* Find where variable starts */
  for (s = hdr->p; s != NULL && s + n < end; s++) {
    if ((s == hdr->p || s[-1] == ch || s[-1] == ch1) && s[n] == '=' &&
        !memcmp(s, var_name, n))
      break;
  }

  if (s != NULL && &s[n + 1] < end) {
    s += n + 1;
    if (*s == '"' || *s == '\'') {
      ch = ch1 = *s++;
    }
    p = s;
    while (p < end && p[0] != ch && p[0] != ch1 && len < (int) buf_size) {
      if (ch != ' ' && p[0] == '\\' && p[1] == ch) p++;
      buf[len++] = *p++;
    }
    if (len >= (int) buf_size || (ch != ' ' && *p != ch)) {
      len = 0;
    } else {
      if (len > 0 && s[len - 1] == ',') len--;
      if (len > 0 && s[len - 1] == ';') len--;
      buf[len] = '\0';
    }
  }

  return len;
}

#ifndef MG_DISABLE_FILESYSTEM
static int is_file_hidden(const char *path,
                          const struct mg_serve_http_opts *opts) {
  const char *p1 = opts->per_directory_auth_file;
  const char *p2 = opts->hidden_file_pattern;
  return !strcmp(path, ".") || !strcmp(path, "..") ||
         (p1 != NULL && !strcmp(path, p1)) ||
         (p2 != NULL && mg_match_prefix(p2, strlen(p2), path) > 0);
}

static int is_authorized(struct http_message *hm, const char *path,
                         int is_directory, struct mg_serve_http_opts *opts) {
  (void) hm;
  (void) path;
  (void) is_directory;
  (void) opts;
  return 1;
}

#ifndef MG_DISABLE_DIRECTORY_LISTING
static size_t mg_url_encode(const char *src, size_t s_len, char *dst,
                            size_t dst_len) {
  static const char *dont_escape = "._-$,;~()";
  static const char *hex = "0123456789abcdef";
  size_t i = 0, j = 0;

  for (i = j = 0; dst_len > 0 && i < s_len && j + 2 < dst_len - 1; i++, j++) {
    if (isalnum(*(const unsigned char *) (src + i)) ||
        strchr(dont_escape, *(const unsigned char *) (src + i)) != NULL) {
      dst[j] = src[i];
    } else if (j + 3 < dst_len) {
      dst[j] = '%';
      dst[j + 1] = hex[(*(const unsigned char *) (src + i)) >> 4];
      dst[j + 2] = hex[(*(const unsigned char *) (src + i)) & 0xf];
      j += 2;
    }
  }

  dst[j] = '\0';
  return j;
}

static void escape(const char *src, char *dst, size_t dst_len) {
  size_t n = 0;
  while (*src != '\0' && n + 5 < dst_len) {
    unsigned char ch = *(unsigned char *) src++;
    if (ch == '<') {
      n += snprintf(dst + n, dst_len - n, "%s", "&lt;");
    } else {
      dst[n++] = ch;
    }
  }
  dst[n] = '\0';
}

static void print_dir_entry(struct mg_connection *nc, const char *file_name,
                            cs_stat_t *stp) {
  char size[64], mod[64], href[MAX_PATH_SIZE * 3], path[MAX_PATH_SIZE];
  int64_t fsize = stp->st_size;
  int is_dir = S_ISDIR(stp->st_mode);
  const char *slash = is_dir ? "/" : "";

  if (is_dir) {
    snprintf(size, sizeof(size), "%s", "[DIRECTORY]");
  } else {
    /*
     * We use (double) cast below because MSVC 6 compiler cannot
     * convert unsigned __int64 to double.
     */
    if (fsize < 1024) {
      snprintf(size, sizeof(size), "%d", (int) fsize);
    } else if (fsize < 0x100000) {
      snprintf(size, sizeof(size), "%.1fk", (double) fsize / 1024.0);
    } else if (fsize < 0x40000000) {
      snprintf(size, sizeof(size), "%.1fM", (double) fsize / 1048576);
    } else {
      snprintf(size, sizeof(size), "%.1fG", (double) fsize / 1073741824);
    }
  }
  strftime(mod, sizeof(mod), "%d-%b-%Y %H:%M", localtime(&stp->st_mtime));
  escape(file_name, path, sizeof(path));
  mg_url_encode(file_name, strlen(file_name), href, sizeof(href));
  mg_printf_http_chunk(nc,
                       "<tr><td><a href=\"%s%s\">%s%s</a></td>"
                       "<td>%s</td><td name=%" INT64_FMT ">%s</td></tr>\n",
                       href, slash, path, slash, mod, is_dir ? -1 : fsize,
                       size);
}

static void scan_directory(struct mg_connection *nc, const char *dir,
                           const struct mg_serve_http_opts *opts,
                           void (*func)(struct mg_connection *, const char *,
                                        cs_stat_t *)) {
  char path[MAX_PATH_SIZE];
  cs_stat_t st;
  struct dirent *dp;
  DIR *dirp;

  if ((dirp = (opendir(dir))) != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      /* Do not show current dir and hidden files */
      if (is_file_hidden(dp->d_name, opts)) {
        continue;
      }
      snprintf(path, sizeof(path), "%s/%s", dir, dp->d_name);
      if (mg_stat(path, &st) == 0) {
        func(nc, dp->d_name, &st);
      }
    }
    closedir(dirp);
  }
}

static void send_directory_listing(struct mg_connection *nc, const char *dir,
                                   struct http_message *hm,
                                   struct mg_serve_http_opts *opts) {
  static const char *sort_js_code =
      "<script>function srt(tb, col) {"
      "var tr = Array.prototype.slice.call(tb.rows, 0),"
      "tr = tr.sort(function (a, b) { var c1 = a.cells[col], c2 = b.cells[col],"
      "n1 = c1.getAttribute('name'), n2 = c2.getAttribute('name'), "
      "t1 = a.cells[2].getAttribute('name'), "
      "t2 = b.cells[2].getAttribute('name'); "
      "return t1 < 0 && t2 >= 0 ? -1 : t2 < 0 && t1 >= 0 ? 1 : "
      "n1 ? parseInt(n2) - parseInt(n1) : "
      "c1.textContent.trim().localeCompare(c2.textContent.trim()); });";
  static const char *sort_js_code2 =
      "for (var i = 0; i < tr.length; i++) tb.appendChild(tr[i]);}"
      "window.onload = function() { "
      "var tb = document.getElementById('tb');"
      "document.onclick = function(ev){ "
      "var c = ev.target.rel; if (c) srt(tb, c)}; srt(tb, 2); };</script>";

  mg_printf(nc, "%s\r\n%s: %s\r\n%s: %s\r\n\r\n", "HTTP/1.1 200 OK",
            "Transfer-Encoding", "chunked", "Content-Type",
            "text/html; charset=utf-8");

  mg_printf_http_chunk(
      nc,
      "<html><head><title>Index of %.*s</title>%s%s"
      "<style>th,td {text-align: left; padding-right: 1em; }</style></head>"
      "<body><h1>Index of %.*s</h1><pre><table cellpadding=\"0\"><thead>"
      "<tr><th><a href=# rel=0>Name</a></th><th>"
      "<a href=# rel=1>Modified</a</th>"
      "<th><a href=# rel=2>Size</a></th></tr>"
      "<tr><td colspan=\"3\"><hr></td></tr></thead><tbody id=tb>",
      (int) hm->uri.len, hm->uri.p, sort_js_code, sort_js_code2,
      (int) hm->uri.len, hm->uri.p);
  scan_directory(nc, dir, opts, print_dir_entry);
  mg_printf_http_chunk(nc, "%s", "</tbody></body></html>");
  mg_send_http_chunk(nc, "", 0);
  /* TODO(rojer): Remove when cesanta/dev/issues/197 is fixed. */
  nc->flags |= MG_F_SEND_AND_CLOSE;
}
#endif /* MG_DISABLE_DIRECTORY_LISTING */

static int is_dav_request(const struct mg_str *s) {
  return !mg_vcmp(s, "PUT") || !mg_vcmp(s, "DELETE") || !mg_vcmp(s, "MKCOL") ||
         !mg_vcmp(s, "PROPFIND");
}

/*
 * Given a directory path, find one of the files specified in the
 * comma-separated list of index files `list`.
 * First found index file wins. If an index file is found, then gets
 * appended to the `path`, stat-ed, and result of `stat()` passed to `stp`.
 * If index file is not found, then `path` and `stp` remain unchanged.
 */
MG_INTERNAL int find_index_file(char *path, size_t path_len, const char *list,
                                cs_stat_t *stp) {
  cs_stat_t st;
  size_t n = strlen(path);
  struct mg_str vec;
  int found = 0;

  /* The 'path' given to us points to the directory. Remove all trailing */
  /* directory separator characters from the end of the path, and */
  /* then append single directory separator character. */
  while (n > 0 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
    n--;
  }

  /* Traverse index files list. For each entry, append it to the given */
  /* path and see if the file exists. If it exists, break the loop */
  while ((list = mg_next_comma_list_entry(list, &vec, NULL)) != NULL) {
    /* Prepare full path to the index file */
    snprintf(path + n, path_len - n, "/%.*s", (int) vec.len, vec.p);
    path[path_len - 1] = '\0';

    /* Does it exist? */
    if (!mg_stat(path, &st)) {
      /* Yes it does, break the loop */
      *stp = st;
      found = 1;
      break;
    }
  }

  /* If no index file exists, restore directory path, keep trailing slash. */
  if (!found) {
    path[n] = '\0';
    strncat(path + n, "/", path_len - n);
  }

  return found;
}

static void uri_to_path(struct http_message *hm, char *buf, size_t buf_len,
                        const struct mg_serve_http_opts *opts) {
  char uri[MG_MAX_PATH];
  struct mg_str a, b, *host_hdr = mg_get_http_header(hm, "Host");
  const char *rewrites = opts->url_rewrites;

  mg_url_decode(hm->uri.p, hm->uri.len, uri, sizeof(uri), 0);
  remove_double_dots(uri);
  snprintf(buf, buf_len, "%s%s", opts->document_root, uri);

  /* Handle URL rewrites */
  while ((rewrites = mg_next_comma_list_entry(rewrites, &a, &b)) != NULL) {
    if (a.len > 1 && a.p[0] == '@' && host_hdr != NULL &&
        host_hdr->len == a.len - 1 &&
        mg_ncasecmp(a.p + 1, host_hdr->p, a.len - 1) == 0) {
      /* This is a virtual host rewrite: @domain.name=document_root_dir */
      snprintf(buf, buf_len, "%.*s%s", (int) b.len, b.p, uri);
      break;
    } else {
      /* This is a usual rewrite, URI=directory */
      int match_len = mg_match_prefix(a.p, a.len, uri);
      if (match_len > 0) {
        snprintf(buf, buf_len, "%.*s%s", (int) b.len, b.p, uri + match_len);
        break;
      }
    }
  }
}

void mg_send_http_file(struct mg_connection *nc, char *path,
                       size_t path_buf_len, struct http_message *hm,
                       struct mg_serve_http_opts *opts) {
  int stat_result, is_directory, is_dav = is_dav_request(&hm->method);
  uint32_t remote_ip = ntohl(*(uint32_t *) &nc->sa.sin.sin_addr);
  cs_stat_t st;

  stat_result = mg_stat(path, &st);
  is_directory = !stat_result && S_ISDIR(st.st_mode);

  if (mg_check_ip_acl(opts->ip_acl, remote_ip) != 1) {
    /* Not allowed to connect */
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
  } else if (is_dav && opts->dav_document_root == NULL) {
    send_http_error(nc, 501, NULL);
  } else if (!is_authorized(hm, path, is_directory, opts)) {
    mg_printf(nc,
              "HTTP/1.1 401 Unauthorized\r\n"
              "WWW-Authenticate: Digest qop=\"auth\", "
              "realm=\"%s\", nonce=\"%lu\"\r\n"
              "Content-Length: 0\r\n\r\n",
              opts->auth_domain, (unsigned long) time(NULL));
  } else if ((stat_result != 0 || is_file_hidden(path, opts)) && !is_dav) {
    mg_printf(nc, "%s", "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
  } else if (is_directory && path[strlen(path) - 1] != '/' && !is_dav) {
    mg_printf(nc,
              "HTTP/1.1 301 Moved\r\nLocation: %.*s/\r\n"
              "Content-Length: 0\r\n\r\n",
              (int) hm->uri.len, hm->uri.p);
  } else if (S_ISDIR(st.st_mode) &&
             !find_index_file(path, path_buf_len, opts->index_files, &st)) {
    if (strcmp(opts->enable_directory_listing, "yes") == 0) {
#ifndef MG_DISABLE_DIRECTORY_LISTING
      send_directory_listing(nc, path, hm, opts);
#else
      send_http_error(nc, 501, NULL);
#endif
    } else {
      send_http_error(nc, 403, NULL);
    }
  } else if (mg_match_prefix(opts->cgi_file_pattern,
                             strlen(opts->cgi_file_pattern), path) > 0) {
    send_http_error(nc, 501, NULL);
  } else {
    mg_send_http_file2(nc, path, &st, hm, opts);
  }
}

void mg_serve_http(struct mg_connection *nc, struct http_message *hm,
                   struct mg_serve_http_opts opts) {
  char path[MG_MAX_PATH];
  uri_to_path(hm, path, sizeof(path), &opts);
  if (opts.per_directory_auth_file == NULL) {
    opts.per_directory_auth_file = ".htpasswd";
  }
  if (opts.enable_directory_listing == NULL) {
    opts.enable_directory_listing = "yes";
  }
  if (opts.cgi_file_pattern == NULL) {
    opts.cgi_file_pattern = "**.cgi$|**.php$";
  }
  if (opts.ssi_pattern == NULL) {
    opts.ssi_pattern = "**.shtml$|**.shtm$";
  }
  if (opts.index_files == NULL) {
    opts.index_files = "index.html,index.htm,index.shtml,index.cgi,index.php";
  }
  mg_send_http_file(nc, path, sizeof(path), hm, &opts);
}

#endif /* MG_DISABLE_FILESYSTEM */

#if 0
struct mg_connection *mg_connect_http(struct mg_mgr *mgr,
                                      mg_event_handler_t ev_handler,
                                      const char *url,
                                      const char *extra_headers,
                                      const char *post_data) {
  struct mg_connection *nc;
  char addr[1100], path[4096]; /* NOTE: keep sizes in sync with sscanf below */
  int use_ssl = 0, addr_len = 0;

  if (memcmp(url, "http://", 7) == 0) {
    url += 7;
  } else if (memcmp(url, "https://", 8) == 0) {
    url += 8;
    use_ssl = 1;
    return NULL; /* SSL is not enabled, cannot do HTTPS URLs */
  }

  addr[0] = path[0] = '\0';

  /* addr buffer size made smaller to allow for port to be prepended */
  sscanf(url, "%1095[^/]/%4095s", addr, path);
  if (strchr(addr, ':') == NULL) {
    addr_len = strlen(addr);
    strncat(addr, use_ssl ? ":443" : ":80", sizeof(addr) - (addr_len + 1));
  }

  if ((nc = mg_connect(mgr, addr, ev_handler)) != NULL) {
    mg_set_protocol_http_websocket(nc);

    if (use_ssl) {
    }

    if (addr_len) {
      /* Do not add port. See https://github.com/cesanta/mongoose/pull/304 */
      addr[addr_len] = '\0';
    }
    mg_printf(nc,
              "%s /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %lu\r\n%s\r\n%s",
              post_data == NULL ? "GET" : "POST", path, addr,
              post_data == NULL ? 0 : strlen(post_data),
              extra_headers == NULL ? "" : extra_headers,
              post_data == NULL ? "" : post_data);
  }

  return nc;
}
#endif

static size_t get_line_len(const char *buf, size_t buf_len) {
  size_t len = 0;
  while (len < buf_len && buf[len] != '\n') len++;
  return buf[len] == '\n' ? len + 1 : 0;
}

size_t mg_parse_multipart(const char *buf, size_t buf_len, char *var_name,
                          size_t var_name_len, char *file_name,
                          size_t file_name_len, const char **data,
                          size_t *data_len) {
  static const char cd[] = "Content-Disposition: ";
  size_t hl, bl, n, ll, pos, cdl = sizeof(cd) - 1;

  if (buf == NULL || buf_len <= 0) return 0;
  if ((hl = get_request_len(buf, (int)buf_len)) <= 0) return 0;
  if (buf[0] != '-' || buf[1] != '-' || buf[2] == '\n') return 0;

  /* Get boundary length */
  bl = get_line_len(buf, buf_len);

  /* Loop through headers, fetch variable name and file name */
  var_name[0] = file_name[0] = '\0';
  for (n = bl; (ll = get_line_len(buf + n, hl - n)) > 0; n += ll) {
    if (mg_ncasecmp(cd, buf + n, cdl) == 0) {
      struct mg_str header;
      header.p = buf + n + cdl;
      header.len = ll - (cdl + 2);
      mg_http_parse_header(&header, "name", var_name, var_name_len);
      mg_http_parse_header(&header, "filename", file_name, file_name_len);
    }
  }

  /* Scan through the body, search for terminating boundary */
  for (pos = hl; pos + (bl - 2) < buf_len; pos++) {
    if (buf[pos] == '-' && !memcmp(buf, &buf[pos], bl - 2)) {
      if (data_len != NULL) *data_len = (pos - 2) - hl;
      if (data != NULL) *data = buf + hl;
      return pos;
    }
  }

  return 0;
}

#endif /* MG_DISABLE_HTTP */
#ifdef NS_MODULE_LINES
#line 1 "src/util.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "internal.h" */

const char *mg_skip(const char *s, const char *end, const char *delims,
                    struct mg_str *v) {
  v->p = s;
  while (s < end && strchr(delims, *(unsigned char *) s) == NULL) s++;
  v->len = s - v->p;
  while (s < end && strchr(delims, *(unsigned char *) s) != NULL) s++;
  return s;
}

static int lowercase(const char *s) {
  return tolower(*(const unsigned char *) s);
}

int mg_ncasecmp(const char *s1, const char *s2, size_t len) {
  int diff = 0;

  if (len > 0) do {
      diff = lowercase(s1++) - lowercase(s2++);
    } while (diff == 0 && s1[-1] != '\0' && --len > 0);

  return diff;
}

int mg_casecmp(const char *s1, const char *s2) {
  return mg_ncasecmp(s1, s2, (size_t) ~0);
}

int mg_vcasecmp(const struct mg_str *str1, const char *str2) {
  size_t n2 = strlen(str2), n1 = str1->len;
  int r = mg_ncasecmp(str1->p, str2, (n1 < n2) ? n1 : n2);
  if (r == 0) {
    return (int)(n1 - n2);
  }
  return r;
}

int mg_vcmp(const struct mg_str *str1, const char *str2) {
  size_t n2 = strlen(str2), n1 = str1->len;
  int r = memcmp(str1->p, str2, (n1 < n2) ? n1 : n2);
  if (r == 0) {
    return (int)(n1 - n2);
  }
  return r;
}

#ifndef MG_DISABLE_FILESYSTEM
int mg_stat(const char *path, cs_stat_t *st) {
#ifdef _WIN32
  wchar_t wpath[MAX_PATH_SIZE];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  DBG(("[%ls] -> %d", wpath, _wstati64(wpath, st)));
  return _wstati64(wpath, (struct _stati64 *) st);
#else
  return stat(path, st);
#endif
}

FILE *mg_fopen(const char *path, const char *mode) {
#ifdef _WIN32
  wchar_t wpath[MAX_PATH_SIZE], wmode[10];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  to_wchar(mode, wmode, ARRAY_SIZE(wmode));
  return _wfopen(wpath, wmode);
#else
  return fopen(path, mode);
#endif
}

int mg_open(const char *path, int flag, int mode) { /* LCOV_EXCL_LINE */
#ifdef _WIN32
  wchar_t wpath[MAX_PATH_SIZE];
  to_wchar(path, wpath, ARRAY_SIZE(wpath));
  return _wopen(wpath, flag, mode);
#else
  return open(path, flag, mode); /* LCOV_EXCL_LINE */
#endif
}
#endif

#if 0
void mg_base64_encode(const unsigned char *src, int src_len, char *dst) {
  cs_base64_encode(src, src_len, dst);
}

int mg_base64_decode(const unsigned char *s, int len, char *dst) {
  return cs_base64_decode(s, len, dst);
}
#endif
/* Set close-on-exec bit for a given socket. */
void mg_set_close_on_exec(sock_t sock) {
#ifdef _WIN32
  (void) SetHandleInformation((HANDLE) sock, HANDLE_FLAG_INHERIT, 0);
#else
  fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif
}

void mg_sock_to_str(sock_t sock, char *buf, size_t len, int flags) {
  union socket_address sa;
#ifndef MG_CC3200
  socklen_t slen = sizeof(sa);
#endif

  memset(&sa, 0, sizeof(sa));
#ifndef MG_CC3200
  if (flags & MG_SOCK_STRINGIFY_REMOTE) {
    getpeername(sock, &sa.sa, &slen);
  } else {
    getsockname(sock, &sa.sa, &slen);
  }
#endif
  mg_sock_addr_to_str(&sa, buf, len, flags);
}

void mg_sock_addr_to_str(const union socket_address *sa, char *buf, size_t len,
                         int flags) {
  int is_v6;
  if (buf == NULL || len <= 0) return;
  buf[0] = '\0';
  is_v6 = 0;
  if (flags & MG_SOCK_STRINGIFY_IP) {
#if   defined(_WIN32) || defined(MG_ESP8266)
    /* Only Windoze Vista (and newer) have inet_ntop() */
    strncpy(buf, inet_ntoa(sa->sin.sin_addr), len);
#else
    inet_ntop(AF_INET, (void *) &sa->sin.sin_addr, buf, (int)len);
#endif
  }
  if (flags & MG_SOCK_STRINGIFY_PORT) {
    int port = ntohs(sa->sin.sin_port);
    if (flags & MG_SOCK_STRINGIFY_IP) {
      snprintf(buf + strlen(buf), len - (strlen(buf) + 1), "%s:%d",
               (is_v6 ? "]" : ""), port);
    } else {
      snprintf(buf, len, "%d", port);
    }
  }
}

int mg_hexdump(const void *buf, int len, char *dst, int dst_len) {
  const unsigned char *p = (const unsigned char *) buf;
  char ascii[17] = "";
  int i, idx, n = 0;

  for (i = 0; i < len; i++) {
    idx = i % 16;
    if (idx == 0) {
      if (i > 0) n += snprintf(dst + n, dst_len - n, "  %s\n", ascii);
      n += snprintf(dst + n, dst_len - n, "%04x ", i);
    }
    n += snprintf(dst + n, dst_len - n, " %02x", p[i]);
    ascii[idx] = p[i] < 0x20 || p[i] > 0x7e ? '.' : p[i];
    ascii[idx + 1] = '\0';
  }

  while (i++ % 16) n += snprintf(dst + n, dst_len - n, "%s", "   ");
  n += snprintf(dst + n, dst_len - n, "  %s\n\n", ascii);

  return n;
}

int mg_avprintf(char **buf, size_t size, const char *fmt, va_list ap) {
  va_list ap_copy;
  int len;

  va_copy(ap_copy, ap);
  len = vsnprintf(*buf, size, fmt, ap_copy);
  va_end(ap_copy);

  if (len < 0) {
    /* eCos and Windows are not standard-compliant and return -1 when
     * the buffer is too small. Keep allocating larger buffers until we
     * succeed or out of memory. */
    *buf = NULL; /* LCOV_EXCL_START */
    while (len < 0) {
      MG_FREE(*buf);
      size *= 2;
      if ((*buf = (char *) MG_MALLOC(size)) == NULL) break;
      va_copy(ap_copy, ap);
      len = vsnprintf(*buf, size, fmt, ap_copy);
      va_end(ap_copy);
    }
    /* LCOV_EXCL_STOP */
  } else if (len > (int) size) {
    /* Standard-compliant code path. Allocate a buffer that is large enough. */
    if ((*buf = (char *) MG_MALLOC(len + 1)) == NULL) {
      len = -1; /* LCOV_EXCL_LINE */
    } else {    /* LCOV_EXCL_LINE */
      va_copy(ap_copy, ap);
      len = vsnprintf(*buf, len + 1, fmt, ap_copy);
      va_end(ap_copy);
    }
  }

  return len;
}

#ifndef MG_DISABLE_FILESYSTEM
void mg_hexdump_connection(struct mg_connection *nc, const char *path,
                           int num_bytes, int ev) {
  const struct mbuf *io = ev == MG_EV_SEND ? &nc->send_mbuf : &nc->recv_mbuf;
  FILE *fp;
  char *buf, src[60], dst[60];
  int buf_size = num_bytes * 5 + 100;

  if ((fp = fopen(path, "a")) != NULL) {
    mg_sock_to_str(nc->sock, src, sizeof(src), 3);
    mg_sock_to_str(nc->sock, dst, sizeof(dst), 7);
    fprintf(
        fp, "%lu %p %s %s %s %d\n", (unsigned long) time(NULL), nc, src,
        ev == MG_EV_RECV ? "<-" : ev == MG_EV_SEND
                                      ? "->"
                                      : ev == MG_EV_ACCEPT
                                            ? "<A"
                                            : ev == MG_EV_CONNECT ? "C>" : "XX",
        dst, num_bytes);
    if (num_bytes > 0 && (buf = (char *) MG_MALLOC(buf_size)) != NULL) {
      mg_hexdump(io->buf + (ev == MG_EV_SEND ? 0 : io->len) -
                     (ev == MG_EV_SEND ? 0 : num_bytes),
                 num_bytes, buf, buf_size);
      fprintf(fp, "%s", buf);
      MG_FREE(buf);
    }
    fclose(fp);
  }
}
#endif

int mg_is_big_endian(void) {
  static const int n = 1;
  /* TODO(mkm) use compiletime check with 4-byte char literal */
  return ((char *) &n)[0] == 0;
}

const char *mg_next_comma_list_entry(const char *list, struct mg_str *val,
                                     struct mg_str *eq_val) {
  if (list == NULL || *list == '\0') {
    /* End of the list */
    list = NULL;
  } else {
    val->p = list;
    if ((list = strchr(val->p, ',')) != NULL) {
      /* Comma found. Store length and shift the list ptr */
      val->len = list - val->p;
      list++;
    } else {
      /* This value is the last one */
      list = val->p + strlen(val->p);
      val->len = list - val->p;
    }

    if (eq_val != NULL) {
      /* Value has form "x=y", adjust pointers and lengths */
      /* so that val points to "x", and eq_val points to "y". */
      eq_val->len = 0;
      eq_val->p = (const char *) memchr(val->p, '=', val->len);
      if (eq_val->p != NULL) {
        eq_val->p++; /* Skip over '=' character */
        eq_val->len = val->p + val->len - eq_val->p;
        val->len = (eq_val->p - val->p) - 1;
      }
    }
  }

  return list;
}

int mg_match_prefix(const char *pattern, int pattern_len, const char *str) {
  const char *or_str;
  int len, res, i = 0, j = 0;

  if ((or_str = (const char *) memchr(pattern, '|', pattern_len)) != NULL) {
    res = mg_match_prefix(pattern, (int)(or_str - pattern), str);
    return res > 0 ? res : mg_match_prefix(
                               or_str + 1,
                               (int)((pattern + pattern_len) - (or_str + 1)), str);
  }

  for (; i < pattern_len; i++, j++) {
    if (pattern[i] == '?' && str[j] != '\0') {
      continue;
    } else if (pattern[i] == '$') {
      return str[j] == '\0' ? j : -1;
    } else if (pattern[i] == '*') {
      i++;
      if (pattern[i] == '*') {
        i++;
        len = (int) strlen(str + j);
      } else {
        len = (int) strcspn(str + j, "/");
      }
      if (i == pattern_len) {
        return j + len;
      }
      do {
        res = mg_match_prefix(pattern + i, pattern_len - i, str + j + len);
      } while (res == -1 && len-- > 0);
      return res == -1 ? -1 : j + res + len;
    } else if (lowercase(&pattern[i]) != lowercase(&str[j])) {
      return -1;
    }
  }
  return j;
}
#ifdef NS_MODULE_LINES
#line 1 "src/json-rpc.c"
/**/
#endif
/* Copyright (c) 2014 Cesanta Software Limited */
/* All rights reserved */

#ifdef NS_MODULE_LINES
#line 1 "src/mqtt.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifdef NS_MODULE_LINES
#line 1 "src/mqtt-broker.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/* Amalgamated: #include "internal.h" */

#ifdef NS_MODULE_LINES
#line 1 "src/dns.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifdef NS_MODULE_LINES
#line 1 "src/dns-server.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifdef NS_MODULE_LINES
#line 1 "src/resolv.c"
/**/
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifdef NS_MODULE_LINES
#line 1 "src/coap.c"
/**/
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/* Amalgamated: #include "internal.h" */

