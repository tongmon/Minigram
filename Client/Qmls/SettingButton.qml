import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    id: settingButton
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    Popup {
        id: settingPopup
        x: (applicationWindow.width - width) / 2 - settingButton.x
        y: (applicationWindow.height - height) / 2 - settingButton.y
        width: 500
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        contentItem: Rectangle {
            anchors.fill: parent

            Button {
                id: settingSaveButton
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: "Save"

                background: Rectangle {
                    color: "transparent"
                }
            }

            Button {
                id: settingCancleButton
                anchors.right: settingSaveButton.left
                anchors.bottom: parent.bottom
                text: "Cancle"

                background: Rectangle {
                    color: "transparent"
                }

                onClicked: {
                    settingPopup.close()
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            settingPopup.open()
        }
    }
}