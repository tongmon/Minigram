import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: applicationWindow
    visible: true
    x: 100
    y: 100
    width: 1024
    height: 768
    minimumWidth: 400
    minimumHeight: 300
    title: qsTr("Messenger")
    flags: Qt.Window
    
    // 창 크기 재조절 외각 부분, C++에서 받아서 처리
    property int resizeBorderWidth: 6

    property string userID
    property string userPW

    //Dialog {
    //    id: settingDialog
    //    x: (applicationWindow.width - width) / 2
    //    y: (applicationWindow.height - height) / 2
    //    implicitWidth: 500
    //    implicitHeight: 300
    //    title: "Setting"
    //    modal: true
//
    //    Button {
    //        id: settingSaveButton
    //        anchors.right: parent.right
    //        anchors.bottom: parent.bottom
    //        text: "Save"
//
    //        background: Rectangle {
    //            color: "transparent"
    //        }
    //    }
//
    //    Button {
    //        id: settingCancleButton
    //        anchors.right: settingSaveButton.left
    //        anchors.bottom: parent.bottom
    //        text: "Cancle"
//
    //        background: Rectangle {
    //            color: "transparent"
    //        }
//
    //        onClicked: {
    //            settingDialog.close()
    //        }
    //    }
    //}

    //Dialog {
    //    id: contactDialog
    //    x: (applicationWindow.width - width) / 2
    //    y: (applicationWindow.height - height) / 2
    //    implicitWidth: 500
    //    implicitHeight: 300
    //    title: "Contact"
    //    modal: true
//
    //    Button {
    //        id: contactAddButton
    //        anchors.left: parent.left
    //        anchors.bottom: parent.bottom
    //        text: "Add Contact"
//
    //        background: Rectangle {
    //            color: "transparent"
    //        }
    //    }
//
    //    Button {
    //        id: contactCloseButton
    //        anchors.right: parent.right
    //        anchors.bottom: parent.bottom
    //        text: "Close"
//
    //        background: Rectangle {
    //            color: "transparent"
    //        }
//
    //        onClicked: {
    //            settingDialog.close()
    //        }
    //    }
    //}

    // 메인 뷰
    Loader { 
        id: mainWindowLoader 
        anchors.fill: parent
        objectName: "mainWindowLoader"
        source: "qrc:/qml/MainPage.qml"
    }
}
