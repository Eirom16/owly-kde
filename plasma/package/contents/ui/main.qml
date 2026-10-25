// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PC
import org.kde.plasma.plasmoid
import "backend" as Owly

PlasmoidItem {
    id: root
    readonly property var backend: controller
    readonly property var session: controller.state
    readonly property bool busy: session.starting || session.stopping || session.sending

    Plasmoid.icon: "network-wireless"
    Plasmoid.status: session.active ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    Plasmoid.backgroundHints: PlasmaCore.Types.DefaultBackground
    toolTipMainText: "Owly KDE · AirDrop"
    toolTipSubText: session.active ? qsTr("Disponible para compartir · %1 min restantes").arg(Math.ceil(session.remaining / 60)) : qsTr("Comparte archivos con dispositivos Apple")
    switchWidth: Kirigami.Units.gridUnit * 18
    switchHeight: Kirigami.Units.gridUnit * 22

    function configureBackend() {
        controller.configure(Plasmoid.configuration.executable,
            Plasmoid.configuration.downloadFolder,
            Plasmoid.configuration.displayName,
            Plasmoid.configuration.sessionMinutes)
    }

    Owly.Backend {
        id: controller
        onIncoming: root.expanded = true
    }

    Component.onCompleted: configureBackend()
    Connections {
        target: Plasmoid.configuration
        function onValueChanged() { root.configureBackend() }
    }

    compactRepresentation: PC.AbstractButton {
        id: compact
        Layout.minimumWidth: Kirigami.Units.iconSizes.small
        Layout.minimumHeight: Kirigami.Units.iconSizes.small
        Accessible.name: root.toolTipMainText
        Accessible.description: root.toolTipSubText
        onClicked: root.expanded = !root.expanded
        DropArea {
            anchors.fill: parent
            onEntered: drag => { if (drag.hasUrls) root.expanded = true }
        }
        contentItem: Item {
            Kirigami.Icon {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height)
                height: width
                source: "network-wireless"
                active: compact.hovered || root.expanded
                opacity: root.session.active ? 1 : 0.65
            }
            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: Math.max(Kirigami.Units.smallSpacing * 2, parent.width * 0.25)
                height: width
                radius: width / 2
                color: root.session.requests.length > 0 ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.positiveTextColor
                border.color: Kirigami.Theme.backgroundColor
                border.width: 1
                visible: root.session.active
            }
            PC.BusyIndicator {
                anchors.fill: parent
                running: root.busy
                visible: running
            }
        }
    }

    fullRepresentation: FullRepresentation {
        backend: root.backend
        Layout.minimumWidth: Kirigami.Units.gridUnit * 20
        Layout.minimumHeight: Kirigami.Units.gridUnit * 26
        Layout.preferredWidth: Kirigami.Units.gridUnit * 24
        Layout.preferredHeight: Kirigami.Units.gridUnit * 34
        onConfigureRequested: Plasmoid.internalAction("configure").trigger()
    }
}
