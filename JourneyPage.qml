import QtQuick 2.15

import QtQuick.Dialogs

Item {
    anchors.fill: parent

    Connections {
        target: tripController

        function onTripChanged() {
            errorBanner.visible = false
        }

        function onErrorOccurred(message) {
            console.log("Trip load error:", message)
            errorBanner.text = message
            errorBanner.visible = true
        }
    }

    Text {
        id: errorBanner
        visible: false
        anchors.top: parent.top
        anchors.topMargin: 90
        anchors.horizontalCenter: parent.horizontalCenter
        color: "#ff7b7b"
        font.pixelSize: 16
        text: ""
    }

    FileDialog {
        id: csvDialog
        title: "Select trip CSV"
        nameFilters: ["CSV files (*.csv)"]

        onAccepted: {
            if (selectedFile)
                tripController.loadTrip(selectedFile)
        }
    }
    
    //background
    Image {
        id: bg
        anchors.fill: parent
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/journey_bg.jpg"
    }

    Column {
        id: leftMenu
        anchors.left: parent.left
        anchors.leftMargin: 110
        anchors.top: parent.top
        anchors.topMargin: 170
        spacing: 18
    }

    //tiitle

    Rectangle {
        id: uploadButton
        width: 150
        height: 42
        radius: 10
        color: "#17304a"
        border.color: "#4aa3ff"
        border.width: 1

        anchors.top: parent.top
        anchors.topMargin: 36
        anchors.right: parent.right
        anchors.rightMargin: 60

        Text {
            anchors.centerIn: parent
            text: tripController.tripLoaded ? "Load New CSV" : "Upload CSV"
            color: "white"
            font.pixelSize: 16
            font.bold: true
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: csvDialog.open()
        }
    }

    Text {
        anchors.top: uploadButton.bottom
        anchors.topMargin: 10
        anchors.right: uploadButton.right
        color: "#cfe3ff"
        font.pixelSize: 14
        opacity: 0.8
        text: tripController.tripLoaded ? tripController.tripFileName : "No trip loaded"
    }

    Text {
        text: "Live Journey Tracker"
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        color: "white"
        font.pixelSize: 30
        font.bold: true
    }

    Item {
        id: ringPlaceholder
        width: 360
        height: 360
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 150

    }

    Rectangle {
        id: efficiencyCard
        width: Math.max(250, Math.min(parent.width * 0.26, 360))
        height: Math.max(180, Math.min(parent.height * 0.26, 250))
        radius: Math.max(14, width * 0.06)
        color: "#08111d"
        border.color: "#2f78c7"
        border.width: 1
        opacity: 0.92

        anchors.left: parent.left
        anchors.leftMargin: 110
        anchors.top: parent.top
        anchors.topMargin: 150

        Column {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 10

            Text {
                text: "Fuel Efficiency"
                color: "#AFC7E8"
                font.pixelSize: 22
                font.bold: true
            }

            Text {
                text: tripController && tripController.liveFuelEfficiencyPercent > 0
                      ? tripController.liveFuelEfficiencyPercent.toFixed(0) + "%"
                      : "--"
                color: "white"
                font.pixelSize: 38
                font.bold: true
            }

            Text {
                text: "Benchmark: 6.5L/100km"
                color: "#AFC7E8"
                font.pixelSize: 18
                opacity: 0.8
            }

            Text {
                text: tripController && tripController.liveFuelEconomyLPer100Km > 0
                      ? tripController.liveFuelEconomyLPer100Km.toFixed(1) + " L/100km"
                      : "--"
                color: "white"
                font.pixelSize: 20
                opacity: 0.9
            }
        }
    }

    // car
    Image {
        id: car
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/car.png"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 120
        width: 420
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    // fuel distance speed
    Column {
        id: rightStats
        anchors.right: parent.right
        anchors.rightMargin: 150
        anchors.top: parent.top
        anchors.topMargin: 210
        spacing: 26

        function statRow(iconSrc, valueText, labelText) {
            return { icon: iconSrc, value: valueText, label: labelText }
        }

        Repeater {
            model: [
                rightStats.statRow(
                    "qrc:/qt/qml/FuelEfficiencyCoach/assets/distance.jpg",
                    tripController ? tripController.distanceTravelledKm.toFixed(2) + " KM" : "--",
                    "Distance So Far"
                ),
                rightStats.statRow(
                    "qrc:/qt/qml/FuelEfficiencyCoach/assets/fuel.jpg",
                    tripController ? Math.floor(tripController.timeTravelledSeconds / 60) + " min" : "--",
                    "Time Travelled"
                ),
                rightStats.statRow(
                    "qrc:/qt/qml/FuelEfficiencyCoach/assets/speed.jpg",
                    tripController ? tripController.avgSpeedKmh.toFixed(1) + " km/h" : "--",
                    "Avg Speed"
                )
            ]

            delegate: Row {
                spacing: 14

                Image {
                    source: modelData.icon
                    width: 46
                    height: 46
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                Column {
                    spacing: 2

                    Text {
                        text: modelData.value
                        color: "white"
                        font.pixelSize: 30
                        font.bold: true
                    }

                    Text {
                        text: modelData.label
                        color: "white"
                        font.pixelSize: 18
                        opacity: 0.7
                    }
                }
            }
        }
    }

    Item {
        id: progressArea
        width: Math.max(360, Math.min(parent.width * 0.46, 620))
        height: Math.max(60, parent.height * 0.10)

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.07

        property int totalBars: 12
        property int activeBars: tripController
                                 ? Math.round((tripController.journeyProgress / 100) * totalBars)
                                 : 0

        Text {
            id: progressLabel
            anchors.left: parent.left
            anchors.verticalCenter: bars.verticalCenter
            text: "Progress"
            color: "white"
            font.pixelSize: Math.max(20, progressArea.width * 0.055)
            opacity: 0.9
        }

        Row {
            id: bars
            anchors.left: progressLabel.right
            anchors.leftMargin: Math.max(18, progressArea.width * 0.04)
            anchors.right: progressValue.left
            anchors.rightMargin: Math.max(18, progressArea.width * 0.04)
            anchors.verticalCenter: parent.verticalCenter
            spacing: Math.max(3, progressArea.width * 0.008)

            Repeater {
                model: progressArea.totalBars

                delegate: Rectangle {
                    width: (bars.width - (progressArea.totalBars - 1) * bars.spacing) / progressArea.totalBars
                    height: Math.max(10, progressArea.height * 0.18)
                    radius: height / 3
                    opacity: 0.9
                    color: index < progressArea.activeBars ? "#4aa3ff" : "#24415f"
                }
            }
        }

        Text {
            id: progressValue
            anchors.right: parent.right
            anchors.verticalCenter: bars.verticalCenter
            color: "white"
            font.pixelSize: Math.max(24, progressArea.width * 0.07)
            font.bold: true
            text: tripController
                  ? Math.round(tripController.journeyProgress) + "%"
                  : "0%"
        }
    }
}
