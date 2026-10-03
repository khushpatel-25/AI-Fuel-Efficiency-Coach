import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    anchors.fill: parent
    // Background

    Image {
        anchors.fill: parent
        source: "qrc:/qt/qml/FuelEfficiencyCoach/assets/journey_bg.jpg"
        fillMode: Image.PreserveAspectCrop
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.35
    }

    function formatDate(iso) {
        var d = new Date(iso)
        var months = ["January","February","March","April","May","June","July","August","September","October","November","December"]
        return months[d.getMonth()] + " " + d.getDate() + ", " + d.getFullYear()
    }

    // Layout

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 18

        Label {
            text: "Trip History"
            color: "white"
            font.pixelSize: 30
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 40
            spacing: 12

            RowLayout {
                spacing: 8
                Layout.rightMargin: 40

                Label {
                    text: "Sort:"
                    color: "#9fb4d6"
                    font.pixelSize: 14
                }

                ComboBox {
                    id: sortBox
                    model: ["Newest", "Oldest"]

                    contentItem: Text {
                            text: sortBox.displayText
                            color: "#ffffff"
                            font.pixelSize: 14
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 6
                        }

                    background: Rectangle {
                        radius: 6
                        color: "#0b1a2c"
                        border.width: 1
                        border.color: "#6faeea"
                    }

                    onActivated: {
                        tripController.setSortMode(currentIndex)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth:true
            height:1
            color:"#9fb4d6"
            opacity:0.25
        }

        StackLayout {
            Layout.fillWidth:true
            Layout.fillHeight:true
            ColumnLayout {

                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0
                ListView {

                    id: tripList
                    model: tripController.tripHistory
                    clip: true
                    spacing: 0

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    delegate: Column {

                        width: tripList.width
                        height: 90

                        RowLayout {

                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 18

                            ColumnLayout {
                                Layout.preferredWidth:150

                                Label {
                                    text: formatDate(model.modelData.savedAt).split(",")[0]
                                    color:"white"
                                    font.pixelSize:24
                                }

                                Label {
                                    text: "Date Saved"
                                    color:"#9fb4d6"
                                }
                            }

                            ColumnLayout{
                                Label {
                                    Layout.fillWidth:true
                                    text: model.modelData.tripFileName
                                    color:"white"
                                    font.pixelSize:20
                                }

                                Label{
                                    text:"Filename"
                                    color:"#9fb4d6"
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth:150

                                Label {
                                    text: model.modelData.durationMinutes.toFixed(1) + " min"
                                    color:"white"
                                    font.pixelSize:20
                                }

                                Label {
                                    text:"Duration"
                                    color:"#9fb4d6"
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth:120

                                Label {
                                    text: model.modelData.fuelEfficiencyPercent.toFixed(0) + " %"
                                    color:"white"
                                    font.pixelSize:20
                                }

                                Label {
                                    text:"Efficiency"
                                    color:"#9fb4d6"
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth:120

                                Label {
                                    text: model.modelData.score.toFixed(0)
                                    color:"white"
                                    font.pixelSize:20
                                }

                                Label {
                                    text:"Score"
                                    color:"#9fb4d6"
                                }
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height:1
                            color:"#9fb4d6"
                            opacity:0.12
                        }
                    }
                }
            }
        }
    }
}
