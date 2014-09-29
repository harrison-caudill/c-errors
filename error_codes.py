CODES = [
    ('OK', False, 0, None, 'Everything is fine', ''),
    ('FAIL', True, 0, None, 'Generic Failure', 'This code should be avoided'),
    ('ENOENT', True, 0, None, 'Requested Item is Missing', ''),
    ('UNIMPLEMENTED', True, 0, None, 'Functionality has not been implemented', ''),
    ('EINVAL', True, 0, None, 'Invalid request', ''),
    ('EOF', False, 0, None, 'End of stream/list/file/foo', ''),
    ('UNITTEST', True, 0, None, 'UnitTest failed', 'This error is produced when a specific check of a unit-test fails'),
]
