import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    color: "#19314F"
    anchors.fill: parent

    Button {
        id: addContactButton
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 5
        }
        height: 35
        text: "Add Contact"
        z: 2
        background: Rectangle {
            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
            radius: 5
        }

        onClicked: {
            contactAddPopup.open()
        }

        Popup {
            id: contactAddPopup
            anchors.centerIn: Overlay.overlay
            width: 500
            height: 300
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            modal: true
            background: Rectangle {
                color: "#333366"
                radius: 5
            }
            contentItem: Item {
                anchors.fill: parent

                Item {
                    id: contactAddPopupTitle
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        topMargin: 20
                    }
                    height: parent.height * 0.1

                    Text {
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                            leftMargin: 15
                        }
                        font {
                            bold: true
                            pointSize: 15
                        }
                        text: "Add New Contact"
                    }
                }

                Item {
                    id: nameInputItemContainer
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: contactAddPopupTitle.bottom
                    }
                    height: parent.height * 0.35

                    CustomImageButton {
                        id: nameInputImage
                        anchors {
                            left: parent.left
                            leftMargin: 5
                            bottom: parent.bottom
                            bottomMargin: 10
                        }
                        height: nameInputTextField.height
                        width: height
                        source: "qrc:/icon/UserID.png"
                    }

                    CustomTextField {
                        id: nameInputTextField
                        anchors {
                            left: nameInputImage.right
                            leftMargin: 5
                            right: parent.right
                            rightMargin: 10
                            bottom: parent.bottom
                            bottomMargin: 10
                        }
                        height: parent.height / 2
                        radius: 5
                        color: "#cccccc"
                        placeholderText: "Put name on here..."

                        onReturnPressed: {
                        
                        }
                    }
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: nameInputItemContainer.bottom
                    }
                    height: parent.height * 0.35        

                    CustomImageButton {
                        id: idInputImage
                        anchors {
                            left: parent.left
                            leftMargin: 5
                            top: parent.top
                            topMargin: 10
                        }
                        height: idInputTextField.height
                        width: height
                        source: "qrc:/icon/UserID.png"
                    }

                    CustomTextField {
                        id: idInputTextField
                        anchors {
                            left: idInputImage.right
                            leftMargin: 5
                            right: parent.right
                            rightMargin: 10
                            top: parent.top
                            topMargin: 10
                        }
                        height: parent.height / 2
                        radius: 5
                        color: "#cccccc"
                        placeholderText: "Put id on here..."

                        onReturnPressed: {
                        
                        }
                    }           
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: parent.height * 0.2    

                    Button {
                        id: cancleButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: registerButton.left
                            rightMargin: 5
                        }
                        height: parent.height - 10
                        text: "Cancle"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }

                        onClicked: {
                            contactAddPopup.close()
                        }
                    }

                    Button {
                        id: registerButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            rightMargin: 5
                        }
                        height: parent.height - 10
                        text: "Register"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }

                        onClicked: {
                            // contact 추가하는 로직 추가

                            contactAddPopup.close()
                        }
                    }                    
                }
            }

            onClosed: {
                nameInputTextField.text = idInputTextField.text = ""
            }
        }
    }

    Rectangle {
        id: contactSearchBarRect
        anchors {
            left: parent.left
            right: parent.right
            top: addContactButton.bottom
        }
        height: 43
        z: 2
        color: "transparent"

        Rectangle {
            id: contactSearchBar
            anchors {
                fill: parent
                margins: 5
            }            
            radius: 5

            TextField {
                id: contactSearchField
                anchors.fill: parent
                selectByMouse: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   
                background: Rectangle {
                    color: "transparent"
                }

                onTextChanged: {
                    contactSortFilterProxyModel.filterRegex = contactSearchField.text
                }

                Keys.onReturnPressed: {
                }
            }
        }
    }

    ListView {
        id: contactListView
        anchors {
            left: parent.left
            right: parent.right
            top: contactSearchBarRect.bottom
            bottom: parent.bottom
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        model: contactSortFilterProxyModel

        delegate: Rectangle {
            id: contactInfo
            width: contactListView.width
            height: 98
            objectName: userId
            color: "#274E7D"

            property int contactIndex: index

            CustomImageButton {
                id: contactImageButton
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: 5
                }
                height: parent.height - 20
                width: height
                rounded: true
                source: userImg
            }    

            Item {
                anchors {
                    left: contactImageButton.right
                    leftMargin: 5
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                    }
                    height: parent.height / 2

                    Text {
                        id: userNameText
                        anchors {
                            left: parent.left
                            bottom: parent.bottom
                            bottomMargin: 6
                        }
                        font {
                            bold: true
                            pointSize: 15
                        }
                        text: userName
                    }
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: parent.height / 2

                    Text {
                        anchors {
                            left: parent.left
                            top: parent.top
                            topMargin: 6
                        }
                        font {
                            pointSize: 11
                        }
                        text: contactInfo.objectName
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
        }        
    }
}