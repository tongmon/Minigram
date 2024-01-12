import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import minigram.chat.component 1.0

Column {
    anchors.fill: parent

    Rectangle {
        id: sessionHeaderRect
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        color: "white"
        height: 60

        Row {
            anchors.fill: parent

            Text {
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                }
                font {
                    bold: true
                    pointSize: 15
                }
                text: chatSessionModel.getNamebyId(sessionId)
            }

            CustomImageButton {
                id: searchBarToggleButton
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: sessionMenuButton.left
                    rightMargin: 5
                }

                onClicked: {

                }
            }

            CustomImageButton {
                id: sessionMenuButton
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    rightMargin: 5
                }

                Menu {
                    id: sessionMenu
                    y: sessionMenuButton.height + 2

                    MenuItem {
                        id: inviteMenuItem
                        text: "Invite new one"
                    }
                    MenuItem {
                        id: expelMenuItem
                        text: "Expel someone"
                    }
                    MenuItem {
                        id: etcMenuItem
                        implicitHeight: 30
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
                                checkable : true
                                source: "qrc:/icon/UserID.png"
                                overlayColor: notifyToggleButton.down ? "#666666" : (notifyToggleButton.checked ? "#000000" : "#cccccc")                    
                            }
                        }
                    }
                }
            }
        }
    }

    ListView {
        id: sessionView
        anchors.fill: parent
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
                height: (item !== null && typeof(item) !== 'undefined') ? item.height : 0
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
}