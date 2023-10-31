import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12

Image {
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    Dialog {
        id: sessionAddDialog
        x: (applicationWindow.width - width) / 2
        y: (applicationWindow.height - height) / 2
        implicitWidth: 500
        implicitHeight: 300
        title: "Make group name"
        modal: true

        component GroupNameDecisionView: Rectangle {
            anchors.fill: parent

            Button {
                anchors.bottom: parent.bottom
                anchors.right: nextButton.left
                text: "Cancle"

                onClicked: {
                    sessionAddDialog.close()
                }
            }

            Button {
                id: nextButton
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                text: "Next"

                onClicked: {
                    sessionAddView.push(personSelectView, StackView.Immediate)
                }
            }   
        }

        component PersonSelectView: Rectangle {
            anchors.fill: parent

            Button {
                anchors.bottom : parent.bottom
                anchors.right: confirmButton.left
                text: "Back"

                onClicked: {
                    sessionAddView.pop(StackView.Immediate)
                }
            }

            Button {
                id: confirmButton
                anchors.bottom : parent.bottom
                anchors.right: parent.right
                text: "Confirm"

                onClicked: {
                    sessionAddDialog.close()
                }
            }   
        }        

        GroupNameDecisionView {
            id: groupNameDecisionView
            visible: false
        }

        PersonSelectView {
            id: personSelectView
            visible: false            
        }

        StackView {
            id: sessionAddView
            anchors.fill: parent
            initialItem: groupNameDecisionView

            Component.onCompleted: { 
                // sessionAddView.push(groupNameMakingView.createObject(sessionAddView), StackView.Immediate)
                // sessionAddView.push(test_1, StackView.Immediate)
            }
        }

        onRejected: {
            if(sessionAddView.depth > 1)
                sessionAddView.pop(StackView.Immediate)
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            sessionAddDialog.open()
        }
    }
}