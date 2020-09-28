#include "cnfilter.h"

CNFilter::CNFilter(QObject *parent) : QAbstractVideoFilter(parent)
{    
    workerThreads.resize(WORKER_THREAD_COUNT);
    cartoonifier = new Cartoonifier(this);
}

CNFilter::~CNFilter()
{

    QVectorIterator<QFuture<void>> i(workerThreads);
    QFuture<void> workerThread;
    int counter = 0;

    while(i.hasNext()){

        workerThread = i.next();

        if(!workerThread.isFinished()){
            workerThreads[counter].cancel();
            workerThreads[counter].waitForFinished();
        }

        counter++;
    }

}

QVideoFilterRunnable *CNFilter::createFilterRunnable()
{    
    return new CNFilterRunnable(this);
}

void CNFilter::registerQMLType()
{    
    qmlRegisterType<CNFilter>("CNFilter", 1, 0, "CNFilter");
    //register Cartoonifier to access the different modes
    qmlRegisterType<Cartoonifier>("Cartoonifier", 1, 0, "Cartoonifier");
}

QImage CNFilter::videoFrameToImage(QVideoFrame *frame)
{
    if(frame->handleType() == QAbstractVideoBuffer::NoHandle){

        QImage image = qt_imageFromVideoFrame(*frame);

        if(image.isNull()){
            qDebug() << "-- null image from qt_imageFromVideoFrame";
            return QImage();
        }

        if(image.format() != QImage::Format_RGB32){
            image = image.convertToFormat(QImage::Format_RGB32);
        }

        return image;
    }

    if(frame->handleType() == QAbstractVideoBuffer::GLTextureHandle){
        QImage image(frame->width(), frame->height(), QImage::Format_RGB32);
        GLuint textureId = frame->handle().toUInt();//static_cast<GLuint>(frame.handle().toInt());
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QOpenGLFunctions *f = ctx->functions();
        GLuint fbo;
        f->glGenFramebuffers(1,&fbo);
        GLint prevFbo;
        f->glGetIntegerv(GL_FRAMEBUFFER_BINDING,&prevFbo);
        f->glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        f->glReadPixels(0, 0, frame->width(), frame->height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
        f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        return image.rgbSwapped();
    }

    qDebug() << "-- Invalid image format...";
    return QImage();
}

CNFilterRunnable::CNFilterRunnable(CNFilter *filter) : QObject(nullptr), filter(filter)
{

}

CNFilterRunnable::~CNFilterRunnable()
{
    filter = nullptr;
}

QVideoFrame CNFilterRunnable::run(QVideoFrame *input, const QVideoSurfaceFormat &surfaceFormat, QVideoFilterRunnable::RunFlags flags)
{
    Q_UNUSED(surfaceFormat);
    Q_UNUSED(flags);

    if(!input || !input->isValid()){
        return QVideoFrame();
    }

    QVectorIterator<QFuture<void>> i(filter->workerThreads);

    QFuture<void> availableWorkerThread;
    int unfinishedThreads = 0;
    int counter = 0;

    while(i.hasNext()){

        availableWorkerThread = i.next();

        if(availableWorkerThread.isFinished()){
            break;
        }else{
            unfinishedThreads++;
        }

        counter++;
    }

    if(unfinishedThreads >= filter->WORKER_THREAD_COUNT){
        return * input;
    }    

    QImage image = filter->videoFrameToImage(input);
    filter->workerThreads[counter] = QtConcurrent::run(this, &CNFilterRunnable::preprocessImage, image);

    return * input;
}

void CNFilterRunnable::preprocessImage(QImage image)
{        
    //if android, make image upright
#ifdef Q_OS_ANDROID
    QPoint center = image.rect().center();
    QMatrix matrix;
    matrix.translate(center.x(), center.y());
    matrix.rotate(90);
    image = image.transformed(matrix);
#endif

    double resizedWidth, resizedHeight;

    resizedWidth = 640;
    resizedHeight = ((double)image.height()/(double)image.width()) * resizedWidth;
    image = image.scaled(resizedWidth, resizedHeight, Qt::KeepAspectRatio);

    image = filter->cartoonifier->cartoonify(image, filter->m_mode);

    if(!image.isNull()){
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        QImageWriter writer(&buffer,QByteArray("JPEG"));
        writer.setQuality(50);
        writer.write(image);

        QString data = QString::fromStdString(byteArray.toBase64().toStdString());
        emit filter->cartoonifiedImageDataReady(data);
    }else {
        qWarning() << "Invalid image....";
    }    

}

