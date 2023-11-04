import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    id: contactButton
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    Popup {
        id: contactPopup
        x: (applicationWindow.width - width) / 2 - contactButton.x
        y: (applicationWindow.height - height) / 2 - contactButton.y
        width: 500
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        Button {
            id: contactAddButton
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            text: "Add Contact"

            background: Rectangle {
                color: "transparent"
            }
        }

        Button {
            id: contactCloseButton
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            text: "Close"

            background: Rectangle {
                color: "transparent"
            }

            onClicked: {
                contactPopup.close()
            }
        }                
    }         

    MouseArea {
        anchors.fill: parent

        onClicked: {
            contactPopup.open()
        }
    }
}