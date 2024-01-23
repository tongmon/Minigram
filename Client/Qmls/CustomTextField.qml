import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    id: customTextFieldRectangle
    radius: 5
    color: "#cccccc"

    property alias placeholderText: customTextField.placeholderText
    property alias placeholderTextColor: customTextField.placeholderTextColor
    property alias echoMode: customTextField.echoMode
    property alias passwordCharacter: customTextField.passwordCharacter
    property alias text: customTextField.text

    signal returnPressed

    TextField {
        id: customTextField
        anchors.fill: parent
        selectByMouse: true
        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   
        placeholderText: "Put something..."

        background: Rectangle {
            color: "transparent"
        }

        Keys.onReturnPressed: customTextFieldRectangle.returnPressed()

        Component.onCompleted: {
        }
    }                    
}