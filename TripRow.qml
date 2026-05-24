import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

Item {
    id: root
    property string dateText: ""
    property string name: ""
    property int distanceKm: 0
    property real fuelL: 0
    property int score: 0

    height: 78

    Rectangle {
        anchors.fill: parent
        radius: 16
        color: "#0c1628"
        border.color: "#253750"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 18

            // Left: date
            ColumnLayout {
                Layout.preferredWidth: 170
                spacing: 2
                Label { text: dateText; color: "#9fb4d6"; font.pixelSize: 13 }
                Label { text: dateText.split(",")[0].replace("2023",""); color: "#cfe2ff"; font.pixelSize: 20 }
            }

            // Route
            Label {
                Layout.fillWidth: true
                text: name
                color: "#cfe2ff"
                font.pixelSize: 18
                elide: Text.ElideRight
            }

            // Distance block
            ColumnLayout {
                Layout.preferredWidth: 120
                spacing: 2
                Label { text: distanceKm + " KM"; color: "#cfe2ff"; font.pixelSize: 18 }
                Label { text: "Distance"; color: "#9fb4d6"; font.pixelSize: 12 }
            }

            // Fuel block
            ColumnLayout {
                Layout.preferredWidth: 95
                spacing: 2
                Label { text: fuelL.toFixed(1) + " L"; color: "#cfe2ff"; font.pixelSize: 18 }
                Label { text: "Fuel"; color: "#9fb4d6"; font.pixelSize: 12 }
            }

            // Score ring
            ScoreRing {
                Layout.preferredWidth: 58
                Layout.preferredHeight: 58
                value: score
            }
        }
    }
}
