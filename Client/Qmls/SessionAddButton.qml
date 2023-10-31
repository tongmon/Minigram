import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    Dialog {
        id: sessionAddDialog
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        implicitWidth: 500
        implicitHeight: 300
        title: "Make group name"
        modal: true
                                
        
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            sessionAddDialog.open()
        }
    }
}