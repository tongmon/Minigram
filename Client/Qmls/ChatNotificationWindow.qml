import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: chatNotificationWindow
    flags: Qt.WindowStaysOnTopHint
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height
    opacity: 1
    
    property var sessionId: ""
    property var senderImgPath: ""
    property var senderName: ""
    property var content: ""

    function closeNotification()
    {
        chatNotificationWindow.close()
    }

    function showNotification()
    {
        notificationTimer.start()
        chatNotificationWindow.show()
    }

    SequentialAnimation {
        id: fadeOutAnim

        NumberAnimation {
            target: chatNotificationWindow
            property: "opacity"
            to: 0
            duration: 500
        }
    }

    Timer {
        id: notificationTimer
        interval: 6000
        onTriggered: {
            fadeOutAnim.running = true
        }
    }

    Rectangle {
        id: chatNotificationRect
        anchors.fill: parent
        color: "blue"

        CustomImageButton {
            id: senderImg
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                margins: 15
            }
            width: height
            rounded: true
            source: "file:///" + senderImgPath
        }

        Item {
            anchors {
                left: senderImg.right
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }

            Item {
                anchors {
                    top: parent.top
                }
                width: parent.width
                height: parent.height * 0.5

                Text {
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: 15
                        verticalCenter: parent.verticalCenter
                    }
                    font {
                        pointSize: 12
                        bold: true
                    }
                    text: senderName
                    clip: true
                }
            }

            Item {
                anchors {
                    bottom: parent.bottom
                }
                width: parent.width
                height: parent.height * 0.5

                Text {
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: 15
                        verticalCenter: parent.verticalCenter
                    }
                    font {
                        pointSize: 12
                    }
                    text: content
                    clip: true
                }
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                chatNotificationManager.processClickedNotification(sessionId)
            }
        }

        Component.onCompleted: {
        }
    }

    onOpacityChanged: {
        if (!opacity)
            chatNotificationManager.pop()
    }

    Component.onCompleted: {
    }
}