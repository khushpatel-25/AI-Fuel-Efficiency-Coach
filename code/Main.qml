import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 1280
    height: 720
    minimumWidth: 1180
    minimumHeight: 680
    visible: true
    color: "#0b0f14"
    title: qsTr("Fuel Efficiency Coach")

    property int pageIndex: 41
    readonly property int sidebarWidth: 170
    readonly property int contentGap: 14

    function syncLoadedItem() {
        if (pageLoader.item) {
            pageLoader.item.x = 0
            pageLoader.item.y = 0
            pageLoader.item.width = contentViewport.width
            pageLoader.item.height = contentViewport.height
        }
    }

    // Main page area
    Item {
        id: contentViewport
        anchors.fill: parent
        anchors.leftMargin: sidebarWidth + contentGap
        clip: true

        Loader {
            id: pageLoader
            anchors.fill: parent
            source: pageIndex === 0 ? "JourneyPage.qml"
                  : pageIndex === 1 ? "PerTripFeedback.qml"
                  : pageIndex === 2 ? "TripHistory.qml"
                  : pageIndex === 3 ? "FuelSavingGoals.qml"
                  : pageIndex === 41 ? "AIDownloading.qml"
                  : ""

            onLoaded: root.syncLoadedItem()
            onWidthChanged: root.syncLoadedItem()
            onHeightChanged: root.syncLoadedItem()
        }
    }

    // Sidebar
    Rectangle {
        id: sidebar
        z: 100
        width: sidebarWidth
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: "#0c1117"
        border.color: "#18222d"
        border.width: 1
        opacity: 0.98

        Column {
            anchors.top: parent.top
            anchors.topMargin: 90
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 16

            Text {
                text: "Fuel Coach"
                color: "#8ea4bf"
                font.pixelSize: 18
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            NavItem {
                label: "Journey"
                selected: pageIndex === 0
                onClicked: if (!disabled) pageIndex = 0
                disabled: pageIndex == 41
            }

            NavItem {
                label: "Trip Feedback"
                selected: pageIndex === 1
                onClicked: if (!disabled) pageIndex = 1
                disabled: pageIndex == 41
            }

            NavItem {
                label: "Trip History"
                selected: pageIndex === 2
                onClicked: if (!disabled) pageIndex = 2
                disabled: pageIndex == 41
            }

            NavItem {
                label: "Fuel Goals"
                selected: pageIndex === 3
                onClicked: if (!disabled) pageIndex = 3
                disabled: pageIndex == 41
            }
        }
    }

    component NavItem : Item {
        required property string label
        property bool selected: false
        property bool disabled
        signal clicked()

        width: 138
        height: 42

        Rectangle {
            anchors.fill: parent
            radius: 12
            color: selected ? "#17304a" : (disabled && mouse.containsMouse ? "#1B1E21" : (mouse.containsMouse ? "#121a23" : "transparent"))
            border.width: 1
            border.color: selected ? "#4aa3ff" : "transparent"

            Behavior on color {
                ColorAnimation { duration: 140 }
            }

            Behavior on border.color {
                ColorAnimation { duration: 140 }
            }
        }

        Rectangle {
            width: 4
            height: parent.height - 12
            radius: 2
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            color: selected ? "#4aa3ff" : "transparent"

            Behavior on color {
                ColorAnimation { duration: 140 }
            }
        }

        Text {
            anchors.centerIn: parent
            width: parent.width - 24
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: label
            color: selected ? "white" : (disabled ? "#aaaaaa" : "#d7e3f4")
            font.pixelSize: 15
            font.bold: selected
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
