#include "cartoonifier.h"

Cartoonifier::Cartoonifier(QObject *parent) : QObject(parent)
{
    loadCascadeClassifier();
}

QImage Cartoonifier::cartoonify(QImage inputImage, Mode mode)
{    
    Mat inputFrame, outputFrame;
    inputFrame = fromQImageToMat(inputImage);

    // Since Laplacian filters use grayscale images, we must convert from OpenCV's
    // default BGR format to Grayscale.
    Mat gray;
    cvtColor(inputFrame, gray, COLOR_BGR2GRAY);

    // We will use a Median filter because it is good at removing noise while keeping edges sharp; also, it is not as
    // slow as a bilateral filter.
    const int MEDIAN_BLUR_FILTER_SIZE = 7;
    medianBlur(gray, gray, MEDIAN_BLUR_FILTER_SIZE);

    Mat mask, edges, edges2;

    if(mode == ScaryCartoon){

        // Instead of following it with a Laplacian filter and Binary threshold, we can get a scarier look if we apply a 3 x 3
        // Scharr gradient filter along x and y (the second image in the figure), and then apply a binary threshold with a very low
        // cutoff (the third image in the figure) and a 3 x 3 Median blur, producing the final "evil" mask

        Scharr(inputFrame, edges, CV_8U, 1, 0);
        Scharr(inputFrame, edges2, CV_8U, 1, 0, -1);
        edges += edges2; // Combine the x & y edges together.
        const int EVIL_EDGE_THRESHOLD = 12;
        threshold(edges, mask, EVIL_EDGE_THRESHOLD, 255, THRESH_BINARY_INV);
        medianBlur(mask, mask, 3);

        removePepperNoise(mask);

    }else {

        const int LAPLACIAN_FILTER_SIZE = 5;
        Laplacian(gray, edges, CV_8U, LAPLACIAN_FILTER_SIZE);

        // The Laplacian filter produces edges with varying brightness, so to make the edges look more like a
        // sketch we apply a binary threshold to make the edges either white or black:
        const int EDGES_THRESHOLD = 80;
        threshold(edges, mask, EDGES_THRESHOLD, 255, THRESH_BINARY_INV);

        removePepperNoise(mask);

        if(mode == Sketch)
            return fromMatToQImage(mask, QImage::Format_Grayscale8);

    }

    // A strong bilateral filter smoothes flat regions while keeping edges sharp, and is therefore great as an
    // automatic cartoonifier or painting filter, except that it is extremely slow (that is, measured in seconds or
    // even minutes rather than milliseconds!). We will therefore use some tricks to obtain a nice cartoonifier
    // that still runs at an acceptable speed. The most important trick we can use is to perform bilateral
    // filtering at a lower resolution. It will have a similar effect as at full resolution, but will run much faster.
    // Let's reduce the total number of pixels by a factor of four (for example, half width and half height):
    Size size = inputFrame.size();
    Size smallSize;
    smallSize.width = size.width/2;
    smallSize.height = size.height/2;
    Mat smallImg = Mat(smallSize, CV_8UC3);
    resize(inputFrame, smallImg, smallSize, 0,0, INTER_LINEAR);

    // Rather than applying a large bilateral filter, we will apply many small bilateral filters to produce a
    // strong cartoon effect in less time.
    //
    // We have four parameters that control the bilateral filter: color strength, positional strength, size, and
    // repetition count. We need a temp Mat since bilateralFilter() can't overwrite its input (referred to as
    // "in-place processing"), but we can apply one filter storing a temp Mat and another filter storing back to
    // the input:
    Mat tmp = Mat(smallSize, CV_8UC3);
    int repetitions = 7; // Repetitions for strong cartoon effect.

    for (int i=0; i<repetitions; i++) {
        int ksize = 9; // Filter size. Has a large effect on speed.
        double sigmaColor = 9; // Filter color strength.
        double sigmaSpace = 7; // Spatial strength. Affects speed.
        bilateralFilter(smallImg, tmp, ksize, sigmaColor, sigmaSpace);
        bilateralFilter(tmp, smallImg, ksize, sigmaColor, sigmaSpace);
    }

    // Remember that this was applied to the shrunken image, so we need to expand the image back to the
    // original size.
    Mat bigImg;
    resize(smallImg, bigImg, size, 0,0, INTER_LINEAR);

    if(mode == Painting)
        return fromMatToQImage(bigImg);

    // Then we can overlay the edge mask that we found earlier. To overlay the edge mask
    // "sketch" onto the bilateral filter "painting" (left-hand side of the following figure), we can start with a
    // black background and copy the "painting" pixels that aren't edges in the "sketch" mask
    outputFrame.setTo(0);
    bigImg.copyTo(outputFrame, mask);

    if(mode == AlienCartoon){

        //detect face
        vector<cv::Rect> detected = detectFace(gray);

        //qDebug() << "detected: " << detected.size();

        if(detected.size() > 0){

            loadAlienMask();

            if(alienImage.data){

                Mat resizedAlienMask;
                Mat resizedAlienImage;
                Mat faceROI;

                outputFrame.convertTo(outputFrame,CV_8UC4);

                for(size_t i=0; i<detected.size(); i++){
                    faceROI = outputFrame(detected[i]);
                    resize(alienImage, resizedAlienImage, Size(faceROI.cols, faceROI.rows));
                    resize(alienMask, resizedAlienMask, Size(faceROI.cols, faceROI.rows));
                    resizedAlienImage.copyTo(faceROI,resizedAlienMask);
                }

            }

        }

    }

    return fromMatToQImage(outputFrame);

}

void Cartoonifier::loadCascadeClassifier()
{
    QFile xml(":/assets/classifiers/haarcascade_frontalface_default.xml");

    if(xml.open(QFile::ReadOnly | QFile::Text))
    {
        QTemporaryFile temp;
        if(temp.open())
        {
            temp.write(xml.readAll());
            temp.close();
            if(classifier.load(temp.fileName().toStdString()))
            {
                qDebug() << "Successfully loaded classifier!";
            }
            else
            {
                qDebug() << "Could not load classifier.";
            }
        }
        else
        {
            qDebug() << "Can't open temp file.";
        }
    }
    else
    {
        qDebug() << "Can't open XML.";
    }
}

void Cartoonifier::loadAlienMask()
{

    if(!alienImage.data){

        QFile png(":/assets/images/icons/alien.png");

        if(png.open(QFile::ReadOnly))
        {
            QTemporaryFile temp;
            if(temp.open())
            {
                temp.write(png.readAll());
                temp.close();

                alienImage = imread(temp.fileName().toStdString(), IMREAD_UNCHANGED);
                cvtColor(alienImage,alienImage,COLOR_RGBA2BGRA);

                if(!alienImage.data)
                {
                    qDebug() << "Could not load alien mask";
                }

                split(alienImage, alienImageLayers);
                Mat rgb[3] = {alienImageLayers[0], alienImageLayers[1], alienImageLayers[2]};
                alienMask = alienImageLayers[3];
                merge(rgb, 3, alienImage); // alienImage is no longer transparent
            }
            else
            {
                qDebug() << "Can't open temp file.";
            }
        }
        else
        {
            qDebug() << "Can't open alien mask";
        }

    }

}

vector<Rect> Cartoonifier::detectFace(Mat mat)
{
    double imageWidth = mat.cols;
    double imageHeight = mat.rows;

    double resizedWidth = 320;
    double resizedHeight = (imageHeight/imageWidth) * resizedWidth;

    cv::resize(mat, mat, cv::Size((int)resizedWidth, (int)resizedHeight));

    equalizeHist(mat, mat);

    vector<cv::Rect> detected;

    classifier.detectMultiScale(mat, detected, 1.3, 10);

    for(size_t i=0; i<detected.size(); i++){
        detected[i].x = (int)(((double)detected[i].x / resizedWidth) * imageWidth);
        detected[i].y = (int)(((double)detected[i].y / resizedHeight) * imageHeight);
        detected[i].width = (int)(((double)detected[i].width / resizedWidth) * imageWidth);
        detected[i].height = (int)(((double)detected[i].height / resizedHeight) * imageHeight);
    }

    return detected;
}

void Cartoonifier::removePepperNoise(Mat &mask)
{
    for (int y=2; y<mask.rows-2; y++) {

        // Get access to each of the 5 rows near this pixel.
        uchar *pUp2 = mask.ptr(y-2);
        uchar *pUp1 = mask.ptr(y-1);
        uchar *pThis = mask.ptr(y);
        uchar *pDown1 = mask.ptr(y+1);
        uchar *pDown2 = mask.ptr(y+2);

        // Skip the first (and last) 2 pixels on each row.
        pThis += 2;
        pUp1 += 2;
        pUp2 += 2;
        pDown1 += 2;
        pDown2 += 2;

        for (auto x=2; x<mask.cols-2; x++) {

            uchar value = *pThis; // Get pixel value (0 or 255).

            // Check if it's a black pixel surrounded bywhite
            // pixels (ie: whether it is an "island" of black).
            if (value == 0) {
                bool above, left, below, right, surroundings;
                above = *(pUp2 - 2) && *(pUp2 - 1) && *(pUp2) && *(pUp2 + 1)
                        && *(pUp2 + 2);
                left = *(pUp1 - 2) && *(pThis - 2) && *(pDown1 - 2);
                below = *(pDown2 - 2) && *(pDown2 - 1) && (pDown2) &&
                        (pDown2 + 1) && *(pDown2 + 2);
                right = *(pUp1 + 2) && *(pThis + 2) && *(pDown1 + 2);
                surroundings = above && left && below && right;
                if (surroundings == true) {
                    // Fill the whole 5x5 block as white. Since we
                    // knowthe 5x5 borders are already white, we just
                    // need tofill the 3x3 inner region.
                    *(pUp1 - 1) = 255;
                    *(pUp1 + 0) = 255;
                    *(pUp1 + 1) = 255;
                    *(pThis - 1) = 255;
                    *(pThis + 0) = 255;
                    *(pThis + 1) = 255;
                    *(pDown1 - 1) = 255;
                    *(pDown1 + 0) = 255;
                    *(pDown1 + 1) = 255;
                    // Since we just covered the whole 5x5 block with
                    // white, we know the next 2 pixels won't be
                    // black,so skip the next 2 pixels on the right.
                    pThis += 2;
                    pUp1 += 2;
                    pUp2 += 2;
                    pDown1 += 2;
                    pDown2 += 2;
                }
            }

            // Move to the next pixel on the right.
            pThis++;
            pUp1++;
            pUp2++;
            pDown1++;
            pDown2++;
        }
    }
}

Mat Cartoonifier::fromQImageToMat(QImage image)
{
    image = image.convertToFormat(QImage::Format_RGB888);
    return cv::Mat(image.height(),
                   image.width(),
                   CV_8UC3,
                   image.bits(),
                   image.bytesPerLine()).clone();
}

QImage Cartoonifier::fromMatToQImage(Mat mat, QImage::Format format)
{
    return QImage(mat.data,
                  mat.cols,
                  mat.rows,
                  mat.step,
                  format).copy();
}
