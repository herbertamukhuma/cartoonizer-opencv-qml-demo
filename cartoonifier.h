#ifndef CARTOONIFIER_H
#define CARTOONIFIER_H

#include <QObject>
#include <QImage>
#include <QFile>
#include <QTemporaryFile>
#include <QDebug>

#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

class Cartoonifier : public QObject
{
    Q_OBJECT
public:

    enum Mode {
        Sketch = 0,
        Painting = 1,
        Cartoon = 2,
        ScaryCartoon = 3,
        AlienCartoon = 4
    };

    Q_ENUMS(Mode)

    explicit Cartoonifier(QObject *parent = nullptr);

    QImage cartoonify(QImage inputImage, Mode mode);

signals:

private:
    CascadeClassifier classifier;

    Mat alienImage;
    vector<Mat> alienImageLayers;
    Mat alienMask;

    void loadCascadeClassifier();
    void loadAlienMask();

    vector<cv::Rect> detectFace(Mat mat);

    void removePepperNoise(Mat &mask);
    cv::Mat fromQImageToMat(QImage image);
    QImage fromMatToQImage(Mat mat, QImage::Format format = QImage::Format_RGB888);

};

#endif // CARTOONIFIER_H
