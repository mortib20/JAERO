#-------------------------------------------------
#
# Project created by QtCreator 2015-08-01T20:41:23
# Jonti Olds
#
#-------------------------------------------------

#the three tricky things will be libvorbis,libogg, and libcorrect
#the settings I have here are for my setup but other most likely will install the libraries
#so you will need to point everything to the right place.
#if you are having trubbles focus on things like "LIBS += -L$$OGG_PATH/src/.libs"
#remember to compile libvorbis,libogg, and libcorrect before compiling this

DEFINES += JAERO_VERSION=\\\"v1.0.4.14\\\"

QT       += multimedia core network gui svg sql qmqtt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  printsupport

TARGET = JAERO
TEMPLATE = app

INSTALL_PATH = /opt/jaero
JFFT_PATH = ../../JFFT/

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$JFFT_PATH

#libacars-2 support
#this is a bit of a mess to get working.
#he put them in libacars-2/libacars but references in libacars-2/libacars only use libacars in paths
#so i have to find where libacars-2 is and add that to the INCLUDEPATH
for(var, $$list($${QMAKE_DEFAULT_INCDIRS})) {
    new_inc_path_posibility=$$files($${var}/libacars-2, false)
    !equals(new_inc_path_posibility,"") {
        #message( "libacars-2 location is "$${new_inc_path_posibility} )
        INCLUDEPATH += $$new_inc_path_posibility
    }
}

DEFINES += _USE_MATH_DEFINES

#for unit tests
CI {
  DEFINES += GENERATE_TEST_OUTPUT_FILES
  SOURCES += \
    tests/testall.cpp \
    tests/fftwrapper_tests.cpp \
    tests/fftrwrapper_tests.cpp \
    tests/jfastfir_tests.cpp \
    tests/jfastfir_data_input.cpp \
    tests/jfastfir_data_expected_output.cpp

  HEADERS += \
    tests/jfastfir_data.h

  LIBS += -lCppUTest

  DEFINES+= MATLAB_PATH=\\\"$${PWD}/matlab/\\\"
  DEFINES+= TEST_OUTPUT_PATH=\\\"$${PWD}/test_output/\\\"

} else {
  SOURCES += \
        main.cpp
}

SOURCES += mainwindow.cpp \
    coarsefreqestimate.cpp \
    DSP.cpp \
    gui_classes/createeditinputdialog.cpp \
    mskdemodulator.cpp \
    audiomskdemodulator.cpp \
    gui_classes/console.cpp \
    gui_classes/qscatterplot.cpp \
    gui_classes/qspectrumdisplay.cpp \
    gui_classes/qled.cpp \
    fftwrapper.cpp \
    fftrwrapper.cpp \
    gui_classes/textinputwidget.cpp \
    gui_classes/settingsdialog.cpp \
    aerol.cpp \
    gui_classes/planelog.cpp \
    databasetext.cpp \
    oqpskdemodulator.cpp \
    audiooqpskdemodulator.cpp \
    burstoqpskdemodulator.cpp \
    audioburstoqpskdemodulator.cpp \
    arincparse.cpp \
    tcpserver.cpp \
    sbs1.cpp \
    tcpclient.cpp \
    burstmskdemodulator.cpp \
    audioburstmskdemodulator.cpp \
    jconvolutionalcodec.cpp \
    audiooutdevice.cpp \
    compressedaudiodiskwriter.cpp \
    $$JFFT_PATH/jfft.cpp \
    util/stdio_utils.cpp \
    util/file_utils.cpp \
    util/RuntimeError.cpp \
    zmq_audiosender.cpp \
    zmq_audioreceiver.cpp \
    mqttsubscriber.cpp \
    acarsitem_converter.cpp \
    jserialize.cpp \
    qcustomplot.cpp


HEADERS  += mainwindow.h \
    coarsefreqestimate.h \
    DSP.h \
    gui_classes/createeditinputdialog.h \
    mskdemodulator.h \
    audiomskdemodulator.h \
    gui_classes/console.h \
    gui_classes/qscatterplot.h \
    gui_classes/qspectrumdisplay.h \
    gui_classes/qled.h \
    fftwrapper.h \
    fftrwrapper.h \
    gui_classes/textinputwidget.h \
    gui_classes/settingsdialog.h \
    aerol.h \
    gui_classes/planelog.h \
    databasetext.h \
    oqpskdemodulator.h \
    audiooqpskdemodulator.h \
    burstoqpskdemodulator.h \
    audioburstoqpskdemodulator.h \
    arincparse.h \
    tcpserver.h \
    sbs1.h \
    tcpclient.h \
    burstmskdemodulator.h \
    audioburstmskdemodulator.h \
    jconvolutionalcodec.h \
    audiooutdevice.h \
    compressedaudiodiskwriter.h \
    $$JFFT_PATH/jfft.h \
    util/stdio_utils.h \
    util/file_utils.h \
    util/RuntimeError.h \
    zmq_audioreceiver.h \
    zmq_audiosender.h \
    mqttsubscriber.h \
    acarsitem_converter.h \
    jserialize.h \
    qcustomplot.h

# Tell the qcustomplot header that it will be used as library:
DEFINES += QCUSTOMPLOT_USE_LIBRARY
#qcustom plot is called different names on different systems
win32 {
#message("windows")
LIBS += -lqcustomplot2 -llibzmq
} else {
#message("not windows")
LIBS += -lzmq
}

FORMS    += mainwindow.ui \
    gui_classes/createeditinputdialog.ui \
    gui_classes/settingsdialog.ui \
    gui_classes/planelog.ui

RESOURCES += \
    jaero.qrc

DISTFILES += \
    LICENSE \
    ../README.md \
    ../images/screenshot-win-main.png \
    ../images/screenshot-win-planelog.png

win32 {
RC_FILE = jaero.rc

}

# remove possible other optimization flags
#QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

# add the desired -O3 if not present
#QMAKE_CXXFLAGS_RELEASE *= -O3

#for static building order seems to matter
LIBS += -lcorrect -lvorbis -lvorbisenc -logg -lacars-2

#desktop
desktop.path = /usr/share/applications
desktop.files += JAERO.desktop
INSTALLS += desktop

#icon
icon.path = $$INSTALL_PATH
icon.files += jaero.ico
INSTALLS += icon

#install sounds
soundsDataFiles.path = $$INSTALL_PATH/sounds/
soundsDataFiles.files = sounds/*.*
INSTALLS += soundsDataFiles

#install library
target.path=$$INSTALL_PATH
INSTALLS += target

# disable stupid deprecated-copy warnings
# cluttering up issues
QMAKE_CXXFLAGS += '-Wno-deprecated-copy'

#QMAKE_CXXFLAGS += '-Werror=format-security'
