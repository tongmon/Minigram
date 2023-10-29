import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Item {
    property int chatBubbleMargin: 3

    width: parent.width
    height: chatBubbleLayout.spacing + (isOpponent ? chatBubbleSenderText.paintedHeight : 0) + chatBubbleBackground.height

    RowLayout {       
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: !isOpponent
        }

        Image {
            source: "data:image/png;base64," + senderImage
            visible: isOpponent
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 50
            Layout.preferredHeight: 50
        }

        ColumnLayout {
            id: chatBubbleLayout
            spacing: 3

            Text {
                id: chatBubbleSenderText
                text: senderName
                font.pixelSize: 15
                visible: isOpponent
                Layout.leftMargin: chatBubbleStemSize.width
            }

            Canvas {
                id: chatBubbleBackground
                Layout.preferredWidth: Math.max(chatBubbleContent.width, dateText.paintedWidth) + chatBubbleStemSize.width + chatBubbleMargin * 2
                Layout.preferredHeight: chatBubbleContent.height + dateText.paintedHeight + chatBubbleMargin * 2

                Item {
                    id: chatBubbleContent
                    x: isOpponent ? (chatBubbleStemSize.width + chatBubbleMargin) : chatBubbleMargin
                    y: chatBubbleMargin
                    width: Math.min(chatBubbleMaximumWidth, Math.max(chatBubbleMinimumSize.width, dummyText.paintedWidth))
                    height: Math.max(chatBubbleText.paintedHeight, chatBubbleMinimumSize.height)

                    TextEdit {
                        id: chatContentTextEdit
                        anchors.verticalCenter: parent.verticalCenter 
                        anchors.left: parent.left
                        width: parent.width
                        height: parent.height
                        text: chatContent
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
                    anchors.top: chatBubbleContent.bottom
                    anchors.left: isOpponent ? undefined : chatBubbleContent.left
                    anchors.right: isOpponent ? chatBubbleContent.right : undefined
                    text: chatDate
                }

                onPaint: {
                    var context = getContext("2d")
                    context.beginPath()

                    if(!isOpponent) {
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
        console.log("Chatbubble created! width: " + width + " height: " + height)
    }
}