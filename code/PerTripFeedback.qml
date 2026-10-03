import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent
    clip: true

    property int totalFlags: tripController ? tripController.highRpmCount + tripController.harshAccelCount + tripController.harshBrakeCount : 0
    property real score: tripController && tripController.tripLoaded? tripController.driveScore: 0

    function scorer() {
        if (score >= 80) return "Excellent"
        if (score >= 60) return "Good"
        if (score >= 40) return "Fair"
        return "Poor"
    }

    Image {
        id: bg
        anchors.fill: parent
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/journey_bg.jpg"
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    Text {
        text: "Per-Trip Feedback"
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
            text: "|  Last Trip Summary"
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
                        value: tripController && tripController.tripLoaded
                               ? tripController.fuelUsedL.toFixed(2) + " L"
                               : "--",
                        label: "Total Fuel Used"
                    },
                    {
                        icon: "qrc:/qt/qml/FuelEfficiencyCoach/assets/bluesign.png",
                        value: tripController && tripController.tripLoaded
                               ? tripController.avgFuelEconomyLPer100Km.toFixed(1) + " L/100km"
                               : "--",
                        label: "Avg. Fuel Economy"
                    },
                    {
                        icon: "qrc:/qt/qml/FuelEfficiencyCoach/assets/yellowsign.png",
                        value: tripController && tripController.tripLoaded
                               ? root.totalFlags + " Detected"
                               : "--",
                        label: "Flags"
                    }
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
            id: ratingTitle
            text: "|  Trip Rating"
            color: "#AFC7E8"
            font.pixelSize: 26
            opacity: 0.95
            anchors.left: parent.left
            anchors.top: lastTripRow.bottom
            anchors.topMargin: 22
        }

        Rectangle {
            id: ratingLine
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: ratingTitle.bottom
            anchors.topMargin: 20
            height: 1
            color: "#BFD1EA"
            opacity: 0.18
        }

        Item {
            id: lowerArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: ratingLine.bottom
            anchors.topMargin: 30
            anchors.bottom: parent.bottom

            RowLayout {
                anchors.fill: parent
                spacing: 28

                Item {
                    id: ringPlaceholder
                    Layout.fillWidth: true
                    Layout.preferredWidth: 420
                    Layout.minimumWidth: 320
                    Layout.fillHeight: true

                    Rectangle {
                        width: Math.min(parent.width, parent.height) - 60
                        height: width
                        radius: width / 2
                        anchors.centerIn: parent
                        color: "#09121d"
                        opacity: 0.45
                        border.width: 2
                        border.color: "#2a88d8"
                    }

                    Rectangle {
                        width: Math.min(parent.width, parent.height) - 110
                        height: width
                        radius: width / 2
                        anchors.centerIn: parent
                        color: "transparent"
                        border.width: 10
                        border.color: "#18344f"
                        opacity: 0.95
                    }

                    Column {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: score.toFixed(0)
                            color: "white"
                            font.pixelSize: 64
                            font.bold: true
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: root.scorer()
                            color: "#AFC7E8"
                            font.pixelSize: 22
                            opacity: 0.9
                        }
                    }
                }

                Item {
                    id: legendArea
                    Layout.preferredWidth: 240
                    Layout.minimumWidth: 210
                    Layout.alignment: Qt.AlignVCenter
                    implicitHeight: 260

                    Rectangle {
                        id: legendBar
                        width: 10
                        height: parent.height - 10
                        radius: 5
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.9

                        gradient: Gradient {
                            GradientStop { position: 0.00; color: "#5FE8FF" }
                            GradientStop { position: 0.35; color: "#4FF5B8" }
                            GradientStop { position: 0.65; color: "#FFD56A" }
                            GradientStop { position: 1.00; color: "#FF6A3D" }
                        }
                    }

                    Column {
                        anchors.left: legendBar.right
                        anchors.leftMargin: 14
                        anchors.verticalCenter: legendBar.verticalCenter
                        spacing: 22

                        Text { text: "Excellent"; color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.85 }
                        Text { text: "Good";      color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.75 }
                        Text { text: "Fair";      color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.75 }
                        Text { text: "Poor";      color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.75 }
                    }

                    Column {
                        anchors.right: parent.right
                        anchors.verticalCenter: legendBar.verticalCenter
                        spacing: 22

                        Text { text: "80–100"; color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.75 }
                        Text { text: "60–79";  color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.65 }
                        Text { text: "40–59";  color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.65 }
                        Text { text: "0–39";   color: "#CFE3FF"; font.pixelSize: 18; opacity: 0.65 }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: "#C9D9F2"
                    opacity: 0.10
                }

                Item {
                    id: rankArea
                    Layout.fillWidth: true
                    Layout.minimumWidth: 320
                    Layout.fillHeight: true
                    clip: true

                    readonly property int totalFlags: tripController
                                                      ? tripController.highRpmCount
                                                        + tripController.harshAccelCount
                                                        + tripController.harshBrakeCount
                                                      : 0

                    Text {
                        text: "Flag Summary"
                        color: "#AFC7E8"
                        font.pixelSize: 26
                        opacity: 0.9
                        anchors.left: parent.left
                        anchors.top: parent.top
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 40
                        height: 1
                        color: "#BFD1EA"
                        opacity: 0.18
                    }

                    Column {
                        id: flagList
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 52
                        spacing: 0

                        Repeater {
                            model: [
                                {
                                    title: "High RPM",
                                    value: tripController && tripController.tripLoaded ? tripController.highRpmCount : "--"
                                },
                                {
                                    title: "Harsh Acceleration",
                                    value: tripController && tripController.tripLoaded ? tripController.harshAccelCount : "--"
                                },
                                {
                                    title: "Harsh Braking",
                                    value: tripController && tripController.tripLoaded ? tripController.harshBrakeCount : "--"
                                }
                            ]

                            delegate: Item {
                                width: flagList.width
                                height: 58

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 1
                                    color: "#BFD1EA"
                                    opacity: 0.10
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 12

                                    Text {
                                        text: index + 1
                                        Layout.preferredWidth: 28
                                        font.pixelSize: 24
                                        color: {
                                            if (index === 0) return "#7FB6FF"
                                            if (index === 1) return "#6FE0D3"
                                            if (index === 2) return "#FFD25A"
                                            return "#FF9B6A"
                                        }
                                        opacity: 0.95
                                    }

                                    Text {
                                        text: modelData.title
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        font.pixelSize: 20
                                        color: "#EAF2FF"
                                        opacity: 0.92
                                    }

                                    Text {
                                        text: modelData.value
                                        Layout.preferredWidth: 70
                                        horizontalAlignment: Text.AlignRight
                                        font.pixelSize: 20
                                        color: "#EAF2FF"
                                        opacity: 0.88
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: curvePlaceholder
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 24
        }
    }

}
