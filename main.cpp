#include <QApplication>
#include <FelgoApplication>
#include <QQmlApplicationEngine>
#include <QSplashScreen>

#include "cnfilter.h"
#include "cnvideo.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    FelgoApplication felgo;

    QQmlApplicationEngine engine;
    felgo.initialize(&engine);

    CNVideo::registerQMLType();
    CNFilter::registerQMLType();

    felgo.setMainQmlFileName(QStringLiteral("qrc:/qml/Main.qml"));
    engine.load(QUrl(felgo.mainQmlFileName()));

    return app.exec();
}
