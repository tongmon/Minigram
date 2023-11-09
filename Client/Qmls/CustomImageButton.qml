import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.12

Button {
    id: customImageButton

    property alias bgColor: backgroundRectangle.color
    property alias imageColor: customImageButtonOverlay.color
    property alias imageSource: customImageButtonImage.source
    property bool rounded: false
    property bool adapt: true

    //signal pressed

    background: Rectangle {
        id: backgroundRectangle
    }

    Image {
        id: customImageButtonImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit

        layer.enabled: rounded
        layer.effect: OpacityMask {
            maskSource: Item {
                width: customImageButtonImage.width
                height: customImageButtonImage.height

                Rectangle {
                    anchors.centerIn: parent
                    width: customImageButton.adapt ? customImageButtonImage.width : Math.min(customImageButtonImage.width, customImageButtonImage.height)
                    height: customImageButton.adapt ? customImageButtonImage.height : width
                    radius: Math.min(width, height)
                }
            }
        }
    }

    ColorOverlay {
        id: customImageButtonOverlay
        anchors.fill: customImageButtonImage
        source: customImageButtonImage
    }

    //onPressed: customImageButton.pressed()
}