import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Item {
    width: parent.width
    height: infoBck.height + 5

    Rectangle {
        id: infoBck
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        width: infoText.paintedWidth + 5
        height: infoText.paintedHeight + 5
        color: Qt.rgba(1, 1, 1, 0.5)
        radius: 5

        Text {
            id: infoText
            anchors {
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            text: content
            font {
                pointSize: 11
            }
        }
    }

    // Component.onCompleted: {
    //     console.log("InfoChat Created!")
    // }
}