#include "node.h"
#include "node_buffer.h"
#include "v8.h"
#include "nan.h"

#include <sys/mman.h>
#include <unistd.h>

namespace node_mmap {

using v8::Handle;
using v8::Number;
using v8::Object;

void FreeCallback(char* data, void* hint) {
  int err;

  err = munmap(data, reinterpret_cast<intptr_t>(hint));
  assert(err == 0);
}


NAN_METHOD(Alloc) {
  NanEscapableScope();

  int len = args[0]->Uint32Value();
  int prot = args[1]->Uint32Value();
  int flags = args[2]->Uint32Value();
  int fd = args[3]->Int32Value();
  int off = args[4]->Uint32Value();

  void* res = mmap(NULL, len, prot, flags, fd, off);
  if (res == MAP_FAILED)
    return NanThrowError("mmap() call failed");

  return NanEscapeScope(NanNewBufferHandle(
        reinterpret_cast<char*>(res),
        len,
        FreeCallback,
        reinterpret_cast<void*>(static_cast<intptr_t>(len))));
}


static void Init(Handle<Object> target) {
  NODE_SET_METHOD(target, "alloc", Alloc);

  target->Set(NanNew("PROT_NONE"), NanNew<Number, uint32_t>(PROT_NONE));
  target->Set(NanNew("PROT_READ"), NanNew<Number, uint32_t>(PROT_READ));
  target->Set(NanNew("PROT_WRITE"), NanNew<Number, uint32_t>(PROT_WRITE));
  target->Set(NanNew("PROT_EXEC"), NanNew<Number, uint32_t>(PROT_EXEC));

  target->Set(NanNew("MAP_ANON"), NanNew<Number, uint32_t>(MAP_ANON));
  target->Set(NanNew("MAP_PRIVATE"), NanNew<Number, uint32_t>(MAP_PRIVATE));
  target->Set(NanNew("MAP_SHARED"), NanNew<Number, uint32_t>(MAP_SHARED));

  target->Set(NanNew("PAGE_SIZE"), NanNew<Number, uint32_t>(getpagesize()));
}


NODE_MODULE(mmap, Init);

}  // namespace node_mmap
