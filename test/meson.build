subdir('libtest')

subdir('camera')
subdir('controls')
subdir('ipa')
subdir('ipc')
subdir('log')
subdir('media_device')
subdir('pipeline')
subdir('process')
subdir('stream')
subdir('v4l2_subdevice')
subdir('v4l2_videodevice')

public_tests = [
    ['geometry',                        'geometry.cpp'],
    ['list-cameras',                    'list-cameras.cpp'],
    ['signal',                          'signal.cpp'],
]

internal_tests = [
    ['camera-sensor',                   'camera-sensor.cpp'],
    ['event',                           'event.cpp'],
    ['event-dispatcher',                'event-dispatcher.cpp'],
    ['event-thread',                    'event-thread.cpp'],
    ['message',                         'message.cpp'],
    ['object',                          'object.cpp'],
    ['object-invoke',                   'object-invoke.cpp'],
    ['signal-threads',                  'signal-threads.cpp'],
    ['threads',                         'threads.cpp'],
    ['timer',                           'timer.cpp'],
    ['timer-thread',                    'timer-thread.cpp'],
]

foreach t : public_tests
    exe = executable(t[0], t[1],
                     dependencies : libcamera_dep,
                     link_with : test_libraries,
                     include_directories : test_includes_public)

    test(t[0], exe)
endforeach

foreach t : internal_tests
    exe = executable(t[0], t[1],
                     dependencies : libcamera_dep,
                     link_with : test_libraries,
                     include_directories : test_includes_internal)

    test(t[0], exe)
endforeach
