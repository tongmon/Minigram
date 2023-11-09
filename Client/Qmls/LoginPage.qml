import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.0

Rectangle {
    color: "#280a3d"
    objectName: "loginPage"

    Popup {
        id: registerPopup
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        width: 400
        height: 500
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: Rectangle {
            anchors.fill: parent
            
            ColumnLayout {
                anchors.fill: parent

                Text {
                    text: "Create Account"
                }

                Image {
                    Layout.preferredHeight: 50
                    Layout.preferredWidth: height
                    Layout.alignment: Qt.AlignHCenter
                    id: registerImage
                    source: "file:/C:/Users/DP91-HSK/Pictures/Saved Pictures/profile.png"
                    fillMode: Image.PreserveAspectFit

                    property bool rounded: true
                    property bool adapt: true

                    layer.enabled: rounded
                    layer.effect: OpacityMask {
                        maskSource: Item {
                            width: registerImage.width
                            height: registerImage.height
                            Rectangle {
                                anchors.centerIn: parent
                                width: registerImage.adapt ? registerImage.width : Math.min(registerImage.width, registerImage.height)
                                height: registerImage.adapt ? registerImage.height : width
                                radius: Math.min(width, height)
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                    }
                }

                CustomTextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: registerPopup.height  * 0.1
                    Layout.margins: 5
                    id: registerIDField
                    radius: 5
                    color: "#cccccc"
                    placeholderText: "Put your id..."

                    onReturnPressed: {
                        
                    }
                }

                CustomTextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: registerPopup.height  * 0.1
                    Layout.margins: 5
                    id: registerNameField
                    radius: 5
                    color: "#cccccc"
                    placeholderText: "Put your name..."

                    onReturnPressed: {
                        
                    }
                }

                CustomTextField {
                    Layout.fillWidth: true
                    Layout.preferredHeight: registerPopup.height  * 0.1
                    Layout.margins: 5
                    id: registerPasswordField
                    radius: 5
                    color: "#cccccc"
                    placeholderText: "Put your password..."
                    echoMode: TextField.Password
                    passwordCharacter: "●"
                    
                    onReturnPressed: {
                        
                    }
                }

                Button {
                    text: "Sign Up"

                    onClicked: {
                        mainContext.trySignUp({
                            "id": registerIDField.text,
                            "pw": registerPasswordField.text,
                            "name": registerNameField.text,
                            "img_path": registerImage.source
                        })
                    }
                }
            }
        }
    }

    Popup {
        id: registerSuccessPopup
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        width: 300
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: Rectangle {
            anchors.fill: parent
            
            ColumnLayout {
                anchors.fill: parent    

                Text {
                    text: "Register Success!"
                }    

                Button {
                    text: "Confirm"

                    onClicked: {
                        registerSuccessPopup.close()
                    }
                }
            }
        }
    }

    function manageRegisterResult(result)
    {
        switch(result) 
        {
        // 가입 성공
        case 1:
            registerPopup.close()
            registerSuccessPopup.open()
            break;
        case 2:
            break;
        case 3:
            break;
        default:
            break;
        }
    }

    // 비밀번호, 아이디 유효성 검사
    function checkLoginValidation()
    {
        if(4 < userIDTextField.text.length && userIDTextField.text.length < 32 &&
           7 < passwordTextField.text.length && passwordTextField.text.length < 64)
            return true
        return false
    }

    // 로그인 성공시 화면 전환
    function successLogin()
    {
        userID = userIDTextField.text
        userPW = passwordTextField.text
        mainWindowLoader.source = "qrc:/qml/MainPage.qml"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Layout.alignment: Qt.AlignTop
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 35
            text: qsTr("User Login")
        }

        Item {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 35
            Layout.preferredWidth: 275
            radius: 10

            Image {
                id: userIDImage
                anchors {
                    left: parent.left              
                    top: parent.top
                    bottom: parent.bottom
                }
                width: parent.height
                source: "qrc:/icon/UserID.png" // from https://www.pngwing.com/
            }

            TextField {
                id: userIDTextField
                placeholderText: "User ID"
                selectByMouse: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                
                // 정규식도 checkLoginValidation() 함수에서 처리하는 것이 바람직해보임
                // validator: RegularExpressionValidator { 
                //     regularExpression: /[0-9a-zA-Z]+/
                // }

                anchors {
                    left: userIDImage.right                
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                }

                Keys.onReturnPressed: {
                    if (checkLoginValidation())
                        mainContext.tryLogin(userIDTextField.text, passwordTextField.text)
                }

                background: Rectangle {
                    color: "transparent"
                }
            }
        }

        Item {
            Layout.preferredHeight: 10
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 35
            Layout.preferredWidth: 275
            radius: 10

            Image {
                id: passwordImage
                anchors {
                    left: parent.left              
                    top: parent.top
                    bottom: parent.bottom
                }
                width: parent.height
                source: "qrc:/icon/Password.png"
            }

            TextField {
                id: passwordTextField
                placeholderText: "Password"
                selectByMouse: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                echoMode: TextField.Password // echoMode: showText ? TextField.Normal : TextField.Password
                passwordCharacter: "●"
                
                anchors {
                    left: passwordImage.right                
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                }

                Keys.onReturnPressed: {
                    if (checkLoginValidation())
                        mainContext.tryLogin(userIDTextField.text, passwordTextField.text)
                }

                background: Rectangle {
                    color: "transparent"
                }
            }
        }

        Item {
            Layout.preferredHeight: 10
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 35
            Layout.preferredWidth: 275
            color: "transparent"

            CheckBox {
                anchors.left: parent.left 
                text: "<font color=\"white\">Remember</font>"
            }

            Button {
                id: registerButton
                anchors.right: parent.right
                text: "Register"

                onClicked: {
                    registerPopup.open()
                }
            }
        }

        Item {
            Layout.preferredHeight: 20
            Layout.fillWidth: true
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Login")

            onClicked: {
                if (checkLoginValidation())
                    mainContext.tryLogin(userIDTextField.text, passwordTextField.text)
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }

    Component.onCompleted: {
        console.log("Login Page Start!")
    }

    Component.onDestruction: {
        console.log("Login Page End!")
    }
}