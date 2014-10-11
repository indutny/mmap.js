{
  "targets": [{
    "target_name": "mmap",
    "include_dirs": [
      "src",
      "<!(node -e \"require('nan')\")",
    ],
    "sources": [
      "src/mmap.cc",
    ],
  }],
}
