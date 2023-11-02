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

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: sessionAddDialog.implicitHeight * 0.7
                    spacing: 0

                    CustomImageButton {
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                        Layout.fillWidth: true
                        Layout.preferredHeight: width
                        imageSource: "qrc:/icon/UserID.png"
                        bgColor: "transparent"
                        imageColor: "transparent"
                    }

                    ColumnLayout {
                        // Layout.preferredWidth: sessionAddDialog.implicitWidth * 0.65

                        Text {
                            // Layout.fillWidth: true
                            // Layout.fillHeight: true                            
                            text: "Group Name"
                        }

                        Rectangle {
                            Layout.preferredWidth: sessionAddDialog.implicitWidth * 0.65
                            Layout.preferredHeight: sessionAddDialog.implicitHeight * 0.1                        
                            radius: 5
                            color: "#cccccc"

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
                    }
                }

                RowLayout {
                    // Layout.fillWidth: true            
                    // Layout.fillHeight: true
                    spacing: 0

                    Item {
                        Layout.fillWidth: true    
                    }
                    Button {
                        text: "Cancle"

                        onClicked: {
                            sessionAddDialog.close()
                        }
                    }
                    Button {
                        text: "Next"

                        onClicked: {
                            sessionAddView.push(personSelectView, StackView.Immediate)
                        }
                    }                      
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