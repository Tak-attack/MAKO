import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "ShaderControlHelpers.js" as Controls

ColumnLayout {
    id: control
    spacing: 10
    required property var parameter
    required property var ownerEditor
    required property string label
    property bool colorControl: false
    readonly property real currentValue: ownerEditor.valueFor(parameter)
    readonly property bool choices: (parameter.type === "int" || parameter.type === "uint") && parameter.items.length > 0 && Number.isInteger(currentValue) && currentValue >= 0 && currentValue < parameter.items.length
    readonly property var range: Controls.sliderRange(parameter, colorControl)
    Layout.fillWidth: true
    Label { textFormat: Text.PlainText; text: control.label; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    CheckBox { visible: control.parameter.type === "bool"; Accessible.name: control.label; checked: control.ownerEditor.valueFor(control.parameter) !== 0; onToggled: control.ownerEditor.change(control.parameter, checked ? 1 : 0) }
    ComboBox { visible: control.parameter.type !== "bool" && control.choices; Accessible.name: control.label; model: control.parameter.items.map(function(label, index) { return label || String(index); }); currentIndex: control.ownerEditor.valueFor(control.parameter); onActivated: control.ownerEditor.change(control.parameter, currentIndex); Layout.fillWidth: true }
    Slider {
        visible: control.parameter.type !== "bool" && !control.choices && control.range !== null
        Accessible.name: control.label
        Layout.fillWidth: true
        from: control.range ? control.range.min : 0
        to: control.range ? control.range.max : 1
        stepSize: control.range ? control.range.step : 0.01
        value: Math.max(from, Math.min(to, control.ownerEditor.valueFor(control.parameter)))
        onMoved: control.ownerEditor.change(control.parameter, value)
    }
    TextField {
        id: numberInput
        property real savedValue: control.ownerEditor.valueFor(control.parameter)
        objectName: "shader-parameter-" + control.parameter.key
        Accessible.name: control.label
        visible: control.parameter.type !== "bool" && !control.choices
        Layout.fillWidth: true
        text: Number(savedValue).toString()
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        function commitValue() {
            var number = Number(text)
            if (text.trim().length && isFinite(number)) control.ownerEditor.change(control.parameter, number)
            text = Qt.binding(function() { return Number(numberInput.savedValue).toString() })
        }
        onEditingFinished: commitValue()
    }
}
