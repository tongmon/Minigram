import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15

Item {
    id: loadingPage
    objectName: "loadingPage"

    TitleBar {
        id: titleBar
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: 35
        color: "#364450"
    }

    Rectangle {
        color: "#add8e6"
        anchors {
            left: parent.left
            right: parent.right
            top: titleBar.bottom
            bottom: parent.bottom
        }

        BusyIndicator {
            anchors {
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            running: true
            width: 400
            height: 400
        }
    }

    Component.onCompleted: {
        
    }

    Component.onDestruction: {
        
    }
}