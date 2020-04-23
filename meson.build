project('libyconfig', 'c',
        version : '0.0.1',
        default_options : [
            'warning_level=3',
            'werror=true',
            'c_std=c99',
        ],
        license : 'MIT',
        meson_version: '>= 0.49',
)


cc = meson.get_compiler('c')

flags = [
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wno-padded',
]
add_project_arguments(cc.get_supported_arguments(flags), language : 'c')

libyaml = cc.find_library('libyaml', required : true)

# follow semantic versionning (https://semver.org)
major = '0'  # incompatible API changes
minor = '0'  # add backwards-compatible functionality
patch = '1'  # backwards-compatible bug fixes
version = major + '.' + minor + '.' + patch

sources = files(
    'src/common.h',
    'src/yconfig.c',
    'src/yconfig.h',
)
install_headers('src/yconfig.h')

libyconfig = shared_library('yconfig',
    sources,
    version : version,
    install : true,
    include_directories : include_directories('src'),
    dependencies : [libyaml],
)

#
# TESTS
#

all_tests_sources = []
if get_option('tests')
    smoketest_files = files('tests/smoketest.c', 'tests/check.h')
    smoketest = executable('smoketest',
        'tests/smoketest.c',
        include_directories : include_directories('src', 'tests'),
        link_with : libyconfig,
        dependencies : [libyaml],
    )
    all_tests_sources += smoketest_files
    test('smoketest',
        smoketest,
        args : ['tests/config.yaml'],
    )
endif # tests

#
# DEVTOOLS
#

codespell = find_program('codespell', required : false)
if codespell.found()
    run_target('spelling',
        command : [
            codespell,
            sources,
            all_tests_sources,
        ]
    )
endif # codespell
