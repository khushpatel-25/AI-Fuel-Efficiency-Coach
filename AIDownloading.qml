import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent

    property double downloadPercentage: aiController
                                        ? aiController.downloadPercentage
                                        : 0.0
    property double downloadFilesize: aiController
                                      ? aiController.downloadFilesize
                                      : 4.0

    Connections {
        target: aiController
    }

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            downloadFilesize = aiController.getDownloadTotal() ?? 3
        }
    }

    Image {
        id: bg
        anchors.fill: parent
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/journey_bg.jpg"
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    Text {
        text: "IBM Granite - Downloading"
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        color: "white"
        font.pixelSize: 30
        font.bold: true
    }

    Item {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 70
        anchors.rightMargin: 70
        anchors.topMargin: 110
        anchors.bottomMargin: 56

        Text {
            id: lastTripTitle
            text: "|  Downloading IBM Granite 4.0-micro from Huggingface"
            color: "#AFC7E8"
            font.pixelSize: 26
            opacity: 0.95
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 10
        }

        Rectangle {
            id: lastTripLine
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: lastTripTitle.bottom
            anchors.topMargin: 22
            height: 1
            color: "#BFD1EA"
            opacity: 0.18
        }

        RowLayout {
            id: lastTripRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: lastTripLine.bottom
            anchors.topMargin: 18
            height: 72
            spacing: 0

            Repeater {
                model: [
                    {
                        icon: "qrc:/qt/qml/FuelEfficiencyCoach/assets/fuelsign.png",
                        value: "Progress",
                        label: (100 * root.downloadPercentage / root.downloadFilesize).toFixed(1) + "%"
                    },
                    {
                        icon: "qrc:/qt/qml/FuelEfficiencyCoach/assets/fuelsign.png",
                        value: "File Information",
                        label: "(" + Math.round(root.downloadPercentage / 10000000.0) / 100.0 + "GB / " + Math.round(root.downloadFilesize / 10000000.0) / 100.0 + "GB) granite-4.0-micro-Q4_K_M.gguf"
                    },
                ]

                delegate: Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 220
                    Layout.preferredHeight: lastTripRow.height

                    Rectangle {
                        visible: index !== 0
                        width: 1
                        height: parent.height * 0.78
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#C9D9F2"
                        opacity: 0.12
                    }

                    Row {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10

                        Image {
                            width: 60
                            height: 60
                            anchors.verticalCenter: parent.verticalCenter
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            opacity: 0.75
                            source: modelData.icon
                        }

                        Column {
                            width: parent.width - 80
                            spacing: 2
                            anchors.verticalCenter: parent.verticalCenter

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.value
                                color: "#EAF2FF"
                                font.pixelSize: 22
                                opacity: 0.95
                            }

                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: modelData.label
                                color: "#A9C0E1"
                                font.pixelSize: 16
                                opacity: 0.75
                            }
                        }
                    }
                }
            }     
        }

        Text {
            text: "|  This may take a while. You will be able to use the app shortly."
            anchors.top: lastTripRow.bottom
            anchors.topMargin: 24
            Layout.preferredWidth: 28
            color: "#AFC7E8"
            font.pixelSize: 26
            opacity: 0.95
        }
    }
}
