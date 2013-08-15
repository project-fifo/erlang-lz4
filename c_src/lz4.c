/*
Copyright 2012, Joe Williams <joe@joetify.com>

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include <erl_nif.h>
#include <erl_driver.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h> 

#include "sys/types.h"
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"

static ERL_NIF_TERM atom_ok;
static ERL_NIF_TERM atom_error;

static inline void store_le32(unsigned char *c, uint32_t x)
{
  c[0] = x & 0xff;
  c[1] = (x >> 8) & 0xff;
  c[2] = (x >> 16) & 0xff;
  c[3] = (x >> 24) & 0xff;
}

static inline uint32_t load_le32(unsigned const char *c)
{
  const uint8_t *d = (const uint8_t *)c;
  uint32_t r = d[0];

  r = (r << 8) | d[1];
  r = (r << 8) | d[2];
  r = (r << 8) | d[3];
  return r;
}

static const int hdr_size = sizeof(uint32_t);

static int
load(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
  atom_ok = enif_make_atom(env, "ok");
  atom_error = enif_make_atom(env, "error");

  return 0;
}

static int
reload(ErlNifEnv* env, void** priv, ERL_NIF_TERM load_info)
{
  return 0;
}

static int
upgrade(ErlNifEnv* env, void** priv, void** old_priv, ERL_NIF_TERM load_info)
{
  return 0;
}

static void
unload(ErlNifEnv* env, void* priv)
{
  return;
}

/**********************************************************************
 * Name: compress
 *
 * Desc: compress binary
 */

static ERL_NIF_TERM
compress(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result, out;
  ErlNifBinary source;
  unsigned char *result_buf = NULL;
  int dest_size;
  int real_size;

  if (!enif_inspect_binary(env, argv[0], &source)) return enif_make_badarg(env);

  dest_size = hdr_size + LZ4_compressBound(source.size);

  result_buf = enif_make_new_binary(env, dest_size, &result);

  store_le32(result_buf, source.size);

  real_size = LZ4_compress((const char *) source.data, (char *) result_buf + hdr_size, source.size);
  out = enif_make_sub_binary(env, result, 0, real_size);

  return enif_make_tuple2(env, atom_ok, out);
}

/**********************************************************************
 * Name: compress_hc
 *
 * Desc: compress (high) binary
 */

static ERL_NIF_TERM
compress_hc(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {

  return atom_ok;
}

/**********************************************************************
 * Name: uncompress
 *
 * Desc: uncompress binary
 */

static ERL_NIF_TERM
uncompress(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[]) {
  ERL_NIF_TERM result;
  ErlNifBinary source;
  unsigned char *result_buf = NULL;
  uint32_t dest_size;

  if (!enif_inspect_binary(env, argv[0], &source)) return enif_make_badarg(env);

  dest_size = load_le32(source.data);

  result_buf = enif_make_new_binary(env, dest_size, &result);

  LZ4_uncompress((const char*) source.data + hdr_size, (char *) result_buf, dest_size);

  return enif_make_tuple2(env, atom_ok, result);
}

static ErlNifFunc nif_funcs[] = {
  {"compress", 1, compress},
  {"compress_hc", 1, compress_hc},
  {"uncompress", 1, uncompress}
};

ERL_NIF_INIT(lz4, nif_funcs, &load, &reload, &upgrade, &unload);

