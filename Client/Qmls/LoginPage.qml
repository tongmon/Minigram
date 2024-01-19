import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15

Item {
    objectName: "loginPage"

    function manageRegisterResult(result)
    {
        switch(result) 
        {
        // 가입 성공
        case 1:
            registerPopup.close()
            registerSuccessPopup.open()
            break
        case 2:
            break
        case 3:
            break
        default:
            break
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

    function processLogin(result)
    {
        // 로그인 성공
        if (result) 
        {
            userID = userIDTextField.text
            userPW = passwordTextField.text
            mainWindowLoader.source = "qrc:/qml/MainPage.qml"
            return
        }

        // 로그인 실패

    }

    component LoginView: Rectangle {
        id: loginView
        anchors.fill: parent
        color: "white"

        Item {
            anchors {
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width
            height: childrenRect.height

            Text {
                id: loginPageTitleText
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                }
                font {
                    bold: true
                    pointSize: 20
                }
                text: "USER LOGIN"
            }

            Rectangle {
                id: userIdInputRect
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: loginPageTitleText.bottom
                    topMargin: 25
                }
                radius: 10
                height: 35
                width: 275
                color: "#cccccc"

                Image {
                    id: userIdImage
                    anchors {
                        left: parent.left              
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: parent.height
                    source: "qrc:/icon/UserID.png"
                }

                TextField {
                    id: userIdTextField
                    placeholderText: "User ID"
                    selectByMouse: true
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                    anchors {
                        left: userIdImage.right
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    background: Rectangle {
                        color: "transparent"
                    }

                    Keys.onReturnPressed: {
                        //if (checkLoginValidation())
                        //    mainContext.tryLogin(userIDTextField.text, passwordTextField.text)
                    }

                    // 정규식도 checkLoginValidation() 함수에서 처리하는 것이 바람직해보임
                    // validator: RegularExpressionValidator { 
                    //     regularExpression: /[0-9a-zA-Z]+/
                    // }

                }
            }

            Rectangle {
                id: userPwInputRect
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: userIdInputRect.bottom
                    topMargin: 10
                }
                radius: 10
                height: 35
                width: 275
                color: "#cccccc"

                Image {
                    id: userPwImage
                    anchors {
                        left: parent.left              
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: parent.height
                    source: "qrc:/icon/UserID.png"
                }

                TextField {
                    id: userPwTextField
                    placeholderText: "User Password"
                    selectByMouse: true
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                    echoMode: TextField.Password // echoMode: showText ? TextField.Normal : TextField.Password
                    passwordCharacter: "●"
                    anchors {
                        left: userPwImage.right
                        right: parent.right
                        top: parent.top
                        bottom: parent.bottom
                    }
                    background: Rectangle {
                        color: "transparent"
                    }

                    Keys.onReturnPressed: {
                        //if (checkLoginValidation())
                        //    mainContext.tryLogin(userIDTextField.text, passwordTextField.text)
                    }

                    // 정규식도 checkLoginValidation() 함수에서 처리하는 것이 바람직해보임
                    // validator: RegularExpressionValidator { 
                    //     regularExpression: /[0-9a-zA-Z]+/
                    // }

                }
            }

            RowLayout {
                id: etcUiContainer
                anchors {
                    left: userPwInputRect.left
                    right: userPwInputRect.right
                    top: userPwInputRect.bottom
                    topMargin: 10
                }
                // Layout.preferredHeight: 50

                CheckBox {
                    text: "<font color=\"grey\">Remember</font>"
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: registerButton
                    text: "Register"
                    background: Rectangle {
                        color: registerButton.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                        radius: 5
                    }

                    onClicked: {
                        userInputView.push(registerView)
                    }
                }
            }

            Button {
                id: loginButton
                text: "Login"
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: etcUiContainer.bottom
                    topMargin: 10
                }
                background: Rectangle {
                    color: loginButton.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                    radius: 5
                }

                onClicked: {

                }
            }
        }
    }

    component RegisterView: Rectangle {
        id: registerView
        anchors.fill: parent
        color: "white"

        Item {
            anchors {
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width
            height: childrenRect.height
        
            Text {
                id: registerPageTitleText
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                }
                font {
                    bold: true
                    pointSize: 20
                }
                text: "REGISTER ACCOUNT"
            }

            CustomTextField {
                id: registerIdField
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: registerPageTitleText.bottom
                    topMargin: 25
                }
                color: "#cccccc"
                placeholderText: "Put your id..."
                radius: 10
                height: 35
                width: 275

                onReturnPressed: {

                }
            }

            CustomTextField {
                id: registerNameField
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: registerIdField.bottom
                    topMargin: 10
                }
                color: "#cccccc"
                placeholderText: "Put your name..."
                radius: 10
                height: 35
                width: 275

                onReturnPressed: {

                }
            }

            CustomTextField {
                id: registerPwField
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: registerNameField.bottom
                    topMargin: 10
                }
                color: "#cccccc"
                placeholderText: "Put your password..."
                radius: 10
                height: 35
                width: 275

                onReturnPressed: {

                }
            }

            RowLayout {
                anchors {
                    left: registerPwField.left
                    right: registerPwField.right
                    top: registerPwField.bottom
                    topMargin: 10
                }

                CustomImageButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: height
                    source: "qrc:/icon/UserID.png"

                    onClicked: {
                        userInputView.pop(null)
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    id: registerConfirmButton
                    text: "Register"
                    background: Rectangle {
                        color: registerConfirmButton.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                        radius: 5
                    }

                    onClicked: {
                        userInputView.push(registerSuccessView)
                    }
                }
            }
        }
    }

    component RegisterSuccessView: Rectangle {
        id: registerSuccessView
        anchors.fill: parent
        color: "white"

        Item {
            anchors {
                verticalCenter: parent.verticalCenter
                horizontalCenter: parent.horizontalCenter
            }
            width: parent.width
            height: childrenRect.height

            Text {
                id: registerSuccessPageTitleText
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                }
                font {
                    bold: true
                    pointSize: 20
                }
                text: "REGISTER SUCCESS!"
            }

            CustomImageButton {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: registerSuccessPageTitleText.bottom
                    topMargin: 10
                }
                width: 40
                height: width
                source: "qrc:/icon/UserID.png"
                rounded: true

                onClicked: {
                    userInputView.pop(null)
                }
            }
        }
    }

    property Component loginView: LoginView {}
    property Component registerView: RegisterView {}
    property Component registerSuccessView: RegisterSuccessView {}

    TitleBar {
        id: titleBar
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: 35
        color: "#364450"
    }

    Item {
        anchors {
            left: parent.left
            right: parent.right
            top: titleBar.bottom
            bottom: parent.bottom
        }
         
        Rectangle {
            id: sideView
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: parent.width * 0.65
            color: "#778899"

            Item {
                anchors.top: parent.top
                width: parent.width
                height: parent.height / 3

                Text {
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        bottom: parent.bottom
                    }
                    font {
                        bold: true
                        pointSize: 35
                    }
                    text: "Welcome to Minigram"
                }
            }

            Item {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                }
                width: parent.width
                height: parent.height * 2 / 3

                Text {
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                        topMargin: 10
                    }
                    font {
                        pointSize: 12
                    }
                    text: "Minigram is lightweight and portable messenger application.\nFeel the unlimited portability freely!"
                }
            }
        }

        StackView {
            id: userInputView
            objectName: "userInputView"
            anchors {
                left: sideView.right
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            clip: true

            Component.onCompleted: {
                userInputView.push(loginView)
            }
        }
    }

    Component.onCompleted: {
        
    }

    Component.onDestruction: {
        
    }
}