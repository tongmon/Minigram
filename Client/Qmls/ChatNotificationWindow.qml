import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: chatNotificationWindow
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    // width: 400
    // height: 200
    // x: 0
    // y: 0
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height

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

    Rectangle {
        anchors.fill: parent
        color: "blue"
        state: "visible"

        states: [
            State {
                name: "visible"
        
                PropertyChanges {
                    target: chatNotificationWindow
                    opacity: 1
                }
            },
            State {
                name: "hide"
        
                PropertyChanges {
                    target: chatNotificationWindow
                    opacity: 0
                }
            }
        ]

        transitions: [
            Transition {
                from: "visible"
                to: "hide"
                PropertyAnimation { 
                    properties: "opacity"
                    duration: 500 
                }
            }
        ]

        Timer {
            id: notificationTimer
            interval: 6000

            onTriggered: {
                // state = "hide"
                chatNotificationManager.pop()
            }
        }

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

        onOpacityChanged: { 
            // 밑에꺼 안먹음...
            //if (!opacity && state == "hide")
            //    console.log("hide")
            //    chatNotificationManager.pop()
        }

        Component.onCompleted: {
            // console.log("timer start!")
            // notificationTimer.start()
        }
    }

    Component.onCompleted: {
    }
}