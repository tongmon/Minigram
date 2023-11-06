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
        height: 700
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        contentItem: Rectangle {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent

                ListView {
                    id: contactListView
                    Layout.fillWidth: true 
                    Layout.fillHeight: true
                    clip: true

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: ListModel {
                        id: contactModel
                    }

                    delegate: Rectangle {
                        height: contactPopup.height * 0.15
                        width: parent.width
                        objectName: userID
                        color: "#B240F5"

                        property int personIndex: index

                        RowLayout {
                            anchors.fill: parent

                            Rectangle {
                                id: userImageRect
                                Layout.fillHeight: true
                                Layout.preferredWidth: userImageRect.height
                                color: "transparent"

                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.horizontalCenter: parent.horizontalCenter 
                                    height: parent.height * 0.8
                                    width: parent.width * 0.8
                                    radius: width * 2

                                    Image {
                                        anchors.fill: parent
                                        source: userImageSource
                                        fillMode: Image.PreserveAspectFit
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                color: "transparent"

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    Text {
                                        Layout.alignment: Qt.AlignVCenter
                                        clip: true
                                        text: userName
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }

                    Component.onCompleted: {
                        //contactModel.append({
                        //    "userID": "tongstar",
                        //    "userImageSource": "",
                        //    "userName": "@tongstar"
                        //})

                        //contactModel.append({
                        //    "userID": "yellowjam",
                        //    "userImageSource": "",
                        //    "userName": "@yellowjam"
                        //})
                    }
                }

                RowLayout {
                    Rectangle {
                        color: "transparent"        
                        Layout.preferredHeight: contactPopup.height * 0.1
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent

                            Button {
                                id: contactAddButton
                                text: "Add Contact"

                                background: Rectangle {
                                    color: "transparent"
                                }

                                onClicked: {
                                    contactAddPopup.open()
                                }                                
                            }

                            Item {
                                Layout.fillWidth: true                    
                            }

                            Button {
                                id: contactCloseButton
                                text: "Close"

                                background: Rectangle {
                                    color: "transparent"
                                }

                                onClicked: {
                                    contactPopup.close()
                                }
                            }  
                        }
                    }
                } 
            }    
        }
    }         

    Popup {
        id: contactAddPopup
        x: (applicationWindow.width - width) / 2 - contactButton.x
        y: (applicationWindow.height - height) / 2 - contactButton.y
        width: 500
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent        

        contentItem: Rectangle {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: contactAddPopup.height  * 0.2
                    Layout.margins: 5
                    radius: 5
                    color: "#cccccc"

                    TextField {
                        id: userNameTextField
                        anchors.fill: parent
                        selectByMouse: true
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   
                        placeholderText: "Put user name here..."

                        background: Rectangle {
                            color: "transparent"
                        }

                        Keys.onReturnPressed: {
                        
                        }
                        Component.onCompleted: {
                        }
                    }                    
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: contactAddPopup.height  * 0.2
                    Layout.margins: 5
                    radius: 5
                    color: "#cccccc"

                    TextField {
                        id: userIDTextField
                        anchors.fill: parent
                        selectByMouse: true
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   
                        placeholderText: "Put user id here..."

                        background: Rectangle {
                            color: "transparent"
                        }

                        Keys.onReturnPressed: {
                        
                        }
                        Component.onCompleted: {
                        }
                    }                    
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: contactAddPopup.height  * 0.3

                    RowLayout {
                        anchors.fill: parent

                        Item {
                            Layout.fillWidth: true                    
                        }
                        Button {
                            id: closeButton
                            text: "Close"

                            background: Rectangle {
                                color: "transparent"
                            }

                            onClicked: {
                                contactAddPopup.close()
                            }
                        }                          
                        Button {
                            id: confirmButton
                            text: "Confirm"

                            background: Rectangle {
                                color: "transparent"
                            }

                            onClicked: {
                                contactAddPopup.close()
                                contactPopup.close()
                            }
                        }
                    }
                }
            }
        }

        onClosed: {
            userNameTextField.text = userIDTextField.text = ""
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            contactPopup.open()
        }
    }
}