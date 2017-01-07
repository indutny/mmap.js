#include "node.h"
#include "node_buffer.h"
#include "v8.h"
#include "nan.h"
#include "errno.h"

#include <sys/mman.h>
#include <unistd.h>

namespace node {
namespace node_mmap {

using v8::Handle;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Value;

static void FreeCallback(char* data, void* hint) {
  int err;

  err = munmap(data, reinterpret_cast<intptr_t>(hint));
  assert(err == 0);
}


static void DontFree(char* data, void* hint) {
}


NAN_METHOD(Alloc) {
  Nan::HandleScope scope;

  int len = info[0]->Uint32Value();
  int prot = info[1]->Uint32Value();
  int flags = info[2]->Uint32Value();
  int fd = info[3]->Int32Value();
  int off = info[4]->Uint32Value();

  void* res = mmap(NULL, len, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return Nan::ThrowError("mmap() call failed");

  info.GetReturnValue().Set(Nan::NewBuffer(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len))).ToLocalChecked());
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
    } else if (offset > 0 && (offset % getpagesize())) {
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

  target->Set(Nan::New("MAP_ANON").ToLocalChecked(), Nan::New<Number>(MAP_ANON));
  target->Set(Nan::New("MAP_PRIVATE").ToLocalChecked(), Nan::New<Number>(MAP_PRIVATE));
  target->Set(Nan::New("MAP_SHARED").ToLocalChecked(), Nan::New<Number>(MAP_SHARED));
  target->Set(Nan::New("MAP_FIXED").ToLocalChecked(), Nan::New<Number>(MAP_FIXED));

  target->Set(Nan::New("MS_ASYNC").ToLocalChecked(), Nan::New<Number>(MS_ASYNC));
  target->Set(Nan::New("MS_SYNC").ToLocalChecked(), Nan::New<Number>(MS_SYNC));
  target->Set(Nan::New("MS_INVALIDATE").ToLocalChecked(), Nan::New<Number>(MS_INVALIDATE));

  target->Set(Nan::New("PAGE_SIZE").ToLocalChecked(), Nan::New<Number>(getpagesize()));
}


}  // namespace node_mmap
}  // namespace node

NODE_MODULE(mmap, node::node_mmap::Init);
