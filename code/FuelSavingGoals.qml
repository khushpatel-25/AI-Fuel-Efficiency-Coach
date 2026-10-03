import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.1

Item {
    id: root
    width: 1280
    height: 720

    // Data
    property int tripsCount: 5
    property real distanceKm: 162.0
    property real weeklyDistanceTargetKm: 200.0
    property real actualLPer100: 5.8

    Settings {
        id: persisted
        category: "fuelSavingGoals"
        property real goalLPer100: 6.5
    }

    Connections {
        target: tripController
    }

    property real draftGoalLPer100: persisted.goalLPer100

    readonly property real progressRatio: weeklyDistanceTargetKm <= 0 ? 0
                                           : Math.min(1.0, Math.max(0.0, distanceKm / weeklyDistanceTargetKm))

    readonly property real fuelSavedLitres: Math.max(0.0, (persisted.goalLPer100 - actualLPer100) * (distanceKm / 100.0))

    function clamp(x, lo, hi) { return Math.min(hi, Math.max(lo, x)); }
    function round1(x) { return Math.round(x * 10) / 10; }

    // Background
    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/journey_bg.jpg"
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#05070d"
        opacity: 0.45
    }

    // Title
    Text {
        text: "Fuel-Saving Goals"
        anchors.top: parent.top
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter

        color: "#f2f5fb"
        font.pixelSize: 30
        font.bold: true
    }

    // Main Layout
    Row {
        id: mainRow
        spacing: 18
        anchors.top: parent.top
        anchors.topMargin: 90
        anchors.horizontalCenter: parent.horizontalCenter

        // Left panel
        Rectangle {
            width: 515
            height: 410
            radius: 24
            color: "#0f163000"
            border.color: "#2b4e84"
            border.width: 1

            Column {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 14

                Text {
                    text: "Fuel Economy"
                    color: "#7fc4ff"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }

                Text {
                    text: tripController && tripController.tripLoaded
                          ? "Average fuel economy over the trip (L/100km)"
                          : "Load a trip to view fuel economy."
                    color: "#c0cadc"
                    font.pixelSize: 15
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "#27406b"
                    opacity: 0.45
                }

                Rectangle {
                    id: chartCard
                    width: parent.width
                    height: 280
                    radius: 16
                    color: "#08101fbb"
                    border.color: "#31517b"
                    border.width: 1

                    property var points: tripController ? tripController.fuelEconomySeries : []
                    property real maxY: {
                        var m = 10
                        for (var i = 0; i < points.length; ++i)
                            m = Math.max(m, points[i].y)
                        return Math.min(Math.ceil(m / 5) * 5, 60)
                    }

                    Canvas {
                        id: fuelCanvas
                        anchors.fill: parent
                        anchors.margins: 12

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.reset()

                            var pts = chartCard.points
                            var w = width
                            var h = height

                            var leftPad = 40
                            var rightPad = 10
                            var topPad = 10
                            var bottomPad = 28

                            var plotW = w - leftPad - rightPad
                            var plotH = h - topPad - bottomPad

                            ctx.strokeStyle = "#31517b"

                            for (var g = 0; g <= 4; ++g) {
                                var gy = topPad + (plotH * g / 4)
                                ctx.beginPath()
                                ctx.moveTo(leftPad, gy)
                                ctx.lineTo(leftPad + plotW, gy)
                                ctx.stroke()
                            }

                            ctx.strokeStyle = "#8fb7ff"
                            ctx.beginPath()
                            ctx.moveTo(leftPad, topPad)
                            ctx.lineTo(leftPad, topPad + plotH)
                            ctx.lineTo(leftPad + plotW, topPad + plotH)
                            ctx.stroke()

                            if (!pts || pts.length < 2)
                                return

                            var minX = pts[0].x
                            var maxX = pts[pts.length - 1].x
                            if (maxX <= minX)
                                maxX = minX + 1

                            var maxY = Math.max(5, chartCard.maxY)

                            function sx(x) {
                                return leftPad + ((x - minX) / (maxX - minX)) * plotW
                            }

                            function sy(y) {
                                return topPad + plotH - (Math.min(y, maxY) / maxY) * plotH
                            }

                            ctx.strokeStyle = "#79d2ff"
                            ctx.lineWidth = 2
                            ctx.beginPath()
                            ctx.moveTo(sx(pts[0].x), sy(pts[0].y))

                            for (var i = 1; i < pts.length; ++i)
                                ctx.lineTo(sx(pts[i].x), sy(pts[i].y))

                            ctx.stroke()
                            // axis titles
                            ctx.fillStyle = "#c0cadc"
                            ctx.font = "14px sans-serif"

                            // X-axis title
                            ctx.textAlign = "center"
                            ctx.textBaseline = "bottom"
                            ctx.fillText("Time / Distance", leftPad + plotW / 2, topPad + plotH + 24)

                            // Y-axis title (rotated)
                            ctx.save()
                            ctx.translate(14, topPad + plotH / 2)
                            ctx.rotate(-Math.PI / 2)
                            ctx.textAlign = "center"
                            ctx.textBaseline = "top"
                            ctx.fillText("Fuel (L/100km)", 0, 0)
                            ctx.restore()
                        }

                        Connections {
                            target: tripController
                            function onFuelEconomySeriesChanged() { fuelCanvas.requestPaint() }
                            function onTripChanged() { fuelCanvas.requestPaint() }
                        }
                    }
                }
            }
        }

        // Right panel
        Rectangle {
            width: 515
            height: 410
            radius: 24
            color: "#0f163000"
            border.color: "#2b4e84"
            border.width: 1

            Column {
                anchors.fill: parent
                anchors.margins: 28
                spacing: 16

                Text {
                    text: "AI Guidance"
                    color: "#7fc4ff"
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "Advice powered by IBM Granite to improve your fuel efficiency."
                    color: "#c0cadc"
                    font.pixelSize: 13
                }

                Text {
                    text: tripController && tripController.aiLoaded
                          ? tripController.aiGuidance1
                          : "Submit a drive to get AI advice."
                    color: "#dce7f7"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                Text {
                    text: tripController && tripController.aiLoaded ? tripController.aiGuidance2 : ""
                    color: "#dce7f7"
                    font.pixelSize: 15
                }

                Text {
                    text: tripController && tripController.aiLoaded ? tripController.aiGuidance3 : ""
                    color: "#dce7f7"
                    font.pixelSize: 15
                }
            }
        }
    }

    // Bottom Summary
    Row {
        spacing: 120
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 28
        anchors.horizontalCenter: parent.horizontalCenter

        Column {
            spacing: 6
            Text { text: "Average Fuel Usage"; color: "#c6d0e2"; font.pixelSize: 16 }
            Text {
                text: tripController ? tripController.avgFuelEconomyLPer100Km.toFixed(1) + " L/100km" : "--"
                color: "white"
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
        }

        Rectangle { width: 1; height: 74; color: "#27406b"; opacity: 0.45 }

        Column {
            spacing: 6
            Text { text: "Fuel Used"; color: "#c6d0e2"; font.pixelSize: 16 }
            Text {
                text: tripController ? tripController.fuelUsedL.toFixed(2) + " L" : "--"
                color: "white"
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
        }
    }
}
