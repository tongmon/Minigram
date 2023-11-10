import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.12

Item {
    id: customImageButton

    property alias overlayColor: buttonImageOverlay.color
    property alias source: buttonImage.source
    property bool down: mouseArea.pressed && hovered
    property bool hovered: {
        if(!mouseArea.containsMouse)
            return false

        if(!rounded)    
            return true
        
        var x1 = width / 2
        var y1 = height / 2
        var x2 = mouseArea.mouseX
        var y2 = mouseArea.mouseY
        var distanceFromCenter = Math.pow(x1 - x2, 2) + Math.pow(y1 - y2, 2)
        var radiusSquared = Math.pow(Math.min(width, height) / 2, 2)
        var isWithinOurRadius = distanceFromCenter < radiusSquared

        return isWithinOurRadius;
    }
    property bool rounded: false
    property bool adapt: true
    property bool checkable: false
    property bool checked: false

    signal clicked

    Image {
        id: buttonImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }

    layer {
        enabled: customImageButton.rounded
        effect: OpacityMask {
            maskSource: Item {
                width: buttonImage.width
                height: buttonImage.height
                Rectangle {
                    anchors.centerIn: parent
                    width: customImageButton.adapt ? buttonImage.width : Math.min(buttonImage.width, buttonImage.height)
                    height: customImageButton.adapt ? buttonImage.height : width
                    radius: Math.min(width, height)
                }
            }
        }
    }    

    ColorOverlay {
        id: buttonImageOverlay
        anchors.fill: buttonImage
        source: buttonImage
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if(!customImageButton.hovered)
                return

            if(checkable)
                checked ^= 1

            customImageButton.clicked()
        }
    }
}
