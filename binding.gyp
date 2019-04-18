{
  "targets": [
    {
      "target_name": "canberra",
      "sources": [ "src/canberra-binding.cc", "src/canberra-context.cc" ],
      "cflags": [
          '-Wall',
          '-Wno-deprecated-declarations',
          '-Wno-cast-function-type'
      ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}