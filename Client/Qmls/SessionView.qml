﻿import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import minigram.chat.component 1.0

Rectangle {
    anchors.fill: parent
    color: "#28343f"

    property string sessionId: ""
    property string sessionName: typeof(mainContext) !== "undefined" ? mainContext.getSessionNameById(sessionId) : ""

    function refreshParticipantInfo(participantInfo)
    {
        sessionView.model.refreshParticipantInfo(participantInfo)
    }

    function addChat(chatInfo)
    {
        // chatListViewMap[chatInfo["sessionId"]].children[0].model.append(chatInfo)
        sessionView.model.append(chatInfo)
    }

    Rectangle {
        id: sessionHeaderRect
        color: "white"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        z: 2
        height: 60

        Text {
            anchors {
                verticalCenter: parent.verticalCenter 
                left: parent.left
                leftMargin: 5
                right: searchBarToggleButton.left
            }
            font {
                bold: true
                pointSize: 15
            }
            width: parent.width - searchBarToggleButton.width - sessionMenuButton.width
            text: sessionName
        }        
            
        CustomImageButton {
            id: searchBarToggleButton
            anchors {
                verticalCenter: parent.verticalCenter 
                right: sessionMenuButton.left
            }
            width: height
            height: parent.height - 10
            source: "qrc:/icon/UserID.png"

            onClicked: {
                // chatSearchBarRect.visible ^= true
                chatSearchBarRect.height = chatSearchBarRect.height ? 0 : 40
            }
        }

        CustomImageButton {
            id: sessionMenuButton
            anchors {
                verticalCenter: parent.verticalCenter 
                right: parent.right
                rightMargin: 5
            }
            width: height
            height: parent.height - 10
            source: "qrc:/icon/UserID.png"

            Menu {
                id: sessionMenu
                y: sessionMenuButton.height + 2

                MenuItem {
                    id: inviteMenuItem
                    text: "Invite new one"
                    background: Rectangle {
                        anchors.fill: parent
                        opacity: inviteMenuItem.highlighted ? 0.7 : 1.0
                        color: "#2F4F4F"
                    }
                }
                MenuItem {
                    id: expelMenuItem
                    text: "Expel someone"
                    background: Rectangle {
                        anchors.fill: parent
                        opacity: expelMenuItem.highlighted ? 0.7 : 1.0
                        color: "#2F4F4F"
                    }
                }
                MenuItem {
                    id: etcMenuItem
                    implicitHeight: 30
                    background: Rectangle {
                        anchors.fill: parent
                        color: "#2F4F4F"
                    }
                    contentItem: Item {
                        anchors.fill: parent

                        CustomImageButton {
                            id: notifyToggleButton
                            height: etcMenuItem.implicitHeight - 5
                            width: height
                            anchors {
                                left: parent.left
                                leftMargin: 5
                                verticalCenter: parent.verticalCenter
                            }
                            checkable: true
                            source: "qrc:/icon/UserID.png"
                            overlayColor: notifyToggleButton.down ? "#666666" : (notifyToggleButton.checked ? "#000000" : "#cccccc")                    
                        
                            onClicked: {
                                            
                            }
                        }

                        CustomImageButton {
                            id: sessionFavoriteButton
                            height: etcMenuItem.implicitHeight - 5
                            width: height
                            anchors {
                                left: notifyToggleButton.right
                                verticalCenter: parent.verticalCenter
                            }
                            checkable: true
                            source: "qrc:/icon/UserID.png"
                            overlayColor: sessionFavoriteButton.down ? "#666666" : (sessionFavoriteButton.checked ? "#000000" : "#cccccc")                    
                        }

                        CustomImageButton {
                            id: sessionLeaveButton
                            height: etcMenuItem.implicitHeight - 5
                            width: height
                            anchors {
                                right: parent.right
                                rightMargin: 5
                                verticalCenter: parent.verticalCenter
                            }
                            source: "qrc:/icon/UserID.png"
                            overlayColor: sessionLeaveButton.hovered ? "#666666" : "transparent"
                            
                            onClicked: {
                                
                            }
                        }
                    }
                }
            }

            onClicked: {
                sessionMenu.open()
            }
        }
    }

    Rectangle {
        id: chatSearchBarRect
        anchors {
            left: parent.left
            right: parent.right
            top: sessionHeaderRect.bottom
        }
        height: 0
        color: "#cccccc"
        z: 2

        Rectangle {
            id: chatSearchBar
            anchors {
                left: parent.left
                right: searchBarCloseButton.left
                top: parent.top
                bottom: parent.bottom
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

        CustomImageButton {
            id: searchBarCloseButton
            height: chatSearchBarRect.height - 5
            width: height
            anchors {
                right: parent.right
                rightMargin: 5
                verticalCenter: parent.verticalCenter
            }
            overlayColor: searchBarCloseButton.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : (searchBarCloseButton.hovered ? "#cccccc" : "transparent")
            source: "qrc:/icon/UserID.png"

            onClicked: {
                // chatSearchBarRect.visible ^= true
                chatSearchBarRect.height = chatSearchBarRect.height ? 0 : 40
            }
        }
    }

    ListView {
        id: sessionView
        anchors {
            left: parent.left
            right: parent.right
            top: chatSearchBarRect.bottom
            bottom: chatInputArea.top
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    
        model: ChatModel {
            id: sessionViewModel
        }

        delegate: Item {
            width: parent.width
            height: chatLoader.height
            objectName: senderId // 어떤 사람이 연속으로 메시지를 보내고 있는지 알기 위함

            // 말풍선 꼭다리 부분 크기
            property var chatBubbleStemSize: Qt.size(11, 8)

            // 말풍선 최대 너비
            property var chatBubbleMaximumWidth: sessionView.width * 0.6

            // 말풍선 최소 크기
            property var chatBubbleMinimumSize: Qt.size(10, 10)

            Loader {
                id: chatLoader
                width: parent.width
                height: (typeof(item) !== 'undefined' && item) ? item.height : 0
                source: qmlSource

                onLoaded: {

                }
            }

            Component.onCompleted: {
            }
        }

        onCountChanged: {
            Qt.callLater(positionViewAtEnd)
        }

        // 밑 로직으로 날짜에 따라 채팅 읽어오기 가능
        // onContentYChanged: {
        //     var currentIndexAtTop = indexAt(1, contentY)
        //     console.log(currentIndexAtTop)
        //                     
        //     var currentItem = itemAt(1, contentY)
        //     console.log(currentItem.objectName)
        // }
    }

    Rectangle {
        id: chatInputArea
        anchors {
            left: parent.left
            right: parent.right
            bottom: sessionFooterRect.top
        }
        height: 100
        color: "#cccccc"
        z: 2

        Flickable {
            id: chatInputAreaFlickable
            anchors.fill: parent
            contentWidth: width
            contentHeight: parent.height

            TextArea.flickable: TextArea {
                id: chatInput
                width: chatInputAreaFlickable.width
                height: chatInputAreaFlickable.height
                font.pointSize: 10
                selectByMouse: true
                wrapMode: TextEdit.Wrap

                Keys.onReturnPressed: {
                    if(event.modifiers & Qt.ShiftModifier) {
                        insert(cursorPosition, "\n")
                        return
                    }
                    
                    if(!text.length)
                        return

                    // mainContext.trySendChat(sessionId, text)

                    // 테스트용
                    sessionViewModel.append({
                        "messageId": 0,
                        "sessionId": sessionId,
                        "senderId": "tongstar",
                        "readerIds": ["tongstar", "yellowjam"],
                        "sendDate": "2024-01-14 04:40",
                        "contentType": 1,
                        "content": text,
                        "isOpponent": false
                    })

                    text = "" 
                }
            }
        }
    }

    Rectangle {
        id: sessionFooterRect
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 50
        z: 2

        CustomImageButton {
            id: imojiButton
            width: height
            height: parent.height - 10
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 5
            }
            source: "qrc:/icon/UserID.png"
            overlayColor: imojiButton.hovered ? "#cccccc" : "transparent"
        }

        CustomImageButton {
            id: attachmentButton
            width: height
            height: parent.height - 10
            anchors {
                verticalCenter: parent.verticalCenter
                left: imojiButton.right
            }
            source: "qrc:/icon/UserID.png"
            overlayColor: attachmentButton.hovered ? "#cccccc" : "transparent"
        }

        Button {
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                rightMargin: 5
            }
            height: parent.height - 10
            text: "Send"
            background: Rectangle {
                color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                radius: 5
            }
        }
    }

    Component.onCompleted: {
        
    }
}