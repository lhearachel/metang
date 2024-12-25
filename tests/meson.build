tests = [
    'append',
    'append_prepend',
    'bitmask',
    'leader',
    'override',
    'pascal',
    'prepend',
    'preproc_guard',
    'start_from',
]

test_files = []
foreach test_name : tests
    test_files += files(test_name + '.test')
    test(
        test_name,
        metang_runtests,
        args: [test_name],
    )
endforeach
