﻿import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    objectName: "contactView"
    color: "#19314F"
    anchors.fill: parent

    function processAddContact(result)
    {
        switch (result)
        {
        case 0: // CONTACTADD_SUCCESS
            contactAddResultText.text = ""
            break
        case 1: // CONTACTADD_DUPLICATION
            contactAddResultText.text = "You have already tried to add this id."
            break
        case 2: // CONTACTADD_ID_NO_EXSIST
            contactAddResultText.text = "There is no id match what you filled"
            break
        case 3: // CONTACTADD_CONNECTION_FAIL
            contactAddResultText.text = "Connection fail!"
            break
        }
    }

    Button {
        id: contactRequestsViewButton
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 5
        }
        height: 35
        text: "View Contact Requests"
        z: 2
        background: Rectangle {
            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
            radius: 5
        }

        onClicked: {
            contactRequestsViewPopup.open()
        }

        Popup {
            id: contactRequestsViewPopup
            anchors.centerIn: Overlay.overlay
            width: 500
            height: 700
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            modal: true
            background: Rectangle {
                color: "#333366"
                radius: 5
            }
            contentItem: Item {
                anchors.fill: parent

                Text {
                    id: contactRequestsViewTitle
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: parent.top
                        topMargin: 5
                    }
                    z: 2
                    text: "Contact Requests"
                    font {
                        bold: true
                        pointSize: 20
                    }
                }

                ListView {
                    id: contactRequestsView
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: contactRequestsViewTitle.bottom
                        topMargin: 5
                        bottom: contactRequestsViewButtonContainer.top
                    }
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: contactRequestFilterProxyModel

                    delegate: Rectangle {
                        id: requesterInfo
                        width: contactRequestsView.width
                        height: 98
                        objectName: requesterId
                        color: Qt.rgba(0.525, 
                                       0.55, 
                                       0.58, 
                                       requesterInfoMouseArea.containsMouse ? 1.0 : 0.6)

                        property int requesterIndex: index

                        CustomImageButton {
                            id: requesterImageButton
                            anchors {
                                verticalCenter: parent.verticalCenter
                                left: parent.left
                                leftMargin: 5
                            }
                            height: parent.height - 20
                            width: height
                            rounded: true
                            source: requesterImg
                        }    

                        Item {
                            anchors {
                                left: requesterImageButton.right
                                leftMargin: 5
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                            }

                            Button {
                                id: contactRequestDissmissButton
                                anchors {
                                    verticalCenter: parent.verticalCenter
                                    right: contactRequestAcceptButton.left
                                    rightMargin: 5
                                }
                                text: "Dismiss"
                                background: Rectangle {
                                    color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                                    radius: 5
                                }
                            }

                            Button {
                                id: contactRequestAcceptButton
                                anchors {
                                    verticalCenter: parent.verticalCenter
                                    right: parent.right
                                    rightMargin: 5
                                }
                                text: "Accept"
                                background: Rectangle {
                                    color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                                    radius: 5
                                }
                            }
                        }

                        MouseArea {
                            id: requesterInfoMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }
                }

                Item {
                    id: contactRequestsViewButtonContainer
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: 50

                    Button {
                        id: contactRequestsViewCloseButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: contactRequestsViewDismissAllButton.left
                            rightMargin: 5
                        }
                        text: "Close"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }
                    }

                    Button {
                        id: contactRequestsViewDismissAllButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: contactRequestsViewClaimAllButton.left
                            rightMargin: 5
                        }
                        text: "Dismiss All"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }
                    }

                    Button {
                        id: contactRequestsViewClaimAllButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            rightMargin: 5
                        }
                        text: "Claim All"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }
                    }
                }
            }
        }
    }

    Item {
        id: contactViewAddDeleteContainer
        anchors {
            left: parent.left
            right: parent.right
            top: contactRequestViewButton.bottom
        }
        height: 35
        z: 2

        Button {
            id: addContactButton
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                margins: 5
            }
            width: parent.width / 2
            text: "Add Contact"
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

                        Text {
                            id: contactAddResultText
                            anchors {
                                verticalCenter: parent.verticalCenter
                                left: parent.left
                                leftMargin: 5
                            }
                            text: ""
                            font {
                                bold: true
                                pointSize: 15
                            }
                            color: "red"
                        }

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

        Button {
            id: contactDeleteButton
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                margins: 5
            }
            width: parent.width / 2
            text: "Delete Contact"
            background: Rectangle {
                color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                radius: 5
            }
        }
    }

    Rectangle {
        id: contactSearchBarRect
        anchors {
            left: parent.left
            right: parent.right
            top: contactViewAddDeleteContainer.bottom
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
            color: Qt.rgba(0.525, 
                           0.55, 
                           0.58, 
                           contactInfoMouseArea.containsMouse ? 1.0 : 0.6)

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
                id: contactInfoMouseArea
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