tests = [
    {'command': 'help', 'name': 'base'},
    {'command': 'version', 'name': 'base'},

    {'command': 'enum', 'name': 'append'},
    {'command': 'enum', 'name': 'base'},
    {'command': 'enum', 'name': 'direct_assignment'},
    {'command': 'enum', 'name': 'guard'},
    {'command': 'enum', 'name': 'leader'},
    {'command': 'enum', 'name': 'prepend'},
    {'command': 'enum', 'name': 'start_from'},
    {'command': 'enum', 'name': 'tag_name'},

    {'command': 'mask', 'name': 'base'},
    {'command': 'mask', 'name': 'guard'},
    {'command': 'mask', 'name': 'leader'},
    {'command': 'mask', 'name': 'tag_name'},
]

test_files = []
foreach test : tests
    test_command = test['command']
    test_name = test['name']
    test_files += files(test_command / (test_name + '.test'))
    test(
        test_command + ' ' + test_name,
        metang_runtests,
        args: [test_command, test_name],
    )
endforeach
