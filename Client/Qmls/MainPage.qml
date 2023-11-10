import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15

Rectangle {
    id: mainPage
    color: "#280a3d"
    objectName: "mainPage"

    property string currentRoomID: ""
    property string currentSessionName: ""

    // chat list view 템플릿이 찍어낸 객체
    property var chatViewObjects: ({})

    // chat list view 템플릿
    property var chatListViewComponent: Qt.createComponent("qrc:/qml/ChatListView.qml")

    function addSession(obj)
    {
        chatRoomListModel.append({
            "sessionID": obj["sessionID"],
            "sessionName": obj["sessionName"],
            "sessionImage": "image://mybase64/data:image/png;base64," + obj["sessionImage"],
            "recentChatDate": obj["recentChatDate"],
            "recentChatContent": obj["recentChatContent"]
        })

        chatViewObjects[obj["sessionID"]] = chatListViewComponent.createObject(chatView)
    }

    // 테스트 전용 함수
    function addSessionTest(sessionID, sessionName, sessionImage, recentChatDate, recentChatContent)
    {
        chatRoomListModel.append({
            "sessionID": sessionID,
            "sessionName": sessionName,
            "sessionImage": "image://mybase64/data:image/png;base64," + sessionImage,
            "recentChatDate": recentChatDate,
            "recentChatContent": recentChatContent
        })
    
        chatViewObjects[sessionID] = chatListViewComponent.createObject(chatView)
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

    // 테스트 전용 함수
    function addChatBubbleTextTest(sessionID, isOpponent, senderID, senderName, senderImage, chatContent, chatDate)
    {
        chatViewObjects[sessionID].children[0].model.append({
            "chatBubbleSource": "qrc:/qml/ChatBubbleText.qml",
            "isOpponent": isOpponent,
            "senderID": senderID,
            "senderName": senderName,
            "senderImage": senderImage,
            "chatContent": chatContent,
            "chatDate": chatDate
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
                addItemByPath("qrc:/qml/SettingButton.qml")
                addItemByPath("qrc:/qml/ContactButton.qml")
                addItemByPath("qrc:/qml/SessionAddButton.qml")
                
                //addItemByCode(String.raw`
                //             Rectangle {
                //                 height: parent.height
                //                 width: 200
                //                 radius: 10
                //
                //                 TextField {
                //                     anchors.fill: parent
                //                     selectByMouse: true
                //                     inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText                    
                //
                //                     Keys.onReturnPressed: {
                //                     
                //                     }
                //
                //                     background: Rectangle {
                //                         color: "transparent"
                //                     }
                //                 }
                //             }
                //             `)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ColumnLayout {
                Layout.fillHeight: true
                spacing: 0                

                Rectangle {
                    Layout.preferredWidth: chatRoomListView.width - 10
                    Layout.preferredHeight: 30
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                    Layout.leftMargin: 5
                    Layout.rightMargin: 5                    
                    radius: 5

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
                                currentSessionName = sessionName

                                if(chatView.empty)
                                    chatView.push(chatViewObjects[currentRoomID], StackView.Immediate)
                                else
                                    chatView.replace(null, chatViewObjects[currentRoomID], StackView.Immediate)
                            }
                        }
                    }

                    Component.onCompleted: {           
                        addSessionTest("test_01", "chat room 1", "", "1997-03-09", "chat preview in chat room 1")
                        addSessionTest("test_02", "chat room 2", "", "2023-09-21", "chat preview in chat room 2")
                    }
                }            
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    id: sessionHeaderRectangle
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    visible: currentRoomID.length === 0 ? false : true

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                            
                        Text {
                            text: currentSessionName
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Button {
                            id: searchBarToggleButton
                            Layout.preferredHeight: sessionHeaderRectangle.height - 5
                            Layout.preferredWidth: height

                            background: Rectangle {
                                color: parent.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : Qt.rgba(1.0, 1.0, 1.0, 0.0)
                            }

                            Image {
                                id: searchBarToggleButtonImage
                                source: "qrc:/icon/UserID.png"
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                            }

                            ColorOverlay {
                                anchors.fill: searchBarToggleButtonImage
                                source: searchBarToggleButtonImage
                                color: searchBarToggleButton.hovered ? "#cccccc" : "transparent"
                            }

                            onClicked: {
                                chatSearchBar.visible ^= true
                            }
                        }
                        Button {
                            id: sessionMenuButton
                            Layout.preferredHeight: sessionHeaderRectangle.height - 5
                            Layout.preferredWidth: height

                            background: Rectangle {
                                color: parent.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : Qt.rgba(1.0, 1.0, 1.0, 0.0)
                            }

                            Image {
                                id: sessionMenuButtonImage
                                source: "qrc:/icon/UserID.png"
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                            }

                            ColorOverlay {
                                anchors.fill: sessionMenuButtonImage
                                source: sessionMenuButtonImage
                                color: sessionMenuButton.hovered ? "#cccccc" : "transparent"
                            }

                            Menu {
                                id: sessionMenu
                                y: sessionMenuButton.height + 2

                                MenuItem {
                                    id: inviteMenuItem
                                    text: "Invite new one"
                                }
                                MenuItem {
                                    id: leaveMenuItem
                                    text: "Expel someone"
                                }
                                MenuItem {
                                    id: etcMenuItem
                                    implicitHeight: 30
                                    contentItem: Rectangle {
                                        anchors.fill: parent
                                        color: "transparent"

                                        CustomImageButton {
                                            id: notifyToggleButton
                                            height: etcMenuItem.implicitHeight - 5
                                            width: height
                                            anchors {
                                                left: parent.left
                                                leftMargin: 5
                                                verticalCenter: parent.verticalCenter
                                            }
                                            checkable : true
                                            source: "qrc:/icon/UserID.png"
                                            overlayColor: notifyToggleButton.down ? "#666666" : 
                                                                                    (notifyToggleButton.checked ? "#000000" : "#cccccc")
                                            
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
                                            checkable : true
                                            source: "qrc:/icon/UserID.png"
                                            overlayColor: sessionFavoriteButton.down ? "#666666" : 
                                                                                       (sessionFavoriteButton.checked ? "#000000" : "#cccccc")
                                            
                                            onClicked: {
                                                
                                            }
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
                                    background: Rectangle {
                                        anchors.fill: parent
                                        // opacity: etcMenuItem.highlighted ? 0.7 : 1.0
                                        color: "blue"
                                    }
                                }
                            }

                            onClicked: {
                                sessionMenu.open()
                            }
                        }
                    }
                }

                Rectangle {
                    id: chatSearchBar
                    visible: false
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: "#cccccc"

                    Rectangle {
                        anchors {
                            left: parent.left
                            top: parent.top
                            right: searchBarCloseButton.left
                            bottom: parent.bottom
                            margins: 5
                        }
                        radius: 5

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
                    
                    Button {
                        id: searchBarCloseButton
                        height: chatSearchBar.height - 5
                        width: height
                        anchors {
                            right: parent.right
                            rightMargin: 5
                            verticalCenter: parent.verticalCenter
                        }

                        background: Rectangle {
                            color: parent.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : Qt.rgba(1.0, 1.0, 1.0, 0.0)
                        }

                        Image {
                            id: searchBarCloseButtonImage
                            source: "qrc:/icon/UserID.png"
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                        }

                        ColorOverlay {
                            anchors.fill: searchBarCloseButtonImage
                            source: searchBarCloseButtonImage
                            color: searchBarCloseButton.hovered ? "#cccccc" : "transparent"
                        }

                        onClicked: {
                            chatSearchBar.visible ^= true
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

                                // 서버로 채팅 내용 전송, 테스트 끝나면 밑 함수 주석 풀기
                                // mainContext.trySendTextChat(currentRoomID, text)

                                // c++ 단에서 수행해야 함, 테스트 끝나면 밑 두줄 삭제 요망
                                var chatTime = Qt.formatDateTime(new Date(), "yyyy-MM-dd hh:mm:ss.zzz")
                                addChatBubbleTextTest(currentRoomID, 
                                                  false, 
                                                  "tongstar",
                                                  "KyungJoonLee",
                                                  "qrc:/icon/UserID.png",
                                                  text,
                                                  chatTime)

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
                    Layout.preferredHeight: 50
                    visible: currentRoomID.length === 0 ? false : true
                    // color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        
                        Button {
                            id: imojiButton
                            Layout.leftMargin: 5
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            Layout.alignment: Qt.AlignVCenter

                            background: Rectangle {
                                color: parent.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : Qt.rgba(1.0, 1.0, 1.0, 0.0)
                            }

                            Image {
                                id: imojiButtonImage
                                source: "qrc:/icon/UserID.png"
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                            }

                            ColorOverlay {
                                anchors.fill: imojiButtonImage
                                source: imojiButtonImage
                                color: imojiButton.hovered ? "#cccccc" : "transparent"
                            }

                            onClicked: {
                                
                            }
                        }

                        Button {
                            id: attachmentButton
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 40
                            Layout.alignment: Qt.AlignVCenter

                            background: Rectangle {
                                color: parent.down ? Qt.rgba(1.0, 1.0, 1.0, 0.4) : Qt.rgba(1.0, 1.0, 1.0, 0.0)
                            }

                            Image {
                                id: attachmentButtonImage
                                source: "qrc:/icon/UserID.png"
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                            }

                            ColorOverlay {
                                anchors.fill: attachmentButtonImage
                                source: attachmentButtonImage
                                color: attachmentButton.hovered ? "#cccccc" : "transparent"
                            }

                            onClicked: {
                                
                            }
                        }

                        Item {   
                            Layout.fillWidth: true
                        }

                        Button {
                            Layout.rightMargin: 5
                            Layout.preferredHeight: 40
                            text: "Send"
                            background: Rectangle {
                                color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                                radius: 5
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        
    }
}
