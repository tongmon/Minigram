import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15

Image {
    id: sessionAddButton
    objectName: "sessionAddButton"
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    property string groupName: ""
    
    Popup {
        id: groupNameDecisionPopup
        x: (applicationWindow.width - width) / 2 - sessionAddButton.x
        y: (applicationWindow.height - height) / 2 - sessionAddButton.y
        width: 500
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        contentItem: Rectangle {
            anchors.fill: parent
            
            ColumnLayout {
                anchors.fill: parent

                Rectangle {
                    Layout.preferredHeight: groupNameDecisionPopup.height * 0.7
                    Layout.fillWidth: true   
                    color: "transparent"
                    
                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        CustomImageButton {
                            id: sessionImageSelectButton
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredWidth: groupNameDecisionPopup.width * 0.25
                            Layout.preferredHeight: width
                            source: "file:/C:/Users/DP91-HSK/Pictures/Saved Pictures/profile.png" // "qrc:/icon/UserID.png"
                            overlayColor: Qt.rgba(100, 100, 100, hovered ? 0.7 : 0)
                            rounded: true

                            onClicked: {
                                var selected_files = mainContext.executeFileDialog(".", "Image File(*.png)\0*.png\0", 1);
                                if(selected_files.length)
                                    source = "file:///" + selected_files[0]
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.fillHeight: true 
                            color: "transparent"

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 0
                                
                                Item {             
                                    Layout.fillHeight: true
                                }
                                Text {                         
                                    text: "Group Name"
                                }
                                Rectangle {               
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: sessionImageSelectButton.height * 0.2        
                                    Layout.rightMargin: 5   
                                    Layout.topMargin: 8             
                                    radius: 5
                                    color: "#cccccc"

                                    TextField {
                                        id: groupNameTextField
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
                                Item {             
                                    Layout.fillHeight: true
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillHeight: true             
                    Layout.fillWidth: true

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true    
                        }
                        Button {
                            Layout.alignment: Qt.AlignVCenter
                            text: "Cancle"

                            onClicked: {
                                groupNameDecisionPopup.close()
                            }
                        }
                        Button {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.rightMargin: 5
                            text: "Next"

                            onClicked: {
                                if (groupNameTextField.text.length > 0) {
                                    sessionAddButton.groupName = groupNameTextField.text
                                    contactChoicePopup.open()
                                }
                            }
                        }                      
                    }
                }
            }
        }

        onClosed: {
            groupNameTextField.text = ""
        }
    }

    Popup {
        id: contactChoicePopup
        x: (applicationWindow.width - width) / 2 - sessionAddButton.x
        y: (applicationWindow.height - height) / 2 - sessionAddButton.y
        width: 500
        height: 700
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

        contentItem: Rectangle {
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                
                ListView {
                    id: selectedPersonView
                    Layout.fillWidth: true     
                    Layout.preferredHeight: contactChoicePopup.height * 0.15
                    orientation: ListView.Horizontal 
                    clip: true
                    
                    ScrollBar.horizontal: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: ListModel {
                        id: selectedPersonModel
                    }

                    delegate: Rectangle {
                        height: parent.height
                        width: 100
                        objectName: userID
                        color: "#B240F5"

                        property int selectedPersonIndex: index
                    }
                }

                Rectangle {
                    Layout.fillWidth: true 
                    Layout.preferredHeight: contactChoicePopup.height * 0.1
                    color: "transparent"

                    Rectangle {               
                        anchors {
                            fill: parent
                            margins: 5
                        }          
                        radius: 5
                        color: "#cccccc"

                        TextField {
                            id: userNameSearchField
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

                ListView {
                    id: contactListView
                    Layout.fillWidth: true 
                    Layout.fillHeight: true
                    clip: true

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: ListModel {
                        id: contactModel
                    }

                    delegate: Rectangle {
                        height: 100
                        width: parent.width
                        objectName: userID
                        color: "#B240F5"

                        property int personIndex: index
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: contactChoicePopup.height * 0.1
                    color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.fillWidth: true    
                        }
                        Button {
                            Layout.alignment: Qt.AlignVCenter
                            text: "Cancle"

                            onClicked: {
                                contactChoicePopup.close()
                            }
                        }
                        Button {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.rightMargin: 5
                            text: "Confirm"

                            onClicked: {
                                groupNameDecisionPopup.close()
                                contactChoicePopup.close()
                            }
                        }   
                    }
                }
            }
        }

        onClosed: {
            userNameSearchField.text = ""
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            groupNameDecisionPopup.open()
        }
    }
}