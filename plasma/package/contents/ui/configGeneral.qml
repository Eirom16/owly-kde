// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.FormLayout {
    id: config
    property alias cfg_executable: executable.text
    property alias cfg_downloadFolder: folder.text
    property alias cfg_displayName: displayName.text
    property alias cfg_sessionMinutes: minutes.value

    QQC2.TextField {
        id: displayName
        Kirigami.FormData.label: qsTr("Nombre visible:")
        placeholderText: qsTr("Nombre del equipo")
        Layout.fillWidth: true
    }
    RowLayout {
        Kirigami.FormData.label: qsTr("Guardar archivos en:")
        Layout.fillWidth: true
        QQC2.TextField {
            id: folder
            Layout.fillWidth: true
            placeholderText: qsTr("Carpeta de descargas del sistema")
        }
        QQC2.Button {
            text: qsTr("Elegir…")
            icon.name: "folder-open"
            onClicked: folderPicker.open()
        }
    }
    QQC2.SpinBox {
        id: minutes
        Kirigami.FormData.label: qsTr("Duración de la sesión (min):")
        from: 1
        to: 60
    }
    QQC2.TextField {
        id: executable
        Kirigami.FormData.label: qsTr("Ejecutable de Owly:")
        placeholderText: qsTr("Detección automática")
        Layout.fillWidth: true
    }
    QQC2.Label {
        Layout.fillWidth: true
        text: qsTr("Los cambios se aplican a la siguiente sesión. Cada transferencia entrante requiere tu aceptación.")
        wrapMode: Text.Wrap
    }
    FolderDialog {
        id: folderPicker
        title: qsTr("Carpeta para los archivos recibidos")
        onAccepted: folder.text = decodeURIComponent(String(selectedFolder).replace(/^file:\/\//, ""))
    }
}
