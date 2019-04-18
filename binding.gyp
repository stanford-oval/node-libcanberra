{
  "targets": [
    {
      "target_name": "canberra",
      "sources": [ "src/canberra-binding.cc", "src/canberra-context.cc" ],
      "cflags": [
          '-Wall',
          '-Wno-deprecated-declarations',
          '-Wno-cast-function-type',
          '<!@(pkg-config --cflags-only-other libcanberra)',
      ],
      'ldflags': [
          '<!@(pkg-config  --libs-only-L --libs-only-other libcanberra)'
      ],
      'libraries': [
          '<!@(pkg-config  --libs-only-l --libs-only-other libcanberra)'
      ],
      "include_dirs": [
          "<!(node -e \"require('nan')\")",
          '<!@(pkg-config --cflags-only-I libcanberra)',
      ]
    }
  ]
}
