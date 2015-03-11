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
  NanScope();

  int len = args[0]->Uint32Value();
  int prot = args[1]->Uint32Value();
  int flags = args[2]->Uint32Value();
  int fd = args[3]->Int32Value();
  int off = args[4]->Uint32Value();

  void* res = mmap(NULL, len, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return NanThrowError("mmap() call failed");

  NanReturnValue(NanNewBufferHandle(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len))));
}


NAN_METHOD(AlignedAlloc) {
  NanScope();

  int len = args[0]->Uint32Value();
  int prot = args[1]->Uint32Value();
  int flags = args[2]->Uint32Value();
  int fd = args[3]->Int32Value();
  int off = args[4]->Uint32Value();
  int align = args[5]->Uint32Value();

  void* res = mmap(NULL, len + align, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return NanThrowError("mmap() call failed");

  Local<Object> buf = NanNewBufferHandle(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len + align)));

  intptr_t ptr = reinterpret_cast<intptr_t>(res);
  if (ptr % align == 0)
    NanReturnValue(buf);

  // Slice the buffer if it was unaligned
  int slice_off = align - (ptr % align);
  Local<Object> slice = NanNewBufferHandle(
      reinterpret_cast<char*>(res) + slice_off,
      len,
      DontFree,
      NULL);

  slice->SetHiddenValue(NanNew("alignParent"), buf);
  NanReturnValue(slice);
}


NAN_METHOD(Sync) {
  NanScope();
  char* data = node::Buffer::Data(args[0]->ToObject());
  size_t length =  node::Buffer::Length(args[0]->ToObject());

  if (args.Length() > 1) {
    // optional argument: offset
    const size_t offset = args[1]->Uint32Value();
    if (length <= offset) {
      return NanThrowRangeError("Offset out of bounds");
    } else if (offset > 0 && (offset % getpagesize())) {
      return NanThrowRangeError("Offset must be a multiple of page size");
    }

    data += offset;
    length -= offset;
  }

  if (args.Length() > 2) {
    // optional argument: length
    const size_t range = args[2]->Uint32Value();
    if (range < length) {
      length = range;
    }
  }

  int flags;
  if (args.Length() > 3) {
    // optional argument: flags
    flags = args[3]->Uint32Value();
  } else {
    flags = MS_SYNC;
  }

  if (msync(data, length, flags) == 0) {
    NanReturnValue(NanTrue());
  } else {
    return NanThrowError(strerror(errno), errno);
  }
}


static void Init(Handle<Object> target) {
  NODE_SET_METHOD(target, "alloc", Alloc);
  NODE_SET_METHOD(target, "alignedAlloc", AlignedAlloc);
  NODE_SET_METHOD(target, "sync", Sync);

  target->Set(NanNew("PROT_NONE"), NanNew<Number>(PROT_NONE));
  target->Set(NanNew("PROT_READ"), NanNew<Number>(PROT_READ));
  target->Set(NanNew("PROT_WRITE"), NanNew<Number>(PROT_WRITE));
  target->Set(NanNew("PROT_EXEC"), NanNew<Number>(PROT_EXEC));

  target->Set(NanNew("MAP_ANON"), NanNew<Number>(MAP_ANON));
  target->Set(NanNew("MAP_PRIVATE"), NanNew<Number>(MAP_PRIVATE));
  target->Set(NanNew("MAP_SHARED"), NanNew<Number>(MAP_SHARED));
  target->Set(NanNew("MAP_FIXED"), NanNew<Number>(MAP_FIXED));

  target->Set(NanNew("MS_ASYNC"), NanNew<Number>(MS_ASYNC));
  target->Set(NanNew("MS_SYNC"), NanNew<Number>(MS_SYNC));
  target->Set(NanNew("MS_INVALIDATE"), NanNew<Number>(MS_INVALIDATE));

  target->Set(NanNew("PAGE_SIZE"), NanNew<Number>(getpagesize()));
}


}  // namespace node_mmap
}  // namespace node

NODE_MODULE(mmap, node::node_mmap::Init);
