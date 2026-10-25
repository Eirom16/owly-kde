// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PE
import "backend" as Owly

PC.ItemDelegate {
    id: fileRow
    required property var file
    required property var backend
    objectName: "historyFile"
    text: file.name
    padding: Kirigami.Units.smallSpacing
    Accessible.name: qsTr("Abrir %1").arg(file.name)
    PC.ToolTip.text: file.path
    PC.ToolTip.visible: hovered
    PC.ToolTip.delay: Kirigami.Units.toolTipDelay
    onClicked: backend.openHistoryFile(file.url)

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing
        Item {
            Layout.preferredWidth: Kirigami.Units.iconSizes.large
            Layout.preferredHeight: Layout.preferredWidth
            Owly.FileThumbnail {
                id: thumbnail
                objectName: "historyThumbnail"
                anchors.fill: parent
                source: fileRow.file.url
            }
            Kirigami.Icon {
                anchors.centerIn: parent
                width: Kirigami.Units.iconSizes.medium
                height: width
                source: fileRow.file.icon || "unknown"
                visible: !thumbnail.ready
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            PC.Label {
                Layout.fillWidth: true
                text: fileRow.file.name
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
            }
            PE.DescriptiveLabel {
                Layout.fillWidth: true
                text: fileRow.file.path
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
            }
        }
        PC.ToolButton {
            icon.name: "document-open-folder"
            text: qsTr("Abrir carpeta contenedora")
            display: PC.AbstractButton.IconOnly
            PC.ToolTip.text: text
            PC.ToolTip.visible: hovered
            onClicked: fileRow.backend.openHistoryFile(fileRow.file.url, true)
        }
    }
}
