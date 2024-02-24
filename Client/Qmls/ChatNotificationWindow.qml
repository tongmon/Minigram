import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: chatNotificationWindow
    flags: Qt.FramelessWindowHint
    width: 400
    height: 200
    x: 0
    y: 0

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
            interval: 1000

            onTriggered: {
                state = "hide"
            }
        }

        onOpacityChanged: {
            if (!opacity && state === "hide")
                chatNotificationWindow.close()
        }

        Component.onCompleted: {
            notificationTimer.start()
        }
    }

    Component.onCompleted: {
    }
}