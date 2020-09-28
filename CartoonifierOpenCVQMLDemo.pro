# allows to add DEPLOYMENTFOLDERS and links to the Felgo library and QtCreator auto-completion
CONFIG += felgo

QT += multimedia-private

# Project identifier and version
# More information: https://felgo.com/doc/felgo-publishing/#project-configuration
PRODUCT_IDENTIFIER = com.bunistack.CartoonifierOpenCVQMLDemo
PRODUCT_VERSION_NAME = 1.0.0
PRODUCT_VERSION_CODE = 1

qmlFolder.source = qml
#DEPLOYMENTFOLDERS += qmlFolder # comment for publishing

assetsFolder.source = assets
#DEPLOYMENTFOLDERS += assetsFolder

RESOURCES += resources.qrc

SOURCES += main.cpp \
    cartoonifier.cpp \
    cnfilter.cpp \
    cnvideo.cpp
# Uncomment this if you choose to use the pre-complied OpenCV binaries provided with this tutorial
# INCLUDEPATH += C:/opencv/build/include

INCLUDEPATH += C:/opencv/build/opencv-4.4.0/install/include

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    OTHER_FILES += android/AndroidManifest.xml       android/build.gradle

    equals(ANDROID_TARGET_ARCH,armeabi-v7a) {

#        Uncomment this if you choose to use the pre-complied OpenCV binaries provided with this tutorial
#        LIBS += -LC:/opencv/build/android/arch-arm/install/sdk/native/staticlibs/armeabi-v7a \
#                -LC:/opencv/build/android/arch-arm/install/sdk/native/3rdparty/libs/armeabi-v7a

        LIBS += -LC:/opencv/build/opencv-4.4.0-armeabi-v7a/install/sdk/native/staticlibs/armeabi-v7a \
                -LC:/opencv/build/opencv-4.4.0-armeabi-v7a/install/sdk/native/3rdparty/libs/armeabi-v7a

        LIBS += -lopencv_imgproc \
                -lopencv_objdetect \
                -lopencv_imgcodecs \
                -lopencv_core \
                -lcpufeatures \
                -lIlmImf \
                -llibjasper \
                -llibjpeg-turbo \
                -llibpng \
                -llibprotobuf \
                -llibtiff \
                -llibwebp \
                -lquirc \
                -ltegra_hal


    }
}

ios {
    QMAKE_INFO_PLIST = ios/Project-Info.plist
    OTHER_FILES += $$QMAKE_INFO_PLIST
}

# set application icons for win and macx
win32 {
    RC_FILE += win/app_icon.rc

#   Uncomment this if you choose to use the pre-complied OpenCV binaries provided with this tutorial
#   LIBS += -LC:/opencv/build/x86/mingw/lib
#    LIBS +=  -lopencv_core410 \
#             -lopencv_imgproc410 \
#             -lopencv_objdetect410 \
#             -lopencv_imgcodecs410

    LIBS += -LC:/opencv/build/opencv-4.4.0/lib

    LIBS +=  -lopencv_core440 \
             -lopencv_imgproc440 \
             -lopencv_objdetect440 \
             -lopencv_imgcodecs440
}
macx {
    ICON = macx/app_icon.icns
}

HEADERS += \
    cartoonifier.h \
    cnfilter.h \
    cnvideo.h
