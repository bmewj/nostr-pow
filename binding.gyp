{
  "targets": [{
    "target_name": "nostr-pow",
    "sources": [
      "./src/nostr-pow.cpp",
      "./src/compute_nonce.c",
      "./src/sha256.c"
    ],
    "cflags": [
      "-Wall",
      "-Wno-maybe-uninitialized",
      "-Wno-uninitialized",
      "-Wno-unused-function",
      "-Wextra"
    ],
    "cflags_cc+": [
      "-std=c++0x"
    ],
    "include_dirs": [
      "/usr/local/include",
      "<!(node -e \"require('nan')\")"
    ],
  }]
}
