import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

Button {
    id: customImageButton

    property alias bgColor: backgroundRectangle.color
    property alias imageColor: customImageButtonOverlay.color
    property alias imageSource: customImageButtonImage.source

    //signal pressed

    background: Rectangle {
        id: backgroundRectangle
    }

    Image {
        id: customImageButtonImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }

    ColorOverlay {
        id: customImageButtonOverlay
        anchors.fill: customImageButtonImage
        source: customImageButtonImage
    }

    //onPressed: customImageButton.pressed()
}