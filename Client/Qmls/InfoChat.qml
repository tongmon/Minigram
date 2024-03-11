import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Item {
    width: parent.width
    height: 25

    Rectangle {
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        width: childrenRect.width
        height: parent.height - 6
        color: "white"
        opacity: 0.5

        Text {
            anchors {
                verticalCenter: parent.verticalCenter
            }
            text: content
        }
    }

    Component.onCompleted: {
        console.log("InfoChat Created!")
    }
}