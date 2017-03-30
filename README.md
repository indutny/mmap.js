# mmap.js

Working `mmap()` binding for node.js

## API

```js
const mmap = require('mmap.js');

// Allocate chunk of memory, see `man 2 mmap`
const buf = mmap.alloc(
    mmap.PAGE_SIZE /* len */,
    mmap.PROT_READ | mmap.PROT_WRITE /* prot */,
    mmap.MAP_ANON | mmap.MAP_PRIVATE /* flags */,
    -1 /* fd */,
    0 /* offset */);

// File mapping:
const fd = fs.openSync(this.file2, 'r+');
const fileBuf = mmap.alloc(
    mmap.PAGE_SIZE * 4,
    mmap.PROT_READ | mmap.PROT_WRITE,
    mmap.MAP_SHARED,
    fd,
    0);
fs.closeSync(fd);

// Syncing data
mmap.sync(fileBuf, 0 /* off */, mmap.PAGE_SIZE /* len */,
          mmap.MS_SYNC /* flags, default: MS_SYNC */)

// Allocating memory with aligned address (low bits set to 0)
const abuf = mmap.alignedAlloc(
    mmap.PAGE_SIZE,
    mmap.PROT_READ | mmap.PROT_WRITE,
    mmap.MAP_ANON | mmap.MAP_PRIVATE,
    -1,
    0,
    1024 * 1024 /* alignment value, address will be a multiple of it */);
```

### License

This software is licensed under the MIT License.

Copyright Fedor Indutny, 2014.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
