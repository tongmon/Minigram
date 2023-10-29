import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    id: mainPage
    color: "#280a3d"
    objectName: "mainPage"

    property string currentRoomID: ""

    // chat list view 템플릿이 찍어낸 객체
    property var chatViewObjects: ({})

    // chat list view 템플릿
    property var chatListViewComponent: Qt.createComponent("qrc:/qml/ChatListView.qml")

    // function addChatRoom(chatRoomID, chatRoomName, chatRoomImage, recentUsedDate, recentChatContent)
    // {
    //     chatRoomListModel.append({
    //         "chatRoomObjectName": chatRoomID,
    //         "chatRoomName": chatRoomName,
    //         "chatRoomImage": "image://mybase64/data:image/png;base64," + chatRoomImage,
    //         "recentUsedDate": recentUsedDate,
    //         "recentChatContent": recentChatContent
    //     })
    // 
    //     chatViewObjects[chatRoomID] = chatListViewComponent.createObject(chatView)
    // }

    // function addChatBubbleText(chatRoomID, isRightAlign, userID, userName, userImage, chatData, chatTime)
    // {
    //     chatViewObjects[chatRoomID].children[0].model.append({
    //         "chatBubbleSource": "qrc:/qml/ChatBubbleText.qml",
    //         "isRightAlign": isRightAlign,
    //         "userID": userID,
    //         "userName": userName,
    //         "userImage": userImage,
    //         "chatData": chatData,
    //         "chatTime": chatTime
    //     })
    // }

    function addSession(obj)
    {
        chatRoomListModel.append({
            "sessionID": obj["sessionID"],
            "sessionName": obj["sessionName"],
            "sessionImage": "image://mybase64/data:image/png;base64," + obj["sessionImage"],
            "recentChatDate": obj["recentChatDate"],
            "recentChatContent": obj["recentChatContent"]
        })

        chatViewObjects[chatRoomID] = chatListViewComponent.createObject(chatView)
    }

    function addChatBubbleText(obj)
    {
        chatViewObjects[obj["sessionID"]].children[0].model.append({
            "chatBubbleSource": "qrc:/qml/ChatBubbleText.qml",
            "isOpponent": obj["isOpponent"],
            "senderID": obj["senderID"],
            "senderName": obj["senderName"],
            "senderImage": obj["senderImage"],
            "chatContent": obj["chatContent"],
            "chatDate": obj["chatDate"]
        })
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            Layout.alignment: Qt.AlignTop

            Component.onCompleted: {
                addItem(String.raw`
                        Button {
                            height: parent.height
                            text: "Setting"
                        }
                        `)

                addItem(String.raw`
                        Button {
                            height: parent.height
                            text: "Contact"
                        }
                        `)

                addItem(String.raw`
                        Button {
                            height: parent.height
                            text: "Chat"
                        }
                        `)
                
                addItem(String.raw`
                        Rectangle {
                            height: parent.height
                            width: 200
                            radius: 10

                            TextField {
                                anchors.fill: parent
                                selectByMouse: true
                                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText                    

                                Keys.onReturnPressed: {

                                }

                                background: Rectangle {
                                    color: "transparent"
                                }
                            }
                        }
                        `)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ListView {
                id: chatRoomListView
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                clip: true

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
                
                model: ListModel {
                    id: chatRoomListModel
                }
                
                delegate: Rectangle {
                    property int chatRoomIndex: index

                    id: chatRoomID
                    objectName: sessionID
                    width: parent.width
                    height: 98
                    color: "#B240F5"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                            
                        Rectangle {
                            id: sessionImageRect
                            Layout.fillHeight: true
                            Layout.preferredWidth: sessionImageRect.height
                            color: "transparent"

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.horizontalCenter: parent.horizontalCenter
                                height: parent.height * 0.8
                                width: parent.width * 0.8
                                radius: width * 2

                                Image {
                                    anchors.fill: parent
                                    source: sessionImage // "qrc:/icon/UserID.png" 
                                    fillMode: Image.PreserveAspectFit
                                }    
                            }
                        }

                        ColumnLayout {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            spacing: 0

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "transparent"

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter 
                                    anchors.left: parent.left
                                    anchors.right: recentChatDateText.left
                                    anchors.rightMargin: 10
                                    clip: true
                                    text: sessionName
                                }

                                Text {
                                    id: recentChatDateText
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.right: parent.right
                                    anchors.rightMargin: 5
                                    text: recentChatDate
                                }                                
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "transparent"

                                Text {                               
                                    anchors.verticalCenter: parent.verticalCenter 
                                    anchors.left: parent.left                            
                                    text: recentChatContent
                                }                            
                            }
                        }

                        Component.onCompleted: { 
                            
                        }

                        Component.onDestruction: {

                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true

                        onEntered: {
                            chatRoomID.color = "#BD5CF5"
                        }
                        onExited: {
                            chatRoomID.color = "#B240F5"
                        }
                        onClicked: {
                            currentRoomID = chatRoomID.objectName

                            if(chatView.empty)
                                chatView.push(chatViewObjects[currentRoomID], StackView.Immediate)
                            else
                                chatView.replace(null, chatViewObjects[currentRoomID], StackView.Immediate)
                        }
                    }
                }

                Component.onCompleted: {           
                    // addChatRoom("test_01", "chat room 1", "", "1997-03-09", "chat preview in chat room 1")
                    // addChatRoom("test_02", "chat room 2", "", "2023-09-21", "chat preview in chat room 2")
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    visible: currentRoomID.length === 0 ? false : true

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                            
                        Text {
                            text: "Some Name"
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "Search"
                        }
                        Button {
                            text: "Chat Room Menu"
                        }
                    }
                }

                StackView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    id: chatView

                    // initialItem: Rectangle {
                    //     anchors.fill: parent
                    //     color: "black"
                    // }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    visible: currentRoomID.length === 0 ? false : true

                    Flickable {
                        id: chatInputAreaFlickable
                        anchors.fill: parent
                        contentWidth: width
                        contentHeight: parent.height

                        TextArea.flickable: TextArea {
                            id: chatInputArea
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

                                // var chatTime = Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss.zzz")

                                // 서버로 채팅 내용 전송
                                mainPageContext.trySendTextChat(currentRoomID, text)

                                // c++ 단에서 수행해야 함, 밑에꺼 삭제 요망
                                // addChatBubbleText(currentRoomID, 
                                //                   true, 
                                //                   "tongstar",
                                //                   "KyungJoonLee",
                                //                   "",
                                //                   text,
                                //                   chatTime)

                                text = ""  
                            }                
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    visible: currentRoomID.length === 0 ? false : true

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        
                        Button {
                            Layout.fillHeight: true
                            text: "Imoji"
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        
    }
}
