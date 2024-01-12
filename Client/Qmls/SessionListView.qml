import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Rectangle {
    id: sessionListBck
    color: "#19314F"

    // chat list view 템플릿
    property var chatListViewComponent: Qt.createComponent("qrc:/qml/SessionView.qml")

    // chat list view 템플릿이 찍어낸 객체
    property var chatListViewMap: ({})

    property string currentSessionId: ""

    function addSession(sessionInfo)
    {
        chatSessionModel.append(sessionInfo)
        // chatListViewMap[sessionInfo["sessionId"]] = chatListViewComponent.createObject(mainView)
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
            width: parent.width
            height: 98
            color: "#274E7D"

            Row {
                anchors.fill: parent

                CustomImageButton {
                    id: sessionImageButton
                    anchors {
                        left: parent.left
                        leftMargin: 5
                        verticalCenter: parent.verticalCenter
                    }
                    height: parent.height - 20
                    width: height
                    rounded: true
                    source: "qrc:/icon/UserID.png" // sessionImg
                }

                Column {
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
                        }
                        height: parent.height / 2

                        Text {
                            id: sessionNameText
                            anchors {
                                verticalCenter: parent.verticalCenter
                                left: parent.left
                            }
                            font {
                                bold: true
                                pointSize: 15
                            }
                            width: parent.width / 2
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
                            text: recentSendDate
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
                            id: recentMessageText
                            anchors {
                                verticalCenter: parent.verticalCenter
                                left: parent.left
                            }
                            font {
                                pointSize: 12
                            }
                            width: parent.width * 0.7
                            clip: true
                            text: recentContent
                        }     

                        Rectangle {
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
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    
                }

                onExited: {

                }

                onClicked: {
                    //currentSessionId = sessionInfo.objectName
//
                    //if(mainView.empty)
                    //    chatView.push(chatListViewMap[currentSessionId], StackView.Immediate)
                    //else
                    //    chatView.replace(null, chatListViewMap[currentSessionId], StackView.Immediate)
                }
            }
        }

        Component.onCompleted: {
            for(let i=0;i<20;i++) {
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
            }
        }
    }
}