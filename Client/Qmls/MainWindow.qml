import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

ApplicationWindow {
    id: applicationWindow
    visible: true
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

    // 메인 뷰
    Loader { 
        id: mainWindowLoader 
        anchors.fill: parent
        objectName: "mainWindowLoader"
        source: "qrc:/qml/MainPage.qml"
    }

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }
}
