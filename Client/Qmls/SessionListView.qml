import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    id: sessionListBck
    color: "#19314F"
    anchors.fill: parent

    // chat list view 템플릿
    property var sessionViewComponent: Qt.createComponent("qrc:/qml/SessionView.qml")

    // chat list view 템플릿이 찍어낸 객체
    property var sessionViewMap: ({})

    property string currentSessionId: ""

    function addSession(sessionInfo)
    {
        chatSessionModel.append(sessionInfo)
        sessionViewMap[sessionInfo["sessionId"]] = sessionViewComponent.createObject(null)
        sessionViewMap[sessionInfo["sessionId"]].sessionId = sessionInfo["sessionId"]
    }

    function addChat(chatInfo)
    {
        // sessionViewMap[chatInfo["sessionId"]].children[2].model.append(chatInfo)
        sessionViewMap[chatInfo["sessionId"]].sessionViewModel.append(chatInfo)
    }

    Button {
        id: sessionAddButton
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 5
        }
        height: 35
        text: "New Chat"
        background: Rectangle {
            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
            radius: 5
        }

        onClicked: {
            sessionNameDecisionPopup.open()
        }

        Popup {
            id: sessionNameDecisionPopup
            anchors.centerIn: Overlay.overlay
            width: 500
            height: 300
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
            modal: true
            background: Rectangle {
                color: "transparent"
            }
            contentItem: Rectangle {
                anchors.fill: parent
                color: "#333366"
                radius: 5

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        bottom: buttonContainer.top
                    }

                    CustomImageButton {
                        id: sessionImageSelectButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                        }
                        width: parent.width * 0.25
                        height: width
                        source: "qrc:/icon/UserID.png"
                        overlayColor: Qt.rgba(100, 100, 100, hovered ? 0.7 : 0)
                        rounded: true

                        onClicked: {
                            var selectedFiles = mainContext.executeFileDialog({
                                "title": "Selete profile image",
                                "init_dir": ".",
                                "filter": "Image File(*.png)\0*.png\0",
                                "max_file_cnt": 1
                            })
                            if(selectedFiles.length)
                                source = "file:///" + selectedFiles[0]
                        }
                    }

                    Item {
                        anchors {
                            left: sessionImageSelectButton.right
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
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                    bottom: parent.bottom
                                    bottomMargin: 20
                                }
                                font {
                                    bold: true
                                    pointSize: 15
                                }
                                text: "Session Name"
                            }                        
                        }

                        Item {
                            anchors {
                                left: parent.left
                                right: parent.right
                                bottom: parent.bottom
                            }
                            height: parent.height / 2         

                            Rectangle {
                                id: sessionNameInput      
                                anchors {
                                    left: parent.left
                                    right: parent.right
                                    rightMargin: 10
                                    top: parent.top
                                    topMargin: 8
                                }
                                height: 35
                                radius: 5

                                TextField {
                                    id: sessionNameTextField
                                    anchors.fill: parent
                                    selectByMouse: true
                                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText   

                                    background: Rectangle {
                                        color: "transparent"
                                    }

                                    Keys.onReturnPressed: {
                                    
                                    }
                                    Component.onCompleted: {
                                    }
                                }
                            }               
                        }
                    }
                }

                Item {
                    id: buttonContainer
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    height: 50

                    Button {
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: nextButton.left
                            rightMargin: 5
                        }
                        height: parent.height - 10;
                        text: "Close"
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }

                        onClicked: {
                            sessionNameDecisionPopup.close()
                        }
                    }

                    Button {
                        id: nextButton
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            rightMargin: 5
                        }
                        background: Rectangle {
                            color: parent.down ? Qt.rgba(0.7, 0.7, 0.7, 1.0) : Qt.rgba(0.7, 0.7, 0.7, 0.4)
                            radius: 5
                        }
                        height: parent.height - 10;
                        text: "Next"

                        onClicked: {
                            
                        }
                    }
                }
            }

            onClosed: {
                sessionNameTextField.text = ""
                sessionImageSelectButton.source = "qrc:/icon/UserID.png"
            }
        }
    }

    Rectangle {
        id: sessionSearchBarRect
        anchors {
            left: parent.left
            right: parent.right
            top: sessionAddButton.bottom
        }
        height: 43
        color: "transparent"

        Rectangle {
            id: sessionSearchBar
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
        id: sessionListView
        anchors {
            left: parent.left
            right: parent.right
            top: sessionSearchBarRect.bottom
            bottom: parent.bottom
        }
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        model: chatSessionSortFilterProxyModel

        delegate: Rectangle {
            property int sessionIndex: index

            id: sessionInfo
            objectName: sessionId
            height: 98
            width: sessionListView.width
            color: "#274E7D"

            CustomImageButton {
                id: sessionImageButton
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                }
                height: parent.height - 20
                width: height
                rounded: true
                source: "qrc:/icon/UserID.png" // sessionImg
            }            

            Item {
                anchors {
                    left: sessionImageButton.right
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        top: parent.top
                        topMargin: 7
                    }
                    height: parent.height / 2

                    Text {
                        id: sessionNameText
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                        }
                        width: parent.width / 2
                        font {
                            bold: true
                            pointSize: 15
                        }
                        clip: true
                        text: sessionName
                    } 

                    Text {
                        id: sessionDateText
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            rightMargin: 5
                        }
                        width: parent.width / 2
                        clip: true
                        horizontalAlignment: Text.AlignRight
                        text: recentSendDate
                    } 
                }

                Item {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                        bottomMargin: 7
                    }
                    height: parent.height / 2

                    Text {
                        id: recentMessageText
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                            right: unreadCntRect.left
                        }
                        font {
                            pointSize: 12
                        }
                        clip: true
                        text: recentContent
                    }     

                    Rectangle {
                        id: unreadCntRect
                        color: "red"
                        anchors {
                            verticalCenter: parent.verticalCenter
                            right: parent.right
                            rightMargin: 5
                        }
                        width: childrenRect.width + 4
                        height: childrenRect.height + 4
                        radius: 5
                        
                        Text {
                            font {
                                pointSize: 12
                            }
                            clip: true
                            text: unreadCnt
                            color: "white"
                        }
                    } 
                }
            }         

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    
                }

                onExited: {

                }

                onClicked: {
                    currentSessionId = sessionInfo.objectName

                    if(mainView.empty)
                        mainView.push(sessionViewMap[currentSessionId], StackView.Immediate)
                    else
                        mainView.replace(null, sessionViewMap[currentSessionId], StackView.Immediate)
                }
            }
        }

        Component.onCompleted: {
            for(let i=0;i<20;i++) {
                addSession({
                    "sessionId": "tongstar-20231023 11:21:06.053" + i,
                    "sessionName": "안양팸",
                    "sessionImg": "",
                    "recentSenderId": "tongstar",
                    "recentSendDate": "20231215 10:21:06.1211",
                    "recentContentType": "text",
                    "recentContent": "Hello Everyone!",
                    "recentMessageId": 442,
                    "unreadCnt": 10
                })  
            }
        }
    }
}