TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        GenThreadPool.cpp \
        main.cpp

HEADERS += \
    GenCASQueue.h \
    GenThreadPool.h

INCLUDEPATH += D:/Apps/boost_1_80_0 # boost 1.80.0

LIBS += D:/Apps/boost_1_80_0/stage/lib/libboost_json-vc141-mt-gd-x64-1_80.lib
LIBS += D:/Apps/boost_1_80_0/stage/lib/libboost_container-vc141-mt-gd-x64-1_80.lib
LIBS += D:/Apps/boost_1_80_0/stage/lib/libboost_filesystem-vc141-mt-gd-x64-1_80.lib
LIBS += D:/Apps/boost_1_80_0/stage/lib/libboost_thread-vc141-mt-gd-x64-1_80.lib
LIBS += D:/Apps/boost_1_80_0/stage/lib/libboost_chrono-vc141-mt-gd-x64-1_80.lib
