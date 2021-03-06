import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

import Firebase 1.0


ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    FirebaseApp{
        id: firebaseApp
        Component.onCompleted: {
            console.log("FirebaseApp "+ready)
        }
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        Messaging{}
        Database{}
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        TabButton {
            text: qsTr("FCM")
        }
        TabButton {
            text: qsTr("Database")
        }
    }
}
