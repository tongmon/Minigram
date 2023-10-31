import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    Dialog {
        id: settingDialog
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        implicitWidth: 500
        implicitHeight: 300
        title: "Setting"
        modal: true
                                
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
                settingDialog.close()
            }
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            settingDialog.open()
        }
    }
}