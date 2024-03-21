import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15

Rectangle {
    id: mainPage
    objectName: "mainPage"
    color: "#28343f"
    anchors.fill: parent

    function createChatNotification(notiInfo)
    {
        var notiComp = Qt.createComponent("qrc:/qml/ChatNotificationWindow.qml")
        var notiWindow = notiComp.createObject(null, 
                                               {    
                                                    x: notiInfo["x"], 
                                                    y: notiInfo["y"], 
                                                    width: notiInfo["width"],
                                                    height: notiInfo["height"]
                                               })

        notiWindow.sessionId = notiInfo["sessionId"]
        notiWindow.senderImgPath = notiInfo["senderImgPath"]
        notiWindow.senderName = notiInfo["senderName"]
        notiWindow.content = notiInfo["content"]
        return notiWindow
    }

    function addSession(sessionInfo)
    {
        sideBarView.sideBarViewMap["session"].addSession(sessionInfo)
    }

    function addChat(chatInfo)
    {
        sideBarView.sideBarViewMap["session"].sessionViewMap[chatInfo["sessionId"]].addChat(chatInfo)
    }

    function insertOrderedChats(sessionId, index, chats)
    {
        sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].insertOrderedChats(index, chats)
    }

    function clearChat(sessionId)
    {
        sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].clearChat()
    }

    function updateReaderIds(readerId, messageId)
    {
        sideBarView.sideBarViewMap["session"].refreshReaderIds(readerId, messageId)
    }

    function selectCurrentView(currentView)
    {
        if (sideBarView.currentView !== currentView)
            sideBarView.currentView = currentView
    }

    function updateParticipantData(participantInfo)
    {
        chatSessionModel.updateParticipantData(participantInfo)
        sideBarView.sideBarViewMap["session"].sessionViewMap[participantInfo["sessionId"]].updateParticipantInfo(participantInfo)
    }

    function insertParticipantData(participantInfo)
    {
        sideBarView.sideBarViewMap["session"].sessionViewMap[participantInfo["sessionId"]].addParticipantCnt()
        chatSessionModel.insertParticipantData(participantInfo)
    }

    function deleteParticipantData(sessionId, participantId)
    {
        sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].subParticipantCnt()
        chatSessionModel.deleteParticipantData(sessionId, participantId)
    }

    function getParticipantData(sessionId, participantId)
    {
        var pData = chatSessionModel.getParticipantData(sessionId, participantId)
        pData["getType"] = "Participant"

        if (!pData.hasOwnProperty("participantName"))
        {
            var cData = getContactData(participantId)

            if (cData.hasOwnProperty("userName"))
            {
                pData["participantName"] = cData["userName"]
                pData["participantImgPath"] = cData["userImg"]
                pData["getType"] = "Contact"
            }
        }

        if (!pData.hasOwnProperty("participantName"))
        {
            pData["participantName"] = qsTr("Unknown")
            pData["participantImgPath"] = ""
            pData["getType"] = "None"
        }

        return pData

        // return chatSessionModel.getParticipantData(sessionId, participantId)
    }

    function addSessions(sessions)
    {
        sideBarView.sideBarViewMap["session"].addSessions(sessions)
    }

    function getSessionData(sessionId)
    {
        var obj = chatSessionModel.get(sessionId)
        if (obj == null)
            return {}
            
        return {
            "sessionId": obj.sessionId,
            "sessionName": obj.sessionName,
            "sessionImg": obj.sessionImg,
            "recentSenderId": obj.recentSenderId,
            "recentSendDate": obj.recentSendDate,
            "recentContentType": obj.recentContentType,
            "recentContent": obj.recentContent,
            "recentMessageId": obj.recentMessageId,
            "unreadCnt": obj.unreadCnt,
            "chatCnt": sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].children[2].count
        }
    }

    function getCurrentSessionId()
    {
        return sideBarView.sideBarViewMap["session"].currentSessionId
    }

    function getContactData(userId)
    {
        return contactModel.getContact(userId)
    }

    function handleOtherLeftSession(sessionId, deletedId)
    {
        var chatInfo = {
            "messageId": chatSessionModel.get(sessionId).recentMessageId + 0.001,
            "sessionId": sessionId,
            "senderId": "",
            "senderName": "",
            "senderImgPath": "",
            "timeSinceEpoch": "",
            "contentType": 4,
            "content": chatSessionModel.getParticipantData(sessionId, deletedId).participantName + " left.",
            "readerIds": {},
            "isOpponent": false
        }

        deleteParticipantData(sessionId, deletedId)
        sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].addChat(chatInfo)
    }

    function handleExpelParticipant(sessionId, expeledId)
    {
        var chatInfo = {
            "messageId": chatSessionModel.get(sessionId).recentMessageId + 0.001,
            "sessionId": sessionId,
            "senderId": "",
            "senderName": "",
            "senderImgPath": "",
            "timeSinceEpoch": "",
            "contentType": 4,
            "content": chatSessionModel.getParticipantData(sessionId, expeledId).participantName + " expeled.",
            "readerIds": {},
            "isOpponent": false
        }

        deleteParticipantData(sessionId, expeledId)
        sideBarView.sideBarViewMap["session"].sessionViewMap[sessionId].addChat(chatInfo)
    }

    function updateSessionData(updateInfo)
    {
        chatSessionModel.renewSessionInfo(updateInfo)
    }

    function addContacts(contactsInfo)
    {
        contactModel.addContacts(contacts)
    }

    function addContactRequest(requesterInfo)
    {
        contactRequestModel.append(requesterInfo)
    }

    function deleteContact(userId)
    {
        contactModel.remove(acqId)
    }

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
            id: sideBar
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: 45
            color: "#191f24"

            CustomImageButton {
                id: sessionListButton
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.top
                    topMargin: 5
                }
                width: parent.width - 10
                height: width
                source: "qrc:/icon/UserID.png"

                onClicked: {
                    if (sideBarView.currentView.length === 0 || sideBarView.currentView !== "session")                    
                        sideBarView.currentView = "session"                        
                    else
                        sideBarView.currentView = ""
                }
            }

            CustomImageButton {
                id: contactButton
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: sessionListButton.bottom
                }
                width: parent.width - 10
                height: width
                source: "qrc:/icon/UserID.png"

                onClicked: {
                    if (sideBarView.currentView.length === 0 || sideBarView.currentView !== "contact")
                        sideBarView.currentView = "contact"
                    else
                        sideBarView.currentView = ""
                }
            }

            CustomImageButton {
                id: settingButton
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: contactButton.bottom
                }
                width: parent.width - 10
                height: width
                // rounded: true
                source: "qrc:/icon/UserID.png"
            }
        }

        /*
        Loader {
            id: sideBarLoader
            anchors {
                left: sideBar.right
                top: parent.top
                bottom: parent.bottom
            }
            width: 0

            // 로드 빠르게 하기 위해 각 뷰를 미리 한 바퀴돈다.
            Component.onCompleted: {
                source = "qrc:/qml/SessionListView.qml"
                source = "qrc:/qml/ContactView.qml"
            }

            onStatusChanged: {
            }
        }
        */

        StackView {
            id: sideBarView
            anchors {
                left: sideBar.right
                top: parent.top
                bottom: parent.bottom
            }
            width: 0

            property string currentView: ""
            property var sideBarViewMap: ({})

            onCurrentViewChanged: {
                if (currentView.length === 0)
                    sideBarView.width = 0
                else
                {
                    sideBarView.replace(null, sideBarViewMap[currentView], StackView.Immediate)
                    sideBarView.width = 400
                }
            }

            Component.onCompleted: {
                sideBarViewMap["session"] = Qt.createComponent("qrc:/qml/SessionListView.qml").createObject(null)
                sideBarViewMap["contact"] = Qt.createComponent("qrc:/qml/ContactView.qml").createObject(null)

                sideBarView.push(sideBarViewMap["session"], StackView.Immediate)
            }
        }

        StackView {
            id: mainView
            anchors {
                left: sideBarView.right
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            objectName: "mainView"
        }
    }

    Component.onCompleted: {
        mainContext.setMainPage(mainPage)
    }
}

/*
Rectangle {
    id: mainPage
    color: "#280a3d"
    objectName: "mainPage"

    property string currentRoomID: ""
    property string currentSessionName: ""

    // chat list view 템플릿이 찍어낸 객체
    property var chatListViewMap: ({})

    // chat list view 템플릿
    property var chatListViewComponent: Qt.createComponent("qrc:/qml/ChatListView.qml")

    function addSession(sessionInfo)
    {
        chatSessionModel.append(sessionInfo)
        chatListViewMap[sessionInfo["sessionId"]] = chatListViewComponent.createObject(chatView)
    }

    function clearSpecificSession(sessionId)
    {
        chatListViewMap[sessionId].children[0].model.clear()
    }

    function getSessionChatCount(sessionId)
    {
        return chatListViewMap[sessionId].children[0].model.count
    }

    function addChat(chatInfo)
    {
        chatListViewMap[chatInfo["sessionId"]].children[0].model.append(chatInfo)
    }

    function refreshReaderIds(sessionId, readerId, messageId)
    {
        chatListViewMap[sessionId].children[0].model.refreshReaderIds(readerId, messageId)
    }

    //function addSession(obj)
    //{
    //    chatRoomListModel.append({
    //        "sessionID": obj["sessionID"],
    //        "sessionName": obj["sessionName"],
    //        "sessionImage": "image://mybase64/data:image/png;base64," + obj["sessionImage"],
    //        "recentChatDate": obj["recentChatDate"],
    //        "recentChatContent": obj["recentChatContent"]
    //    })
//
    //    chatViewObjects[obj["sessionID"]] = chatListViewComponent.createObject(chatView)
    //}
//
    //// 테스트 전용 함수
    //function addSessionTest(sessionID, sessionName, sessionImage, recentChatDate, recentChatContent)
    //{
    //    chatRoomListModel.append({
    //        "sessionID": sessionID,
    //        "sessionName": sessionName,
    //        "sessionImage": "image://mybase64/data:image/png;base64," + sessionImage,
    //        "recentChatDate": recentChatDate,
    //        "recentChatContent": recentChatContent
    //    })
    //
    //    chatViewObjects[sessionID] = chatListViewComponent.createObject(chatView)
    //}
//
    //function addChatBubbleText(obj)
    //{
    //    chatViewObjects[obj["sessionID"]].children[0].model.append({
    //        "chatBubbleSource": "qrc:/qml/ChatBubbleText.qml",
    //        "isOpponent": obj["isOpponent"],
    //        "senderID": obj["senderID"],
    //        "senderName": obj["senderName"],
    //        "senderImage": obj["senderImage"],
    //        "chatContent": obj["chatContent"],
    //        "chatDate": obj["chatDate"]
    //    })
    //}
//
    //// 테스트 전용 함수
    //function addChatBubbleTextTest(sessionID, isOpponent, senderID, senderName, senderImage, chatContent, chatDate)
    //{
    //    chatViewObjects[sessionID].children[0].model.append({
    //        "chatBubbleSource": "qrc:/qml/ChatBubbleText.qml",
    //        "isOpponent": isOpponent,
    //        "senderID": senderID,
    //        "senderName": senderName,
    //        "senderImage": senderImage,
    //        "chatContent": chatContent,
    //        "chatDate": chatDate
    //    })
    //}

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
                    Layout.preferredWidth: chatSessionListView.width - 10
                    Layout.preferredHeight: 30
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                    Layout.leftMargin: 5
                    Layout.rightMargin: 5                    
                    radius: 5

                    TextField {
                        id: chatSessionSearchField
                        anchors.fill: parent
                        selectByMouse: true
                        inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   

                        onTextChanged: {
                            chatSessionSortFilterProxyModel.filterRegex = chatSessionSearchField.text
                        }

                        Keys.onReturnPressed: {
                            // mainContext.tryRefreshSession(0);
                        }

                        background: Rectangle {
                            color: "transparent"
                        }
                    }
                }

                ListView {
                    id: chatSessionListView
                    Layout.preferredWidth: 250
                    Layout.fillHeight: true
                    clip: true

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: chatSessionSortFilterProxyModel

                    delegate: Rectangle {
                        property int chatSessionIndex: index

                        id: sessionInfoRect
                        objectName: sessionId
                        width: parent.width
                        height: 98
                        color: "#B240F5"

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Rectangle {
                                color: "transparent"
                                Layout.fillHeight: true
                                Layout.preferredWidth: height

                                CustomImageButton {
                                    id: sessionImageButton
                                    anchors {
                                        fill: parent
                                        leftMargin: 5
                                        rightMargin: 5
                                        topMargin: 5
                                        bottomMargin: 5
                                    }
                                    rounded: true
                                    source: "data:image/png;base64," + sessionImg
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
                                        //anchors.leftMargin: 5
                                        width: parent.width / 2
                                        clip: true
                                        text: sessionName
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.right: parent.right
                                        anchors.rightMargin: 5
                                        width: parent.width / 2
                                        clip: true
                                        text: recentSendDate
                                    }                                
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    color: "transparent"

                                    Text {                               
                                        anchors.verticalCenter: parent.verticalCenter 
                                        anchors.left: parent.left                            
                                        text: recentContent
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
                                sessionInfoRect.color = "#BD5CF5"
                            }
                            onExited: {
                                sessionInfoRect.color = "#B240F5"
                            }
                            onClicked: {
                                currentRoomID = sessionInfoRect.objectName
                                currentSessionName = sessionName

                                if(chatView.empty)
                                    chatView.push(chatListViewMap[currentRoomID], StackView.Immediate)
                                else
                                    chatView.replace(null, chatListViewMap[currentRoomID], StackView.Immediate)
                            }
                        }
                    }

                    Component.onCompleted: {           
                        // addSessionTest("test_01", "chat room 1", "", "1997-03-09", "chat preview in chat room 1")
                        // addSessionTest("test_02", "chat room 2", "", "2023-09-21", "chat preview in chat room 2")

                        addSession({
                            "sessionId": "tongstar-20231023 11:21:06.0532",
                            "sessionName": "안양팸",
                            "sessionImg": "",
                            "recentSenderId": "tongstar",
                            "recentSendDate": "20231215 10:21:06.1211",
                            "recentContentType": "text",
                            "recentContent": "Hello Everyone!",
                            "recentMessageId": 442,
                            "unreadCnt": 10
                        })

                        addSession({
                            "sessionId": "yellowjam-20221122 09:56:16.4215",
                            "sessionName": "엄마",
                            "sessionImg": "",
                            "recentSenderId": "yellowjam",
                            "recentSendDate": "20231214 09:11:25.4533",
                            "recentContentType": "text",
                            "recentContent": "How is ur weekend?",
                            "recentMessageId": 371,
                            "unreadCnt": 1
                        })

                        //chatSessionModel.append({
                        //    "sessionId": "tongstar-20231023 11:21:06.0532",
                        //    "sessionName": "안양팸",
                        //    "sessionImg": "",
                        //    "recentSenderId": "tongstar",
                        //    "recentSendDate": "20231215 10:21:06.1211",
                        //    "recentContentType": "text",
                        //    "recentContent": "Hello Everyone!",
                        //    "recentMessageId": 442,
                        //    "unreadCnt": 10
                        //})
//
                        //chatSessionModel.append({
                        //    "sessionId": "yellowjam-20221122 09:56:16.4215",
                        //    "sessionName": "엄마",
                        //    "sessionImg": "",
                        //    "recentSenderId": "yellowjam",
                        //    "recentSendDate": "20231214 09:11:25.4533",
                        //    "recentContentType": "text",
                        //    "recentContent": "How is ur weekend?",
                        //    "recentMessageId": 371,
                        //    "unreadCnt": 1
                        //})
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
                    objectName: "chatView"

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
                                
                                addChat({
                                    "messageId": 0,
                                    "sessionId": currentRoomID,
                                    "senderId": "tongstar",
                                    "readerIds": ["tongstar", "yellowjam"],
                                    "sendDate": chatTime,
                                    "contentType": 0,
                                    "content": text,
                                    "isOpponent": false
                                })

                                //addChatBubbleTextTest(currentRoomID, 
                                //                  false, 
                                //                  "tongstar",
                                //                  "KyungJoonLee",
                                //                  "qrc:/icon/UserID.png",
                                //                  text,
                                //                  chatTime)

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
*/