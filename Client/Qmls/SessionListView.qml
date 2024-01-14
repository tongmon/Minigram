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
        sessionViewMap[sessionInfo["sessionId"]].sessionName = mainContext.getSessionNameById(sessionInfo["sessionId"])
    }

    function addChat(chatInfo)
    {
        sessionViewMap[chatInfo["sessionId"]].children[2].model.append(chatInfo)
    }

    ListView {
        id: sessionListView
        anchors.fill: parent
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