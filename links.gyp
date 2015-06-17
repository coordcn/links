{
  'targets': [
    {
      'target_name': 'liblinks',
      'type': 'static_library',
      'dependencies': [
        'deps/http_parser/http_parser.gyp:http_parser',
        'deps/lua/lua.gyp:liblua',
        'deps/cares/cares.gyp:cares',
        'deps/uv/uv.gyp:libuv',
        'deps/zlib/zlib.gyp:zlib',
        'deps/openssl/openssl.gyp:openssl',
      ],
      'export_dependent_settings': [
        'deps/http_parser/http_parser.gyp:http_parser',
        'deps/lua/lua.gyp:liblua',
        'deps/cares/cares.gyp:cares',
        'deps/uv/uv.gyp:libuv',
        'deps/zlib/zlib.gyp:zlib',
        'deps/openssl/openssl.gyp:openssl',
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'defines': [ '_GNU_SOURCE' ]
        }]
      ],
      'include_dirs': [
        'src',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'src',
        ]
      },
      'sources': [
        'src/alloc.c',
        'src/palloc.c',
        'src/pmem.c',
        'src/buf.c',
        'src/string.c',
        'src/hash.c',
        'src/util.c',
        'src/system.c',
        'src/process.c',
        'src/dns.c',
        'src/tcp.c',
        'src/http.c',
        'src/init.c',
      ],
    },
    {
      'target_name': 'links',
      'type': 'executable',
      'dependencies': [
        'liblinks',
      ],
      'include_dirs': [
        'src',
      ],
      'direct_dependent_settings': {
       'include_dirs': [
         'src',
       ]
      },
      'sources': [
        'src/main.c',
      ],
      'msvs-settings': {
        'VCLinkerTool': {
          'SubSystem': 1, # /subsystem:console
        },
      },
      'conditions': [
        ['OS == "win"', {
          'libraries': [
            '-lgdi32.lib',
            '-luser32.lib'
          ],
        }],
        ['OS == "linux"', {
          'libraries': [ '-ldl' ],
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'cflags': [ '--std=c99' ],
          'defines': [ '_GNU_SOURCE' ]
        }],
      ],
      'defines': [ 'BUNDLE=1' ]
    },
  ],
}
