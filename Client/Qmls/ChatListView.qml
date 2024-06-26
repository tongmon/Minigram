﻿import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import minigram.chat.component 1.0

Rectangle {
    color: "transparent"

    ListView {
        anchors.fill: parent
        id: chatListView
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    
        model: ChatModel {
            id: chatListModel
        }

        delegate: Item {
            width: parent.width
            height: chatLoader.height
            objectName: senderId // 어떤 사람이 연속으로 메시지를 보내고 있는지 알기 위함

            // 말풍선 꼭다리 부분 크기
            property var chatBubbleStemSize: Qt.size(11, 8)

            // 말풍선 최대 너비
            property var chatBubbleMaximumWidth: chatListView.width * 0.6

            // 말풍선 최소 크기
            property var chatBubbleMinimumSize: Qt.size(10, 10)

            Loader {
                id: chatLoader
                width: parent.width
                height: (item !== null && typeof(item) !== 'undefined') ? item.height : 0
                source: qmlSource // "qrc:/qml/TextChat.qml"

                onLoaded: {
                    // item.objectName = chatBubbleID
                }
            }

            Component.onCompleted: {
                
            }
        }

        onCountChanged: {
            Qt.callLater(positionViewAtEnd)
        }

        Component.onCompleted: {
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