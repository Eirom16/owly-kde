// SPDX-License-Identifier: MIT
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.extras as PE

PC.Page {
    id: view
    required property var backend
    readonly property var session: backend.state
    property var selectedFiles: []
    property string selectedTarget: ""
    property int page: 0
    signal configureRequested()
    padding: 0

    function chooseFiles(urls) {
        if (urls.length > 0) {
            selectedFiles = Array.from(urls)
            page = 0
        }
    }

    function fileName(url) {
        const text = String(url)
        try { return decodeURIComponent(text.substring(text.lastIndexOf("/") + 1)) }
        catch (error) { return text.substring(text.lastIndexOf("/") + 1) }
    }

    FileDialog {
        id: picker
        title: qsTr("Seleccionar archivos para enviar")
        fileMode: FileDialog.OpenFiles
        onAccepted: view.chooseFiles(selectedFiles)
    }

    DropArea {
        anchors.fill: parent
        onEntered: drag => { drag.accepted = drag.hasUrls }
        onDropped: drop => {
            if (drop.hasUrls) {
                view.chooseFiles(drop.urls)
                drop.acceptProposedAction()
            }
        }
    }

    header: PE.PlasmoidHeading {
        contentItem: RowLayout {
            spacing: Kirigami.Units.largeSpacing
            AirDropIcon {
                implicitWidth: Kirigami.Units.iconSizes.medium
                implicitHeight: implicitWidth
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Kirigami.Heading { text: "Owly KDE"; level: 2 }
                PE.DescriptiveLabel {
                    Layout.fillWidth: true
                    text: qsTr("AirDrop, desde tu escritorio")
                    elide: Text.ElideRight
                }
            }
            PC.ToolButton {
                icon.name: "configure"
                text: qsTr("Configurar Owly")
                display: PC.AbstractButton.IconOnly
                PC.ToolTip.text: text
                PC.ToolTip.visible: hovered
                onClicked: view.configureRequested()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        PC.Frame {
            Layout.fillWidth: true
            padding: Kirigami.Units.largeSpacing
            ColumnLayout {
                width: parent.width
                spacing: Kirigami.Units.smallSpacing
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.largeSpacing
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.smallSpacing
                        PC.Label {
                            Layout.fillWidth: true
                            font.bold: true
                            wrapMode: Text.Wrap
                            text: view.session.stopping ? qsTr("Restaurando Wi-Fi…")
                                : view.session.starting ? qsTr("Activando AirDrop…")
                                : view.session.active ? qsTr("AirDrop está activo") : qsTr("Listo para compartir")
                        }
                        PE.DescriptiveLabel {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            text: view.session.active
                                ? qsTr("La sesión termina en %1:%2").arg(Math.floor(view.session.remaining / 60)).arg(String(view.session.remaining % 60).padStart(2, "0"))
                                : qsTr("Activa una sesión para enviar y recibir.")
                        }
                    }
                    PC.BusyIndicator {
                        Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                        Layout.preferredHeight: Layout.preferredWidth
                        visible: view.session.starting || view.session.stopping || view.session.checking
                        running: visible
                    }
                    PC.Button {
                        text: view.session.active || view.session.starting ? qsTr("Desactivar") : qsTr("Activar")
                        icon.name: view.session.active ? "media-playback-stop" : "network-connect"
                        enabled: !view.session.stopping && !view.session.checking && (view.session.available || view.session.active || view.session.starting)
                        onClicked: view.session.active || view.session.starting ? view.backend.stop() : view.backend.start()
                    }
                }
                PC.Switch {
                    Layout.fillWidth: true
                    text: qsTr("Permitir solicitudes de recepción")
                    checked: view.session.receiving
                    enabled: view.session.active && !view.session.stopping
                    onToggled: view.backend.setReceiving(checked)
                }
            }
        }

        PC.TabBar {
            id: tabs
            Layout.fillWidth: true
            currentIndex: view.page
            onCurrentIndexChanged: view.page = currentIndex
            PC.TabButton { text: qsTr("Enviar"); icon.name: "document-send" }
            PC.TabButton { text: qsTr("Actividad"); icon.name: "view-history" }
        }

        PC.ScrollView {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth
            PC.ScrollBar.horizontal.policy: PC.ScrollBar.AlwaysOff

            ColumnLayout {
                width: scroller.availableWidth
                spacing: Kirigami.Units.largeSpacing

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: view.session.error.length > 0
                    text: view.session.error
                    type: Kirigami.MessageType.Error
                    showCloseButton: true
                    onVisibleChanged: if (!visible && view.session.error.length > 0) view.backend.dismissError()
                }

                Repeater {
                    model: view.session.requests
                    delegate: PC.Frame {
                        id: requestCard
                        required property var modelData
                        Layout.fillWidth: true
                        padding: Kirigami.Units.largeSpacing
                        ColumnLayout {
                            width: parent.width
                            spacing: Kirigami.Units.smallSpacing
                            PC.Label {
                                Layout.fillWidth: true
                                text: qsTr("%1 quiere compartir contigo").arg(requestCard.modelData.sender)
                                textFormat: Text.PlainText
                                font.bold: true
                                wrapMode: Text.Wrap
                            }
                            Repeater {
                                model: requestCard.modelData.items
                                delegate: PC.Label {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    textFormat: Text.PlainText
                                    elide: Text.ElideMiddle
                                }
                            }
                            PE.DescriptiveLabel {
                                Layout.fillWidth: true
                                text: qsTr("Responde antes de 15 segundos.")
                                wrapMode: Text.Wrap
                            }
                            RowLayout {
                                Layout.alignment: Qt.AlignRight
                                spacing: Kirigami.Units.smallSpacing
                                PC.Button {
                                    text: qsTr("Rechazar")
                                    onClicked: view.backend.answer(requestCard.modelData.id, false)
                                }
                                PC.Button {
                                    text: qsTr("Aceptar")
                                    icon.name: "dialog-ok"
                                    onClicked: view.backend.answer(requestCard.modelData.id, true)
                                }
                            }
                        }
                    }
                }

                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: !view.session.active && !!view.session.report.mode && view.session.report.mode !== "shared"
                    text: qsTr("Con esta tarjeta, activar AirDrop desconectará temporalmente el Wi-Fi. Se restaurará al terminar la sesión.")
                    type: Kirigami.MessageType.Information
                }

                Repeater {
                    model: !view.session.active ? (view.session.report.missing || []) : []
                    delegate: Kirigami.InlineMessage {
                        required property var modelData
                        Layout.fillWidth: true
                        text: modelData.say
                        type: Kirigami.MessageType.Warning
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: view.page === 0
                    spacing: Kirigami.Units.largeSpacing

                    PC.Frame {
                        Layout.fillWidth: true
                        padding: Kirigami.Units.largeSpacing
                        ColumnLayout {
                            width: parent.width
                            spacing: Kirigami.Units.smallSpacing
                            Kirigami.Icon {
                                Layout.alignment: Qt.AlignHCenter
                                source: view.selectedFiles.length > 0 ? "document-multiple" : "document-send"
                                implicitWidth: Kirigami.Units.iconSizes.large
                                implicitHeight: implicitWidth
                            }
                            PC.Label {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                font.bold: true
                                text: view.selectedFiles.length > 0 ? qsTr("%1 archivos seleccionados").arg(view.selectedFiles.length) : qsTr("Comparte algo")
                                wrapMode: Text.Wrap
                            }
                            PE.DescriptiveLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                text: view.selectedFiles.length > 0 ? view.fileName(view.selectedFiles[0]) : qsTr("Arrastra archivos aquí o selecciónalos.")
                                textFormat: Text.PlainText
                                elide: Text.ElideMiddle
                            }
                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                PC.Button {
                                    text: view.selectedFiles.length > 0 ? qsTr("Cambiar archivos…") : qsTr("Seleccionar archivos…")
                                    icon.name: "list-add"
                                    enabled: !view.session.sending
                                    onClicked: picker.open()
                                }
                                PC.ToolButton {
                                    visible: view.selectedFiles.length > 0
                                    icon.name: "edit-clear"
                                    text: qsTr("Quitar selección")
                                    display: PC.AbstractButton.IconOnly
                                    enabled: !view.session.sending
                                    PC.ToolTip.text: text
                                    PC.ToolTip.visible: hovered
                                    onClicked: view.selectedFiles = []
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Kirigami.Heading {
                            Layout.fillWidth: true
                            text: qsTr("Dispositivos cercanos")
                            level: 3
                            elide: Text.ElideRight
                        }
                        PC.BusyIndicator {
                            Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                            Layout.preferredHeight: Layout.preferredWidth
                            running: view.session.scanning
                            visible: running
                        }
                        PC.ToolButton {
                            icon.name: "view-refresh"
                            text: qsTr("Buscar dispositivos")
                            display: PC.AbstractButton.IconOnly
                            enabled: view.session.active && !view.session.scanning && !view.session.stopping
                            PC.ToolTip.text: text
                            PC.ToolTip.visible: hovered
                            onClicked: { view.selectedTarget = ""; view.backend.scan() }
                        }
                    }

                    Repeater {
                        model: view.session.devices
                        delegate: PC.ItemDelegate {
                            id: deviceRow
                            required property var modelData
                            Layout.fillWidth: true
                            text: modelData.name
                            icon.name: "smartphone"
                            highlighted: view.selectedTarget === modelData.id
                            enabled: !view.session.sending && !view.session.stopping
                            Accessible.name: qsTr("Enviar a %1").arg(modelData.name)
                            onClicked: view.selectedTarget = modelData.id
                            contentItem: RowLayout {
                                spacing: Kirigami.Units.largeSpacing
                                Kirigami.Icon {
                                    source: "smartphone"
                                    implicitWidth: Kirigami.Units.iconSizes.medium
                                    implicitHeight: implicitWidth
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    PC.Label {
                                        Layout.fillWidth: true
                                        text: deviceRow.modelData.name
                                        textFormat: Text.PlainText
                                        elide: Text.ElideRight
                                    }
                                    PE.DescriptiveLabel { text: qsTr("Disponible para AirDrop") }
                                }
                                Kirigami.Icon {
                                    source: "dialog-ok"
                                    visible: view.selectedTarget === deviceRow.modelData.id
                                    implicitWidth: Kirigami.Units.iconSizes.smallMedium
                                    implicitHeight: implicitWidth
                                }
                            }
                        }
                    }

                    PE.PlaceholderMessage {
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        visible: view.session.devices.length === 0
                        iconName: view.session.active ? "network-wireless" : "network-disconnect"
                        text: view.session.scanning ? qsTr("Buscando dispositivos…") : view.session.active ? qsTr("Aún no hay dispositivos") : qsTr("Activa AirDrop para empezar")
                        explanation: view.session.active ? qsTr("Desbloquea el dispositivo Apple y elige «Todos durante 10 minutos» en AirDrop.") : qsTr("Tus dispositivos cercanos aparecerán aquí.")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: view.page === 1
                    spacing: Kirigami.Units.largeSpacing
                    PE.PlaceholderMessage {
                        Layout.fillWidth: true
                        visible: view.session.history.length === 0
                        iconName: "view-history"
                        text: qsTr("Todo empieza con un archivo")
                        explanation: qsTr("Las transferencias y los errores aparecerán aquí durante esta sesión del widget.")
                    }
                    Repeater {
                        model: view.session.history
                        delegate: PC.Frame {
                            id: historyRow
                            required property var modelData
                            Layout.fillWidth: true
                            padding: Kirigami.Units.largeSpacing
                            RowLayout {
                                width: parent.width
                                spacing: Kirigami.Units.largeSpacing
                                Kirigami.Icon {
                                    source: historyRow.modelData.icon
                                    implicitWidth: Kirigami.Units.iconSizes.medium
                                    implicitHeight: implicitWidth
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: Kirigami.Units.smallSpacing
                                    PC.Label {
                                        Layout.fillWidth: true
                                        text: historyRow.modelData.title
                                        textFormat: Text.PlainText
                                        font.bold: true
                                        wrapMode: Text.Wrap
                                    }
                                    PC.TextArea {
                                        Layout.fillWidth: true
                                        text: historyRow.modelData.detail
                                        textFormat: TextEdit.PlainText
                                        wrapMode: TextEdit.WrapAnywhere
                                        readOnly: true
                                        selectByMouse: true
                                        padding: 0
                                        background: null
                                    }
                                }
                                PE.DescriptiveLabel { text: historyRow.modelData.time }
                            }
                        }
                    }
                }

                PC.Button {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !view.session.active && !view.session.starting
                    text: qsTr("Comprobar de nuevo")
                    icon.name: "view-refresh"
                    enabled: !view.session.checking
                    onClicked: view.backend.diagnose()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            PC.Label {
                Layout.fillWidth: true
                visible: view.session.notice.length > 0
                text: view.session.notice
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
            PC.ProgressBar {
                Layout.fillWidth: true
                visible: view.session.sending
                indeterminate: view.session.progress < 0
                value: Math.max(0, view.session.progress)
                Accessible.name: qsTr("Progreso del envío")
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                PC.ToolButton {
                    text: qsTr("Descargas")
                    icon.name: "folder-downloads"
                    onClicked: view.backend.openDownloads()
                    PC.ToolTip.text: view.session.destination
                    PC.ToolTip.visible: hovered
                }
                Item { Layout.fillWidth: true }
                PC.Button {
                    text: view.session.sending ? qsTr("Cancelar") : qsTr("Enviar")
                    icon.name: view.session.sending ? "dialog-cancel" : "document-send"
                    enabled: view.session.sending || (view.session.active && !view.session.stopping && view.selectedFiles.length > 0 && view.session.devices.some(device => device.id === view.selectedTarget))
                    onClicked: view.session.sending ? view.backend.cancelSend() : view.backend.sendFiles(view.selectedTarget, view.selectedFiles)
                }
            }
        }
    }
}
