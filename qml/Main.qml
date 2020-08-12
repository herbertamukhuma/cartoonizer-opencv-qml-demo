import Felgo 3.0
import QtQuick 2.12
import QtMultimedia 5.12
import CNVideo 1.0
import CNFilter 1.0
import Cartoonifier 1.0

App {
    width: 900
    height: 600

    property int currentModeIndex: 2

    VideoOutput {
        id: output
        source: camera
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        filters: [cnFilter]
        //visible: false
        autoOrientation: true
    }

    Camera {
        id: camera
        position: Camera.FrontFace
        imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash

        exposure {
            exposureCompensation: -1.0
            exposureMode: Camera.ExposurePortrait
        }

    }

    CNFilter{
        id: cnFilter

        onCartoonifiedImageDataReady: {
            cnVideo.updateImage(data);
        }
    }

    Item{
        anchors.fill: parent

        Column{
            anchors.fill: parent

            CNVideo{
                id: cnVideo
                width: parent.width
                height: parent.height - tabBar.height
                fillMode: CNVideo.PreserveAspectCrop
            }

            Rectangle{
                id: tabBar
                width: parent.width
                height: 80

                ListView{
                    id: listView
                    anchors.fill: parent
                    model: modesModel
                    delegate: listViewDelegate
                    orientation: ListView.Horizontal
                }
            }

        }

        Rectangle{
            width: 50
            height: 50
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 100
            radius: width/2
            visible: QtMultimedia.availableCameras.length > 1

            RoundedImage{
                anchors.fill: parent
                anchors.margins: 10
                source: "qrc:/assets/images/icons/switch_camera.png"
            }

            MouseArea{
                anchors.fill: parent
                onClicked: {

                    if(QtMultimedia.availableCameras.length > 1){

                        if(camera.position === Camera.BackFace){
                            camera.position = Camera.FrontFace;
                        }else{
                            camera.position = Camera.BackFace;
                        }
                    }

                }
            }
        }

        //end of item
    }

    Component{
        id: listViewDelegate

        Rectangle{
            width: listView.width/modesModel.count < 100 ? 100 : listView.width/modesModel.count
            height: listView.height
            color: currentModeIndex === index ? "#dcdcdc" : "#ffffff"

            Column{
                anchors.fill: parent
                topPadding: 10
                bottomPadding: 10

                Image {
                    source: modeIcon
                    width: 40
                    height: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: modeName
                    width: parent.width
                    height: 20
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 15
                }
            }

            MouseArea{
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {

                    currentModeIndex = index;

                    switch(index){
                    case 0:
                        cnFilter.mode = Cartoonifier.Sketch;
                        break;
                    case 1:
                        cnFilter.mode = Cartoonifier.Painting;
                        break;
                    case 2:
                        cnFilter.mode = Cartoonifier.Cartoon;
                        break;
                    case 3:
                        cnFilter.mode = Cartoonifier.ScaryCartoon;
                        break;
                    case 4:
                        cnFilter.mode = Cartoonifier.AlienCartoon;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

    }

    ListModel{
        id: modesModel

        ListElement{
            modeName: "Sketch"
            modeIcon: "qrc:/assets/images/icons/sketch.png"
        }

        ListElement{
            modeName: "Painting"
            modeIcon: "qrc:/assets/images/icons/painting.png"
        }

        ListElement{
            modeName: "Cartoon"
            modeIcon: "qrc:/assets/images/icons/cartoon.png"
        }

        ListElement{
            modeName: "Scary"
            modeIcon: "qrc:/assets/images/icons/scary.png"
        }

        ListElement{
            modeName: "Alien"
            modeIcon: "qrc:/assets/images/icons/martian.png"
        }
    }

    //end of root
}
