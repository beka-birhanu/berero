#include <stdio.h>
#include <zlib.h>

#include "blob.h"

#define CHUNK_SIZE 16384

int chimek(FILE *source, FILE *dest) {
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK_SIZE];
  unsigned char out[CHUNK_SIZE];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  // windowBits = 15 + 16 produces gzip format
  ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                     Z_DEFAULT_STRATEGY);
  if (ret != Z_OK)
    return ret;

  do {
    strm.avail_in = fread(in, 1, CHUNK_SIZE, source);
    if (ferror(source)) {
      deflateEnd(&strm);
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;

    do {
      strm.avail_out = CHUNK_SIZE;
      strm.next_out = out;
      ret = deflate(&strm, flush);
      if (ret == Z_STREAM_ERROR) {
        deflateEnd(&strm);
        return ret;
      }
      have = CHUNK_SIZE - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        deflateEnd(&strm);
        return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
  } while (flush != Z_FINISH);

  ret = deflateEnd(&strm);
  return (ret == Z_OK) ? Z_OK : ret;
}

int zerga(FILE *source, FILE *dest) {
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK_SIZE];
  unsigned char out[CHUNK_SIZE];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  // windowBits = 32 + 15 auto-detects zlib or gzip format
  ret = inflateInit2(&strm, 32 + 15);
  if (ret != Z_OK)
    return ret;

  do {
    strm.avail_in = fread(in, 1, CHUNK_SIZE, source);
    if (ferror(source)) {
      inflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = in;

    do {
      strm.avail_out = CHUNK_SIZE;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR ||
          ret == Z_MEM_ERROR) {
        inflateEnd(&strm);
        return ret;
      }
      have = CHUNK_SIZE - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        inflateEnd(&strm);
        return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
  } while (ret != Z_STREAM_END);

  inflateEnd(&strm);
  return (ret == Z_STREAM_END) ? Z_OK : Z_DATA_ERROR;
}
