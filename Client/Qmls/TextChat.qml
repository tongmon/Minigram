import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Item {
    property int contentMargin: 3
    property bool hasStem: {
        var index = sessionViewModel.getIndexFromMsgId(messageId)
        return index <= 0 ? true : 
               ((senderId === sessionView.itemAtIndex(index - 1).objectName && sendDate === sessionView.itemAtIndex(index - 1).sendDateStr) ? false : true)
    }

    width: parent.width
    height: chatItemsContainer.height // (isOpponent && hasStem) ? Math.max(senderProfileImg.height, chatItemsContainer.height) : chatItemsContainer.height
    // anchors.topMargin: 2

    CustomImageButton {
        id: senderProfileImg
        source: "file:///" + senderImgPath
        anchors {
            left: parent.left
            top: parent.top
        }
        width: 50
        height: 50
        visible: isOpponent && hasStem
        rounded: true
    }

    Item {
        id: imgSpacer
        visible: isOpponent && !hasStem
        anchors {
            left: parent.left
            top: parent.top
        }
        width: senderProfileImg.width
        height: chatItemsContainer.height
    }

    Item {
        id: chatItemsContainer
        anchors {
            left: isOpponent ? (hasStem ? senderProfileImg.right : imgSpacer.right) : parent.left
            right: parent.right
            //right: isOpponent ? undefined : parent.right
            top: parent.top
        }
        width: childrenRect.width
        height: isOpponent && hasStem ? childrenRect.height : chatCanvas.height

        Text {
            id: chatSenderText
            text: senderName
            font.pixelSize: 15
            visible: isOpponent && hasStem
            anchors {
                left: parent.left
                leftMargin: chatBubbleStemSize.width
                top: parent.top
            }
        }

        Canvas {
            id: chatCanvas
            width: Math.max(chatContent.width, chatDateText.paintedWidth) + chatBubbleStemSize.width + contentMargin * 2
            height: childrenRect.height + contentMargin * 2
            anchors {
                left: isOpponent ? parent.left : undefined
                right: isOpponent ? undefined : parent.right
                top: isOpponent ? chatSenderText.bottom : parent.top
                topMargin: isOpponent ? 2 : 0
            }

            Item {
                id: chatContent
                anchors {
                    left: parent.left
                    leftMargin: contentMargin + (isOpponent ? chatBubbleStemSize.width : 0)
                    right: parent.right
                    rightMargin: contentMargin + (isOpponent ? 0 : chatBubbleStemSize.width)
                    top: parent.top
                    topMargin: contentMargin
                }
                width: Math.min(chatBubbleMaximumWidth, Math.max(chatBubbleMinimumSize.width, Math.max(dummyText.paintedWidth, chatDateText.paintedWidth)))
                height: Math.max(chatContentTextEdit.paintedHeight, chatBubbleMinimumSize.height)

                TextEdit {
                    id: chatContentTextEdit
                    anchors.fill: parent
                    text: content
                    font.pixelSize: 15
                    selectByMouse: true
                    wrapMode: TextEdit.Wrap
                    readOnly: true
                }

                Text {
                    id: dummyText
                    text: chatContentTextEdit.text
                    wrapMode: chatContentTextEdit.wrapMode
                    font: chatContentTextEdit.font
                    visible: false
                }
            }

            Text {
                id: chatDateText
                anchors {
                    left: isOpponent ? undefined : parent.left
                    leftMargin: contentMargin
                    right: isOpponent ? parent.right : undefined
                    rightMargin: contentMargin
                    top: chatContent.bottom
                    topMargin: 2
                }
                text: sendDate
            }

            onPaint: {
                var context = getContext("2d")
                context.beginPath()

                if (hasStem) {
                   if (!isOpponent) {
                       context.moveTo(width - chatBubbleStemSize.width, 0)
                       context.lineTo(width - chatBubbleStemSize.width, 5)
                       context.lineTo(width, 5)
                       context.lineTo(width - chatBubbleStemSize.width, 5 + chatBubbleStemSize.height)
                       context.lineTo(width - chatBubbleStemSize.width, height)
                       context.lineTo(0, height)
                       context.lineTo(0, 0)
                       context.lineTo(width - chatBubbleStemSize.width, 0)
                   }
                   else {
                       context.moveTo(chatBubbleStemSize.width, 0)
                       context.lineTo(chatBubbleStemSize.width, 5)
                       context.lineTo(0, 5)
                       context.lineTo(chatBubbleStemSize.width, 5 + chatBubbleStemSize.height)
                       context.lineTo(chatBubbleStemSize.width, height)
                       context.lineTo(width, height)
                       context.lineTo(width, 0)
                       context.lineTo(chatBubbleStemSize.width, 0)
                   }
                }
                else {
                   if (!isOpponent) {
                       context.moveTo(width - chatBubbleStemSize.width, 0)
                       context.lineTo(width - chatBubbleStemSize.width, height)
                       context.lineTo(0, height)
                       context.lineTo(0, 0)
                       context.lineTo(width - chatBubbleStemSize.width, 0)
                   }
                   else {
                       context.moveTo(chatBubbleStemSize.width, 0)
                       context.lineTo(width, 0)
                       context.lineTo(width, height)
                       context.lineTo(chatBubbleStemSize.width, height)
                       context.lineTo(chatBubbleStemSize.width, 0)
                   }
                }
 
                context.fillStyle = "white"
                context.fill()
            }
        }
    }

    Component.onCompleted: { 
        console.log(chatCanvas.height)
        console.log(chatItemsContainer.height)
    }
}

// Item {
//     property int contentMargin: 3
// 
//     property bool hasStem: {
//         var index = sessionViewModel.getIndexFromMsgId(messageId)
//         console.log(index)
//         return index <= 0 ? true : 
//                ((senderId === sessionView.itemAtIndex(index - 1).objectName && sendDate === sessionView.itemAtIndex(index - 1).sendDateStr) ? false : true)
//     }
// 
//     width: parent.width
//     height: chatLayout.spacing + ((isOpponent && hasStem) ? chatSenderText.paintedHeight : 0) + chatBackground.height
// 
//     RowLayout {       
//         anchors.fill: parent
//         spacing: 0
// 
//         Item {
//             Layout.fillWidth: !isOpponent
//         }
// 
//         Item {
//             visible: isOpponent && !hasStem
//             Layout.preferredWidth: 50
//         }
// 
//         CustomImageButton {
//             source: "file:///" + senderImgPath
//             Layout.alignment: Qt.AlignTop
//             Layout.preferredWidth: 50
//             Layout.preferredHeight: 50
//             visible: isOpponent && hasStem
//             rounded: true
//         }
// 
//         // Image {
//         //     source: "file:///" + senderImgPath // mainContext.getUserImgById(senderId)  // "data:image/png;base64," + senderImg
//         //     visible: isOpponent
//         //     Layout.alignment: Qt.AlignTop
//         //     Layout.preferredWidth: 50
//         //     Layout.preferredHeight: 50
//         // }
// 
//         ColumnLayout {
//             id: chatLayout
//             spacing: 0
// 
//             Text {
//                 id: chatSenderText
//                 text: senderName // mainContext.getUserNameById(senderId)
//                 font.pixelSize: 15
//                 visible: isOpponent && hasStem
//                 Layout.leftMargin: chatBubbleStemSize.width
//             }
// 
//             Canvas {
//                 id: chatBackground
//                 Layout.preferredWidth: Math.max(chatContent.width, chatDateText.paintedWidth) + chatBubbleStemSize.width + contentMargin * 2
//                 Layout.preferredHeight: chatContent.height + chatDateText.paintedHeight + contentMargin * 2
// 
//                 Item {
//                     id: chatContent
//                     x: isOpponent ? (chatBubbleStemSize.width + contentMargin) : contentMargin
//                     y: contentMargin
//                     width: Math.min(chatBubbleMaximumWidth, Math.max(chatBubbleMinimumSize.width, Math.max(dummyText.paintedWidth, chatDateText.paintedWidth)))
//                     height: Math.max(chatContentTextEdit.paintedHeight, chatBubbleMinimumSize.height)
// 
//                     TextEdit {
//                         id: chatContentTextEdit
//                         anchors.verticalCenter: parent.verticalCenter 
//                         anchors.left: parent.left
//                         width: parent.width
//                         height: parent.height
//                         text: content
//                         font.pixelSize: 15
//                         selectByMouse: true
//                         wrapMode: TextEdit.Wrap
//                         readOnly: true
//                     }
// 
//                     Text {
//                         id: dummyText
//                         text: chatContentTextEdit.text
//                         wrapMode: chatContentTextEdit.wrapMode
//                         font: chatContentTextEdit.font
//                         visible: false
//                     }
//                 }
// 
//                 Text {
//                     id: chatDateText
//                     anchors.top: chatContent.bottom
//                     anchors.left: isOpponent ? undefined : chatContent.left
//                     anchors.right: isOpponent ? chatContent.right : undefined
//                     text: sendDate
//                 }
// 
//                 onPaint: {
//                     var context = getContext("2d")
//                     context.beginPath()
// 
//                     console.log(hasStem)
// 
//                     if (hasStem) {
//                         if (!isOpponent) {
//                             context.moveTo(width - chatBubbleStemSize.width, 0)
//                             context.lineTo(width - chatBubbleStemSize.width, 5)
//                             context.lineTo(width, 5)
//                             context.lineTo(width - chatBubbleStemSize.width, 5 + chatBubbleStemSize.height)
//                             context.lineTo(width - chatBubbleStemSize.width, height)
//                             context.lineTo(0, height)
//                             context.lineTo(0, 0)
//                             context.lineTo(width - chatBubbleStemSize.width, 0)
//                         }
//                         else {
//                             context.moveTo(chatBubbleStemSize.width, 0)
//                             context.lineTo(chatBubbleStemSize.width, 5)
//                             context.lineTo(0, 5)
//                             context.lineTo(chatBubbleStemSize.width, 5 + chatBubbleStemSize.height)
//                             context.lineTo(chatBubbleStemSize.width, height)
//                             context.lineTo(width, height)
//                             context.lineTo(width, 0)
//                             context.lineTo(chatBubbleStemSize.width, 0)
//                         }
//                     }
//                     else {
//                         if (!isOpponent) {
//                             context.moveTo(width - chatBubbleStemSize.width, 0)
//                             context.lineTo(width - chatBubbleStemSize.width, height)
//                             context.lineTo(0, height)
//                             context.lineTo(0, 0)
//                             context.lineTo(width - chatBubbleStemSize.width, 0)
//                         }
//                         else {
//                             context.moveTo(chatBubbleStemSize.width, 0)
//                             context.lineTo(width, 0)
//                             context.lineTo(width, height)
//                             context.lineTo(chatBubbleStemSize.width, height)
//                             context.lineTo(chatBubbleStemSize.width, 0)
//                         }
//                     }
// 
//                     context.fillStyle = "white"
//                     context.fill()
//                 }
//             }
//         }
//                             
//         Item {
//             Layout.fillWidth: isOpponent
//         }
//     }
// 
//     Component.onCompleted: { 
//         
//     }
// }