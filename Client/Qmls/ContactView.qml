﻿import QtQuick 2.15
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
        background: Rectangle {
            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
            radius: 5
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
        color: "transparent"

        Rectangle {
            id: contactSearchBar
            anchors {
                fill: parent
                margins: 5
            }            
            radius: 5

            TextField {
                anchors.fill: parent
                selectByMouse: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   
                background: Rectangle {
                    color: "transparent"
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