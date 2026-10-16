import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "ShaderControlHelpers.js" as Controls

Window {
    id: editor
    property var strings
    property string profileName: ""
    property var owner
    property var effects: []
    property var values: ({})
    property var pending: ({})
    property bool awaitingMetadata: false
    property bool discarding: false
    property bool preview: false
    property real expandedHeight: height
    onPreviewChanged: {
        if (preview) {
            commitFocusedValue()
            expandedHeight = height; minimumHeight = 150
            height = Qt.binding(function() { return Math.max(150, content.implicitHeight + 28) })
        }
        else { minimumHeight = 260; height = expandedHeight }
    }
    property int effectIndex: 0
    property var effect: effects.length > effectIndex ? effects[effectIndex] : null
    readonly property var controlSections: Controls.sections(effect ? effect.parameters : [])
    title: strings.shaderEditorTitle
    flags: Qt.Tool | Qt.WindowStaysOnTopHint | Qt.WindowTitleHint | Qt.WindowCloseButtonHint
    transientParent: null
    width: 360
    height: Math.min(650, Screen.desktopAvailableHeight - 48)
    minimumWidth: 280
    minimumHeight: 260
    color: "#18212b"
    x: Screen.virtualX + 16
    y: Screen.virtualY + 16

    function openEditor() {
        saveTimer.stop()
        discarding = false
        effects = []; values = ({}); pending = ({}); preview = false; effectIndex = 0
        awaitingMetadata = true
        backend.loadShaderParameters()
        owner.hide()
        show()
        requestActivate()
    }
    function setValues(changes) {
        if (discarding) return
        var next = Object.assign({}, values), edits = Object.assign({}, pending)
        for (var key in changes) { next[key] = changes[key]; edits[key] = changes[key] }
        values = next; pending = edits
        saveTimer.restart()
    }
    function valueFor(p) { return Object.prototype.hasOwnProperty.call(values, p.key) ? (values[p.key] === null ? p.default : values[p.key]) : p.value }
    function change(p, value) { if (value === valueFor(p)) return; var edits = {}; edits[p.key] = value; setValues(edits) }
    function flush() {
        saveTimer.stop()
        if (!Object.keys(pending).length) return true
        if (!backend.setShaderParameters(pending)) return false
        pending = ({})
        return true
    }
    function commitFocusedValue() {
        if (activeFocusItem && typeof activeFocusItem.commitValue === "function") activeFocusItem.commitValue()
    }
    function selectEffect(index) {
        commitFocusedValue()
        effectIndex = index
    }
    function resetEffect() {
        commitFocusedValue()
        var changes = {}
        if (effect) effect.parameters.forEach(function(p) { if (p.resettable !== false) changes[p.key] = null })
        if (Object.keys(changes).length) setValues(changes)
    }
    function discardAndClose() {
        discarding = true
        saveTimer.stop()
        pending = ({})
        close()
    }
    onClosing: function(event) {
        if (!discarding) commitFocusedValue()
        event.accepted = discarding || flush()
        if (event.accepted) { owner.show(); owner.requestActivate() }
    }
    Timer { id: saveTimer; interval: 350; onTriggered: editor.flush() }
    Connections {
        target: backend
        function onShaderParametersChanged() {
            // Save/error notifications must not rebuild controls or steal an unfinished draft.
            if (editor.awaitingMetadata && !backend.shader_parameters_loading) {
                editor.effects = backend.shader_parameter_effects
                editor.awaitingMetadata = false
            }
        }
    }
    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10
        Label { textFormat: Text.PlainText; text: strings.shaderEditorTitle; font.pixelSize: 20; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Label { textFormat: Text.PlainText; text: (editor.profileName ? editor.profileName + " · " : "") + strings.shaderEditorLive; color: "#bac8d6"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        RowLayout {
            Layout.fillWidth: true
            Button { text: editor.preview ? strings.shaderEditorShow : strings.shaderEditorHide; onClicked: editor.preview = !editor.preview; Layout.fillWidth: true }
            Button { text: strings.shaderEditorClose; onClicked: editor.close(); Layout.fillWidth: true }
        }
        Label { textFormat: Text.PlainText; visible: backend.shader_parameters_loading; text: strings.shaderEditorLoading; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        ComboBox {
            visible: !editor.preview && editor.effects.length > 0
            Layout.fillWidth: true
            model: editor.effects
            textRole: "name"
            currentIndex: editor.effectIndex
            onActivated: editor.selectEffect(currentIndex)
        }
        ScrollView {
            id: controlsView
            objectName: "shader-editor-scroll"
            readonly property int frameInset: 12
            readonly property int scrollbarGap: 8
            visible: !editor.preview
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            topPadding: frameInset
            bottomPadding: frameInset
            leftPadding: frameInset + (mirrored ? ScrollBar.vertical.width + scrollbarGap : 0)
            rightPadding: frameInset + (mirrored ? 0 : ScrollBar.vertical.width + scrollbarGap)
            // Reserve a separate lane without Breeze's permanent separator frame.
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.active: ScrollBar.vertical.size < 1
            ScrollBar.vertical.objectName: "shader-editor-scrollbar"
            ScrollBar.vertical.x: mirrored ? frameInset : width - ScrollBar.vertical.width - frameInset
            ScrollBar.vertical.y: topPadding
            ScrollBar.vertical.height: availableHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            background: Rectangle {
                color: "#131c26"
                border.color: "#53677c"
                border.width: 1
                radius: 6
            }
            contentWidth: availableWidth
            ColumnLayout {
                objectName: "shader-editor-controls"
                width: controlsView.availableWidth
                spacing: 12
                Label { textFormat: Text.PlainText;
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#ffbdad"
                    visible: !!editor.effect && !!editor.effect.error
                    text: editor.effect ? (editor.effect.error || "") : ""
                }
                Label { textFormat: Text.PlainText;
                    objectName: "shader-editor-empty"
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#bac8d6"
                    visible: !backend.shader_parameters_loading && !backend.shader_parameter_error && (!editor.effect || !editor.effect.parameters.length) && (!editor.effect || !editor.effect.error)
                    text: strings.shaderEditorEmpty
                }
                Repeater {
                    model: editor.controlSections
                    delegate: ColumnLayout {
                        id: group
                        spacing: 16
                        required property var modelData
                        required property int index
                        property int selectedColor: 0
                        property var g: modelData[selectedColor] || modelData[0]
                        function selectColor(index) {
                            editor.commitFocusedValue()
                            selectedColor = index
                        }
                        Layout.fillWidth: true
                        Label { textFormat: Text.PlainText; visible: group.g.category.length > 0 && (group.index === 0 || editor.controlSections[group.index - 1][0].category !== group.g.category); text: group.g.category; font.bold: true; color: "#bac8d6"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        GridLayout {
                            visible: group.modelData.length > 1
                            columns: 4
                            columnSpacing: 8
                            rowSpacing: 8
                            Layout.fillWidth: true
                            Repeater {
                                model: group.modelData.length > 1 ? group.modelData : []
                                delegate: Button {
                                    id: swatch
                                    required property var modelData
                                    required property int index
                                    objectName: "shader-color-swatch-" + modelData.key
                                    Accessible.name: modelData.label
                                    checkable: true
                                    autoExclusive: true
                                    checked: group.selectedColor === index
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                    implicitWidth: 40
                                    padding: 4
                                    onClicked: group.selectColor(index)
                                    background: Rectangle {
                                        color: "#18212b"
                                        radius: 4
                                        border.width: 2
                                        border.color: swatch.activeFocus ? "#edf8fb" : swatch.checked ? "#83bff0" : "#444c56"
                                    }
                                    contentItem: ColumnLayout {
                                        spacing: 4
                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 26
                                            color: "#596674"
                                            function channel(index) { return Math.max(0, Math.min(1, editor.valueFor(swatch.modelData.parameters[index]))) }
                                            Rectangle {
                                                anchors.fill: parent
                                                color: Qt.rgba(parent.channel(0), parent.channel(1), parent.channel(2), swatch.modelData.parameters.length === 4 ? parent.channel(3) : 1)
                                            }
                                        }
                                        Label {
                                            Layout.fillWidth: true
                                            textFormat: Text.PlainText
                                            text: swatch.modelData.label
                                            color: "#edf8fb"
                                            font.pixelSize: 11
                                            wrapMode: Text.Wrap
                                            horizontalAlignment: Text.AlignHCenter
                                        }
                                    }
                                }
                            }
                        }
                        Label { textFormat: Text.PlainText; visible: group.g.parameters.length > 1; text: group.g.label; font.bold: true; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        Label { textFormat: Text.PlainText; visible: group.g.description.length > 0; text: group.g.description; color: "#bac8d6"; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        Rectangle {
                            visible: group.g.color
                            objectName: "shader-color-preview-" + group.g.key
                            Layout.fillWidth: true; implicitHeight: 32; color: "#596674"
                            Accessible.name: strings.shaderEditorColorPreview
                            function channel(index) { return Math.max(0, Math.min(1, editor.valueFor(group.g.parameters[index]))) }
                            Rectangle { anchors.fill: parent; border.color: "#8393a5"; color: group.g.color ? Qt.rgba(parent.channel(0), parent.channel(1), parent.channel(2), group.g.parameters.length === 4 ? parent.channel(3) : 1) : "transparent" }
                        }
                        Label {
                            textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: "#bac8d6"; font.pixelSize: 11
                            visible: group.g.color && group.g.parameters.some(function(p) { var value = editor.valueFor(p); return value < 0 || value > 1; })
                            text: strings.shaderEditorColorRange
                        }
                        Repeater {
                            model: group.g.parameters
                            delegate: ShaderParameterControl {
                                required property var modelData
                                required property int index
                                parameter: modelData
                                ownerEditor: editor
                                colorControl: group.g.color
                                label: group.g.parameters.length > 1 ? (group.g.color ? [strings.shaderEditorRed, strings.shaderEditorGreen, strings.shaderEditorBlue, strings.shaderEditorAlpha][index] : ["X", "Y", "Z", "W"][index]) : modelData.label
                            }
                        }
                    }
                }
                Button {
                    objectName: "shader-editor-reset"
                    visible: !!editor.effect && editor.effect.parameters.some(function(p) { return p.resettable !== false })
                    text: strings.shaderEditorReset
                    Layout.fillWidth: true
                    onClicked: editor.resetEffect()
                }
            }
        }
        Label { textFormat: Text.PlainText; visible: backend.shader_parameter_error.length > 0; text: backend.shader_parameter_error; color: "#ffbdad"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        Button { visible: backend.shader_parameter_error.length > 0 && Object.keys(editor.pending).length > 0; text: strings.shaderEditorRetry; onClicked: editor.flush() }
        Button { visible: backend.shader_parameter_error.length > 0 && Object.keys(editor.pending).length > 0; text: strings.shaderEditorDiscard; onClicked: editor.discardAndClose() }
        Item { visible: editor.preview; Layout.fillHeight: true }
    }
}
