// mmap test
var assert = require('assert');
var fs = require('fs');
var mmap = require('../');

describe('mmap.js', function() {
  describe('.alloc()', function() {
    it('should allocate anonymous memory', function() {
      var buf = mmap.alloc(
          mmap.PAGE_SIZE,
          mmap.PROT_READ | mmap.PROT_WRITE,
          mmap.MAP_ANON | mmap.MAP_PRIVATE,
          -1,
          0);
      assert(buf);

      buf[0] = 123;
      buf = null;
      gc();
    });

    it('should allocate file-bound memory', function() {
      var fd = fs.openSync(__filename, 'r');
      var buf = mmap.alloc(
          mmap.PAGE_SIZE,
          mmap.PROT_READ,
          mmap.MAP_PRIVATE,
          fd,
          0);
      fs.closeSync(fd);
      assert(buf);

      assert(/mmap test/.test(buf.slice(0, 100).toString()));
      buf = null;
      gc();
    });
  });

  describe('.alignedAlloc()', function() {
    it('should allocate aligned memory', function() {
      var buf = mmap.alignedAlloc(
          mmap.PAGE_SIZE,
          mmap.PROT_READ | mmap.PROT_WRITE,
          mmap.MAP_ANON | mmap.MAP_PRIVATE,
          -1,
          0,
          1024 * 1024);
      assert(buf);
      assert.equal(buf.length, mmap.PAGE_SIZE);

      buf[0] = 123;
      buf = null;
      gc();
    });
  });

  describe('.sync()', function () {
    before(function () {
      this.file = __dirname + '/test.txt';
      fs.writeFileSync(this.file, 'Hello World');
    });
    after(function (done) {
      fs.unlink(this.file, done);
    });
    it('should sync some memory to disk', function () {
      var fd = fs.openSync(this.file, 'r+');
      var buf = mmap.alloc(
          mmap.PAGE_SIZE,
          mmap.PROT_READ | mmap.PROT_WRITE,
          mmap.MAP_SHARED,
          fd,
          0);
      fs.closeSync(fd);
      assert(buf);
      buf.write("! Greetings.", 11);
      assert(mmap.sync(buf, 0, 23, mmap.MS_SYNC));

      assert(/Hello World! Greetings./.test(buf.toString('utf8')));
      buf = null;
      gc();

      var content = fs.readFileSync(this.file, 'utf8');
      assert(content === 'Hello World! Greetings.');
    });
  });
});
