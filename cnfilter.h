#ifndef MYFILTER_H
#define MYFILTER_H

#include <QVideoFilterRunnable>
#include <QDebug>
#include <QQmlEngine>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QOpenGLFunctions>
#include <QOpenGLContext>
#include <QImageWriter>

#include <private/qvideoframe_p.h>
#include <cartoonifier.h>

class CNFilter : public QAbstractVideoFilter {
    Q_OBJECT
    Q_PROPERTY(Cartoonifier::Mode mode MEMBER m_mode NOTIFY modeChanged)
friend class CNFilterRunnable;

public:
    explicit CNFilter(QObject *parent = nullptr);
    virtual ~CNFilter();

    QVideoFilterRunnable *createFilterRunnable();

    void static registerQMLType();

signals:
    void cartoonifiedImageDataReady(QString data);
    void modeChanged();

private:
    QVector<QFuture<void>> workerThreads;
    const int WORKER_THREAD_COUNT = 3;
    Cartoonifier *cartoonifier;    
    bool isProcessing = false;

    Cartoonifier::Mode m_mode = Cartoonifier::Cartoon;

    QImage videoFrameToImage(QVideoFrame *frame);
};






class CNFilterRunnable : public QObject, public QVideoFilterRunnable {

public:
    explicit CNFilterRunnable(CNFilter *filter);
    virtual ~CNFilterRunnable();

    QVideoFrame run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, RunFlags flags);   
    void preprocessImage(QImage image);

private:
    CNFilter *filter;    
};


#endif // MYFILTER_H
