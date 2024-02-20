import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Item {
    property int contentMargin: 3

    width: parent.width
    height: chatLayout.spacing + (isOpponent ? chatSenderText.paintedHeight : 0) + chatBackground.height

    RowLayout {       
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: !isOpponent
        }

        Image {
            source: "file:///" + senderImgPath // mainContext.getUserImgById(senderId)  // "data:image/png;base64," + senderImg
            visible: isOpponent
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
        }

        ColumnLayout {
            id: chatLayout
            spacing: 3

            Text {
                id: chatSenderText
                text: senderName // mainContext.getUserNameById(senderId)
                font.pixelSize: 15
                visible: isOpponent
                Layout.leftMargin: chatBubbleStemSize.width
            }

            Canvas {
                id: chatBackground
                Layout.preferredWidth: Math.max(chatContent.width, chatDateText.paintedWidth) + chatBubbleStemSize.width + contentMargin * 2
                Layout.preferredHeight: chatContent.height + chatDateText.paintedHeight + contentMargin * 2

                Item {
                    id: chatContent
                    x: isOpponent ? (chatBubbleStemSize.width + contentMargin) : contentMargin
                    y: contentMargin
                    width: Math.min(chatBubbleMaximumWidth, Math.max(chatBubbleMinimumSize.width, Math.max(dummyText.paintedWidth, chatDateText.paintedWidth)))
                    height: Math.max(chatContentTextEdit.paintedHeight, chatBubbleMinimumSize.height)

                    TextEdit {
                        id: chatContentTextEdit
                        anchors.verticalCenter: parent.verticalCenter 
                        anchors.left: parent.left
                        width: parent.width
                        height: parent.height
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
                    anchors.top: chatContent.bottom
                    anchors.left: isOpponent ? undefined : chatContent.left
                    anchors.right: isOpponent ? chatContent.right : undefined
                    text: sendDate
                }

                onPaint: {
                    var context = getContext("2d")
                    context.beginPath()

                    var noStem = parent.model.count == 1 ? true : false

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

                    context.fillStyle = "white"
                    context.fill()
                }
            }
        }
                            
        Item {
            Layout.fillWidth: isOpponent
        }
    }

    Component.onCompleted: { 
        
    }
}