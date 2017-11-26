#include "node.h"
#include "node_buffer.h"
#include "v8.h"
#include "nan.h"
#include "errno.h"

#include <cinttypes>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>

namespace node {
namespace node_mmap {

using v8::Array;
using v8::Handle;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Value;

static void FreeCallback(char* data, void* hint) {
  int err;

  err = munmap(static_cast<void*>(data), static_cast<std::size_t>(reinterpret_cast<intptr_t>(hint)));
  assert(err == 0);
}


static void DontFree(char* data, void* hint) {
}


NAN_METHOD(Alloc) {
  Nan::HandleScope scope;

  std::size_t len;

  if (node::Buffer::HasInstance(info[0])) {
    len = *reinterpret_cast<std::size_t *>(node::Buffer::Data(info[0]));
  } else {
    len = info[0]->Uint32Value();
  }

  int prot = info[1]->Uint32Value();
  int flags = info[2]->Uint32Value();
  int fd = info[3]->Int32Value();

  std::int64_t off;

  if (node::Buffer::HasInstance(info[4])) {
    off = *reinterpret_cast<std::int64_t *>(node::Buffer::Data(info[4]));
  } else {
    off = info[4]->Int32Value();
  }

  void* res = mmap(NULL, len, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return Nan::ThrowError("mmap() call failed");

  const std::size_t max_len = Nan::imp::kMaxLength;
  Local<Value> ret;

  if (len <= max_len) {
    ret = Nan::NewBuffer(
      reinterpret_cast<char*>(res),
      len,
      FreeCallback,
      reinterpret_cast<void*>(static_cast<intptr_t>(len))).ToLocalChecked();
  } else {
    std::size_t ret_arr_len;

    if ((len % max_len) == 0) {
      ret_arr_len = len / max_len;
    } else {
      ret_arr_len = (len / max_len) + 1u;
    }

    Local<Array> ret_arr = Nan::New<Array>(ret_arr_len);

    for (std::size_t i = 0u; i < ret_arr_len; i++) {
      std::size_t curr_len;

      if (i == (ret_arr_len - 1) &&
          (len % max_len) != 0) {
        curr_len = len % max_len;
      } else {
        curr_len = max_len;
      }

      ret_arr->Set(i, Nan::NewBuffer(
        reinterpret_cast<char*>(res) + (i * max_len),
        curr_len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(curr_len))).ToLocalChecked());
    }

    ret = ret_arr;
  }

  info.GetReturnValue().Set(ret);
}


NAN_METHOD(AlignedAlloc) {
  Nan::HandleScope scope;

  int len = info[0]->Uint32Value();
  int prot = info[1]->Uint32Value();
  int flags = info[2]->Uint32Value();
  int fd = info[3]->Int32Value();
  int off = info[4]->Uint32Value();
  int align = info[5]->Uint32Value();

  void* res = mmap(NULL, len + align, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return Nan::ThrowError("mmap() call failed");

  Local<Object> buf = Nan::NewBuffer(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len + align))).ToLocalChecked();

  intptr_t ptr = reinterpret_cast<intptr_t>(res);
  if (ptr % align == 0)
    info.GetReturnValue().Set(buf);

  // Slice the buffer if it was unaligned
  int slice_off = align - (ptr % align);
  Local<Object> slice = Nan::NewBuffer(
      reinterpret_cast<char*>(res) + slice_off,
      len,
      DontFree,
      NULL).ToLocalChecked();

  Nan::SetPrivate(slice, Nan::New("alignParent").ToLocalChecked(), buf);
  info.GetReturnValue().Set(slice);
}


NAN_METHOD(Sync) {
  Nan::HandleScope scope;
  char* data = node::Buffer::Data(info[0]->ToObject());
  size_t length =  node::Buffer::Length(info[0]->ToObject());

  if (info.Length() > 1) {
    // optional argument: offset
    const size_t offset = info[1]->Uint32Value();
    if (length <= offset) {
      return Nan::ThrowRangeError("Offset out of bounds");
    } else if (offset > 0 && (offset % sysconf(_SC_PAGE_SIZE))) {
      return Nan::ThrowRangeError("Offset must be a multiple of page size");
    }

    data += offset;
    length -= offset;
  }

  if (info.Length() > 2) {
    // optional argument: length
    const size_t range = info[2]->Uint32Value();
    if (range < length) {
      length = range;
    }
  }

  int flags;
  if (info.Length() > 3) {
    // optional argument: flags
    flags = info[3]->Uint32Value();
  } else {
    flags = MS_SYNC;
  }

  if (msync(data, length, flags) == 0) {
    info.GetReturnValue().Set(Nan::True());
  } else {
    return Nan::ThrowError(strerror(errno));
  }
}


static void Init(Handle<Object> target) {
  Nan::SetMethod(target, "alloc", Alloc);
  Nan::SetMethod(target, "alignedAlloc", AlignedAlloc);
  Nan::SetMethod(target, "sync", Sync);

  target->Set(Nan::New("PROT_NONE").ToLocalChecked(), Nan::New<Number>(PROT_NONE));
  target->Set(Nan::New("PROT_READ").ToLocalChecked(), Nan::New<Number>(PROT_READ));
  target->Set(Nan::New("PROT_WRITE").ToLocalChecked(), Nan::New<Number>(PROT_WRITE));
  target->Set(Nan::New("PROT_EXEC").ToLocalChecked(), Nan::New<Number>(PROT_EXEC));

  Local<Number> map_anonymous_flag_number;
  bool map_anonymous_supported = false;
#if defined(MAP_ANONYMOUS)
  map_anonymous_flag_number = Nan::New<Number>(MAP_ANONYMOUS);
  map_anonymous_supported = true;
#elif defined(MAP_ANON)
  map_anonymous_flag_number = Nan::New<Number>(MAP_ANON);
  map_anonymous_supported = true;
#else
  map_anonymous_flag_number = Nan::New<Number>(0x00);
#endif
  target->Set(Nan::New("MAP_ANON").ToLocalChecked(), map_anonymous_flag_number);
  target->Set(Nan::New("MAP_ANONYMOUS").ToLocalChecked(), map_anonymous_flag_number);
  target->Set(Nan::New("MAP_ANONYMOUS_SUPPORTED").ToLocalChecked(),
    (map_anonymous_supported) ?
      Nan::True() :
      Nan::False());
  target->Set(Nan::New("MAP_PRIVATE").ToLocalChecked(), Nan::New<Number>(MAP_PRIVATE));
  target->Set(Nan::New("MAP_SHARED").ToLocalChecked(), Nan::New<Number>(MAP_SHARED));
  target->Set(Nan::New("MAP_FIXED").ToLocalChecked(), Nan::New<Number>(MAP_FIXED));

  target->Set(Nan::New("MS_ASYNC").ToLocalChecked(), Nan::New<Number>(MS_ASYNC));
  target->Set(Nan::New("MS_SYNC").ToLocalChecked(), Nan::New<Number>(MS_SYNC));
  target->Set(Nan::New("MS_INVALIDATE").ToLocalChecked(), Nan::New<Number>(MS_INVALIDATE));

  target->Set(Nan::New("PAGE_SIZE").ToLocalChecked(), Nan::New<Number>(
    sysconf(_SC_PAGE_SIZE)));
}


}  // namespace node_mmap
}  // namespace node

NODE_MODULE(mmap, node::node_mmap::Init);
