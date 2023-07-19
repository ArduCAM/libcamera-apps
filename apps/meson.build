pthread_dep = dependency('threads')


libcamera_still = executable('libcamera-still', files(['libcamera_still.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                             include_directories : include_directories('..'),
                             dependencies: [libcamera_dep, boost_dep, pthread_dep],
                             link_with : libcamera_app,
                             install : true)

libcamera_vid = executable('libcamera-vid', files(['libcamera_vid.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                           include_directories : include_directories('..'),
                           dependencies: [libcamera_dep, boost_dep, pthread_dep],
                           link_with : libcamera_app,
                           install : true)

libcamera_hello = executable('libcamera-hello', files(['libcamera_hello.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                             include_directories : include_directories('..'),
                             dependencies: [libcamera_dep, boost_dep, pthread_dep],
                             link_with : libcamera_app,
                             install : true)

libcamera_raw = executable('libcamera-raw', files('libcamera_raw.cpp'),
                           include_directories : include_directories('..'),
                           dependencies: [libcamera_dep, boost_dep],
                           link_with : libcamera_app,
                           install : true)

libcamera_jpeg = executable('libcamera-jpeg', files(['libcamera_jpeg.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                            include_directories : include_directories('..'),
                            dependencies: [libcamera_dep, boost_dep, pthread_dep],
                            link_with : libcamera_app,
                            install : true)

if enable_tflite
    libcamera_detect = executable('libcamera-detect', files('libcamera_detect.cpp'),
                                  include_directories : include_directories('..'),
                                  dependencies: [libcamera_dep, boost_dep],
                                  link_with : libcamera_app,
                                  install : true)
endif
