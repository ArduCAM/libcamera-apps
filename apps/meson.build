pthread_dep = dependency('threads')

rpicam_still = executable('rpicam-still', files(['rpicam_still.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                          include_directories : include_directories('..'),
                          dependencies: [libcamera_dep, boost_dep, pthread_dep],
                          link_with : rpicam_app,
                          install : true)

rpicam_vid = executable('rpicam-vid', files(['rpicam_vid.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                        include_directories : include_directories('..'),
                        dependencies: [libcamera_dep, boost_dep, pthread_dep],
                        link_with : rpicam_app,
                        install : true)

rpicam_hello = executable('rpicam-hello', files(['rpicam_hello.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                          include_directories : include_directories('..'),
                          dependencies: [libcamera_dep, boost_dep, pthread_dep],
                          link_with : rpicam_app,
                          install : true)

rpicam_raw = executable('rpicam-raw', files('rpicam_raw.cpp'),
                        include_directories : include_directories('..'),
                        dependencies: [libcamera_dep, boost_dep],
                        link_with : rpicam_app,
                        install : true)

rpicam_jpeg = executable('rpicam-jpeg', files(['rpicam_jpeg.cpp', '../SignalServer/Semaphore.hpp', '../SignalServer/SignalServer.cpp']),
                         include_directories : include_directories('..'),
                         dependencies: [libcamera_dep, boost_dep, pthread_dep],
                         link_with : rpicam_app,
                         install : true)

if enable_tflite
    rpicam_detect = executable('rpicam-detect', files('rpicam_detect.cpp'),
                               include_directories : include_directories('..'),
                               dependencies: [libcamera_dep, boost_dep],
                               link_with : rpicam_app,
                               install : true)
endif
