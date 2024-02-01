import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.12
import minigram.contact.component 1.0
import minigram.chatsession.component 1.0

ApplicationWindow {
    id: applicationWindow
    objectName: "applicationWindow"
    visible: true
    width: 1400
    height: 900
    minimumWidth: 600
    minimumHeight: 400
    title: qsTr("Messenger")
    flags: Qt.Window
    
    // 창 크기 재조절 외각 부분, C++에서 받아서 처리
    property int resizeBorderWidth: 6

    property string userID
    property string userPW

    ContactModel {
        id: contactModel
        objectName: "contactModel"

        Component.onCompleted: {
            // 테스트 코드
            // for(let i = 0; i < 20; i++) {
            //     contactModel.append({
            //         "userId": "tongstar_" + i,
            //         "userImg": "qrc:/icon/UserID.png",
            //         "userName": "KyungJoonLee_" + i,
            //         "userInfo": i
            //     })
            // }
        }
    }

    ContactSortFilterProxyModel {
        id: contactSortFilterProxyModel
        objectName: "contactSortFilterProxyModel"
        sourceModel: contactModel
        dynamicSortFilter: true // sort() 함수가 한번이라도 불렸을 때 효과있음
        sortRole: 258
        filterRole: 258
    }

    ContactModel {
        id: contactRequestModel
        objectName: "contactRequestModel"

        Component.onCompleted: {
            // 테스트 코드
            //for(let i = 0; i < 20; i++) {
            //    contactRequestModel.append({
            //        "userId": "tongstar_" + i,
            //        "userImg": "qrc:/icon/UserID.png",
            //        "userName": "KyungJoonLee_" + i,
            //        "userInfo": i
            //    })
            //}
        }
    }

    ContactSortFilterProxyModel {
        id: contactRequestFilterProxyModel
        objectName: "contactRequestFilterProxyModel"
        sourceModel: contactRequestModel
        dynamicSortFilter: true
        sortRole: 258
        filterRole: 258
    }

    ChatSessionModel {
        id: chatSessionModel
        objectName: "chatSessionModel"
    }

    ChatSessionSortFilterProxyModel {
        id: chatSessionSortFilterProxyModel
        objectName: "chatSessionSortFilterProxyModel"
        sourceModel: chatSessionModel
        dynamicSortFilter: true
        sortRole: 261
        filterRole: 258
    }

    // 메인 뷰
    Loader { 
        id: mainWindowLoader 
        anchors.fill: parent
        objectName: "mainWindowLoader"
        // source: "qrc:/qml/LoginPage.qml" // "qrc:/qml/MainPage.qml" "qrc:/qml/LoginPage.qml"

        // onLoaded: {
        //     mainContext.getEssentialObjects()
        // }

        onLoaded: {
            if (typeof(mainContext) !== "undefined" && mainContext)
            {
                if (source.toString().indexOf('MainPage') < 0) 
                    return

                mainContext.tryGetContactList()
                mainContext.tryGetContactRequestList()
            }
        }
    }

    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }
}
