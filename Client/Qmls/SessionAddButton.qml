import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import QtGraphicalEffects 1.15
import minigram.contact.component 1.0

Image {
    id: sessionAddButton
    objectName: "sessionAddButton"
    height: parent.height
    width: height
    source: "qrc:/icon/UserID.png"

    property string groupName: ""
    property var addedPerson: ({})
    
    Popup {
        id: groupNameDecisionPopup
        x: (applicationWindow.width - width) / 2 - sessionAddButton.x
        y: (applicationWindow.height - height) / 2 - sessionAddButton.y
        width: 500
        height: 300
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

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
                            source: "qrc:/icon/UserID.png"
                            overlayColor: Qt.rgba(100, 100, 100, hovered ? 0.7 : 0)
                            rounded: true

                            onClicked: {
                                var selected_files = mainContext.executeFileDialog({
                                    "title": "Selete profile image",
                                    "initDir": ".",
                                    "filter": "Image File(*.png)\0*.png\0",
                                    "maxFileCnt": 1
                                })
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
            sessionImageSelectButton.source = "qrc:/icon/UserID.png"
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
                    Layout.preferredHeight: 50
                    orientation: ListView.Horizontal
                    clip: true
                    visible: selectedPersonModel.count ? true : false
                    
                    ScrollBar.horizontal: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: ListModel {
                        id: selectedPersonModel
                    }

                    delegate: Rectangle {
                        id: selectedPerson
                        height: parent.height
                        width: userImage.width + userNameText.width + selectedPersonRemoveButton.width
                        objectName: userId
                        color: "#B240F5"

                        property int selectedPersonIndex: index

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Image {
                                id: userImage
                                Layout.preferredHeight: selectedPerson.height * 0.8
                                Layout.preferredWidth: height
                                source: userImageSource
                                fillMode: Image.PreserveAspectFit

                                layer {
                                    enabled: true
                                    effect: OpacityMask {
                                        maskSource: Item {
                                            width: userImage.width
                                            height: userImage.height
                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: userImage.width
                                                height: userImage.height
                                                radius: Math.min(width, height)
                                            }
                                        }
                                    }
                                }
                            }

                            Text {
                                id: userNameText
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: selectedPerson.objectName
                            }

                            Button {
                                id: selectedPersonRemoveButton
                                Layout.preferredHeight: selectedPerson.height * 0.8
                                Layout.preferredWidth: height
                                text: "Del"

                                onClicked: {
                                    delete addedPerson[selectedPerson.objectName];
                                    selectedPersonModel.remove(selectedPerson.selectedPersonIndex)
                                }
                            }
                        }
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

                            onTextChanged: {
                                contactSortFilterProxyModel.filterRegex = userNameSearchField.text
                            }

                            Keys.onReturnPressed: {
                            
                            }

                            Component.onCompleted: {
                            }
                        }
                    }
                }

                //ContactModel {
                //    id: contactModel
                //}

                ListView {
                    id: userListView
                    Layout.fillWidth: true 
                    Layout.fillHeight: true
                    clip: true

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    model: contactSortFilterProxyModel
                    //model: ContactSortFilterProxyModel {
                    //    id: contactSortFilterProxyModel
                    //    sourceModel: contactModel
                    //    dynamicSortFilter: true // sort() 함수가 한번이라도 불렸을 때 효과있음
                    //    sortRole: 258
                    //    filterRole: 258
                    //}

                    delegate: Rectangle {
                        id: userData
                        height: 100
                        width: parent.width
                        objectName: userId
                        color: "#B240F5"

                        property int personIndex: index

                        RowLayout {
                            anchors.fill: parent
                            spacing: 0

                            Image {
                                id: userImage
                                Layout.alignment: Qt.AlignVCenter 
                                Layout.preferredHeight: userData.height * 0.8
                                Layout.preferredWidth: height
                                source: userImg
                                fillMode: Image.PreserveAspectFit

                                layer {
                                    enabled: true
                                    effect: OpacityMask {
                                        maskSource: Item {
                                            width: userImage.width
                                            height: userImage.height

                                            Rectangle {
                                                anchors.centerIn: parent
                                                width: userImage.width
                                                height: userImage.height
                                                radius: Math.min(width, height)
                                            }
                                        }
                                    }
                                }
                            }

                            Item {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                
                                ColumnLayout {
                                    anchors.fill: parent

                                    Text {
                                        text: userName
                                    }
                                    Text {
                                        text: userData.objectName
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true

                            onClicked: {
                                if(!addedPerson.hasOwnProperty(userId)) {
                                    selectedPersonModel.append({
                                        "userId": userId,
                                        "userImageSource": userImg
                                    })
                                    addedPerson[userId] = true
                                }
                            }
                        }

                        Component.onCompleted: {
                            // console.log("Created!")
                        }
                    }

                    // 테스트 코드
                    Component.onCompleted: {
                        
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
                                var img_path = sessionImageSelectButton.source.toString()
                                switch (img_path[0])
                                {
                                // 파일 선택한 경우
                                case 'f':
                                    img_path = decodeURIComponent(img_path.replace(/^(file:\/{3})/,""));
                                    break
                                // qrc, http 등... 유저가 세션 이미지를 따로 선택하지 않은 경우
                                default:
                                    img_path = ""
                                    break
                                }

                                mainContext.tryAddSession(groupName, img_path, Object.keys(addedPerson))
                                groupNameDecisionPopup.close()
                                contactChoicePopup.close()
                            }
                        }   
                    }
                }
            }
        }

        // 테스트 코드
        onOpened: {
            contactModel.append({
                "userId": "tongstar",
                "userImg": "file:///C:/Users/DP91-HSK/Pictures/Saved Pictures/profile.png",
                "userName": "abratacabra",
                "userInfo": ""
            })

            contactModel.append({
                "userId": "yellowjam",
                "userImg": "file:///C:/Users/DP91-HSK/Pictures/Saved Pictures/profile.png",
                "userName": "kingofkfc",
                "userInfo": ""
            }) 

            contactModel.append({
                "userId": "zebra",
                "userImg": "file:///C:/Users/DP91-HSK/Pictures/Saved Pictures/profile.png",
                "userName": "yellowyolo",
                "userInfo": ""
            })         
        }

        onClosed: {
            userNameSearchField.text = ""
            addedPerson = ({})
            selectedPersonModel.clear()
            contactModel.clear()
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            groupNameDecisionPopup.open()
        }
    }
}