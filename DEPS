vars = {
  'chromium_git': 'https://chromium.googlesource.com',
  'buildtools_revision': 'd511e4d53d6fc8037badbdf4b225c137e94b52fb',
  'gtest_revision': '42bc671f47b122fad36db5eccbc06868afdf7862',
  'gyp_revision': 'd61a9397e668fa9843c4aa7da9e79460fe590bfb',
  'picojson_revision': '990f6f8f2f284d0e670c058002a0ea86e0b9b2ea',
}

deps = {
  'm88/buildtools': Var('chromium_git') + '/chromium/buildtools.git' + '@' + Var('buildtools_revision'),
  'm88/third_party/gtest': 'git@github.com:google/googletest.git' + '@' + Var('gtest_revision'),
  'm88/third_party/picojson': 'git://github.com/kazuho/picojson.git' + '@' + Var('picojson_revision'),
  'm88/tools/gyp': Var('chromium_git') + '/external/gyp.git' + '@' + Var('gyp_revision'),
}

hooks = [
  # Pull GN binaries. This needs to be before running GYP below.
  {
    'name': 'gn_win',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'm88/buildtools/win/gn.exe.sha1',
    ],
  },
  {
    'name': 'gn_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'm88/buildtools/mac/gn.sha1',
    ],
  },
  {
    'name': 'gn_linux64',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-gn',
                '-s', 'm88/buildtools/linux64/gn.sha1',
    ],
  },
  # Pull clang-format binaries using checked-in hashes.
  {
    'name': 'clang_format_win',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=win32',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'm88/buildtools/win/clang-format.exe.sha1',
    ],
  },
  {
    'name': 'clang_format_mac',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=darwin',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'm88/buildtools/mac/clang-format.sha1',
    ],
  },
  {
    'name': 'clang_format_linux',
    'pattern': '.',
    'action': [ 'download_from_google_storage',
                '--no_resume',
                '--platform=linux*',
                '--no_auth',
                '--bucket', 'chromium-clang-format',
                '-s', 'm88/buildtools/linux64/clang-format.sha1',
    ],
  },
]

recursedeps = [
  'm88/buildtools',
]
