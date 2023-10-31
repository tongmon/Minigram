import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"
                            
    Dialog {
        id: contactDialog
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        implicitWidth: 500
        implicitHeight: 300
        title: "Contact"
        modal: true

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
                contactDialog.close()
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            contactDialog.open()
        }
    }
}