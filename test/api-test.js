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
    beforeEach(function () {
      this.file = __dirname + '/test.txt';
      fs.writeFileSync(this.file, Array(1000).join('Hello World '));
    });
    afterEach(function (done) {
      fs.unlink(this.file, done);
    });
    it('should sync some memory to disk', function () {
      var fd = fs.openSync(this.file, 'r+');
      var buf = mmap.alloc(
          fs.fstatSync(fd).size,
          mmap.PROT_READ | mmap.PROT_WRITE,
          mmap.MAP_SHARED,
          fd,
          0);
      fs.closeSync(fd);
      assert(buf);
      buf.write("There", 6);
      buf.write("There", buf.length - 6);
      assert(/Hello There/.test(buf.toString('utf8')));
      assert(mmap.sync(buf, 0, mmap.PAGE_SIZE, mmap.MS_SYNC));
      assert(mmap.sync(buf, mmap.PAGE_SIZE, mmap.PAGE_SIZE, mmap.MS_SYNC));
      var content = fs.readFileSync(this.file, 'utf8');
      assert(content.slice(0, 11) === 'Hello There');
      assert(content.slice(-12) === 'Hello There ');
      buf = null;
      gc();
    });

    it('should sync 4 pages at once', function () {
      var file = __dirname + '/test-4-pages.txt';
      fs.writeFileSync(file, (new Buffer(mmap.PAGE_SIZE * 4)).fill(0));
      var fd = fs.openSync(file, 'r+');
      var buf = mmap.alloc(
          mmap.PAGE_SIZE * 4,
          mmap.PROT_READ | mmap.PROT_WRITE,
          mmap.MAP_SHARED,
          fd,
          0);
      fs.closeSync(fd);
      try {
        assert(buf);
        buf.fill(65);
        assert(mmap.sync(buf));
        var content = fs.readFileSync(file);
        for (var i = 0; i < content.length; i++) {
          assert(content[i] === 65);
        }
        fs.unlinkSync(file);
      }
      catch (e) {
        fs.unlinkSync(file);
        throw e;
      }
    });

    describe('sync errors', function () {
      var buf;
      beforeEach(function () {
        var fd = fs.openSync(this.file, 'r+');
        buf = mmap.alloc(
            fs.fstatSync(fd).size,
            mmap.PROT_READ | mmap.PROT_WRITE,
            mmap.MAP_SHARED,
            fd,
            0);
        fs.closeSync(fd);
        assert(buf);
      });

      it('should reject invalid page size offsets', function () {
        var threw = false;
        try {
          mmap.sync(buf, 2);
        }
        catch (e) {
          assert(/multiple of page size/i.test(e.message));
          threw = true;
        }
        assert(threw);
      });

      it('should reject offsets larger than the length', function () {
        var threw = false;
        try {
          mmap.sync(buf, 2000000000);
        }
        catch (e) {
          assert(/offset out of bounds/i.test(e.message));
          threw = true;
        }
        assert(threw);
      });
    });
  });
});
