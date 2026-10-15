/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QPointer>
#include <iostream>
#include <stdexcept>

class ShaderBackendFixture : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool shader_parameters_loading READ loading NOTIFY shaderParametersChanged)
    Q_PROPERTY(QString shader_parameter_error READ error NOTIFY shaderParametersChanged)
    Q_PROPERTY(QVariantList shader_parameter_effects READ effects NOTIFY shaderParametersChanged)
public:
    bool fail = false;
    bool noParameters = false;
    int saves = 0;
    QVariantMap last;
    bool loading() const { return false; }
    QString error() const { return fail ? QStringLiteral("disk full") : QString(); }
    QVariantList effects() const {
        if (noParameters) return {};
        auto result = QJsonDocument::fromJson(R"JSON([{"id":"Tone","name":"Tone","error":null,"parameters":[
        {"key":"strength","label":"Strength","description":"Effect intensity","category":"Colour","type":"float","min":0,"max":1,"step":0.01,"value":0.5,"default":0.25,"items":[]},
        {"key":"mode","label":"Mode","description":"","category":"","type":"int","min":null,"max":null,"step":null,"value":0,"default":0,"items":["Soft","Strong"]},
        {"key":"dither","label":"Dithering","description":"Reduce colour banding","category":"","type":"bool","min":null,"max":null,"step":null,"value":1,"default":1,"items":[]}
        ]}])JSON").array().toVariantList();
        auto effect = result[0].toMap();
        auto parameters = effect["parameters"].toList();
        for (int component = 0; component < 4; ++component) {
            const QString base = QStringLiteral("makoShader_41_436f6c6f7572_");
            parameters.append(QVariantMap{{"key", base + QString::number(component)}, {"label", QStringLiteral("Border colour [%1]").arg(component + 1)}, {"description", "Authored colour help"}, {"category", "Colour"}, {"type", "float"}, {"value", component == 2 ? 1.5 : .123456789}, {"default", 0.0}, {"resettable", component != 3}, {"min", QVariant()}, {"max", QVariant()}, {"step", QVariant()}, {"items", QVariantList()}, {"presentation", QVariantMap{{"widget", "color"}, {"group", base + "0"}, {"label", "Border colour"}, {"count", 4}, {"component", component}}}});
        }
        effect["parameters"] = parameters;
        result[0] = effect;
        return result;
    }
    Q_INVOKABLE void loadShaderParameters() { emit shaderParametersChanged(); }
    Q_INVOKABLE bool setShaderParameters(const QVariantMap& changes) { ++saves; last = changes; emit shaderParametersChanged(); return !fail; }
signals:
    void shaderParametersChanged();
};
static void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
int main(int argc, char** argv) {
    QGuiApplication application(argc, argv);
    ShaderBackendFixture backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    const auto directory = QUrl::fromLocalFile(QString::fromUtf8(MAKO_UI_DIALOGS_DIRECTORY)).toString();
    try {
        engine.loadData((QStringLiteral(R"QML(
import QtQuick
import QtQuick.Controls
import ")QML") + directory + QStringLiteral(R"QML("
ApplicationWindow {
    id: main
    width: 700; height: 400; visible: true
    ShaderParametersWindow {
        id: editor
        objectName: "editor"
        owner: main
        strings: ({shaderEditorTitle:"Customize shaders",shaderEditorLive:"Saved to this profile and applied live.",shaderEditorHide:"Hide controls",shaderEditorShow:"Show controls",shaderEditorClose:"Close",shaderEditorLoading:"Reading shader options...",shaderEditorEmpty:"No editable parameters",shaderEditorReset:"Reset shader to defaults",shaderEditorRetry:"Retry save",shaderEditorDiscard:"Discard unsaved changes",shaderEditorRed:"Red",shaderEditorGreen:"Green",shaderEditorBlue:"Blue",shaderEditorAlpha:"Alpha",shaderEditorColorPreview:"Colour preview",shaderEditorColorRange:"Preview is limited to 0–1. Exact values are preserved."})
    }
    Component.onCompleted: editor.openEditor()
})QML")).toUtf8());
        require(!engine.rootObjects().empty(), "cannot load shader editor");
        auto* owner = engine.rootObjects().front();
        auto* editor = owner->findChild<QQuickWindow*>(QStringLiteral("editor"));
        require(editor && editor->isVisible(), "editor did not open");
        require(!owner->property("visible").toBool(), "configuration window still obscures the game");
        require(editor->width() <= 360, "shader editor is too wide");
        QVariantMap values{{QStringLiteral("strength"), 0.8}};
        require(QMetaObject::invokeMethod(editor,"setValues",Q_ARG(QVariant,QVariant(values))), "cannot adjust shader value");
        backend.fail = true;
        editor->close();
        require(editor->isVisible() && backend.saves == 1, "failed save incorrectly closed editor");
        backend.fail = false;
        editor->close();
        require(!editor->isVisible() && owner->property("visible").toBool() && backend.saves == 2 && backend.last == values, "close did not retry pending values and restore main window");
        QMetaObject::invokeMethod(editor,"openEditor");
        const auto findNamed = [](auto&& find, QQuickItem* item, const QString& name) -> QQuickItem* {
            if (item->objectName() == name) return item;
            for (auto* child : item->childItems())
                if (auto* result = find(find, child, name)) return result;
            return nullptr;
        };
        const auto findInput = [&](auto&&, QQuickItem* item) { return findNamed(findNamed, item, QStringLiteral("shader-parameter-strength")); };
        auto* red = findNamed(findNamed, editor->contentItem(), QStringLiteral("shader-parameter-makoShader_41_436f6c6f7572_0"));
        auto* blue = findNamed(findNamed, editor->contentItem(), QStringLiteral("shader-parameter-makoShader_41_436f6c6f7572_2"));
        auto* swatch = findNamed(findNamed, editor->contentItem(), QStringLiteral("shader-color-preview-makoShader_41_436f6c6f7572_0"));
        require(red && blue && swatch && swatch->isVisible(), "declared colour did not create channel controls and a preview");
        require(red->parentItem()->property("label").toString() == "Red" && blue->parentItem()->property("label").toString() == "Blue", "colour channel labels are incorrect");
        require(red->property("text").toString() == "0.123456789" && blue->property("text").toString() == "1.5", "colour presentation rounded or clamped exact values");
        const auto savesBeforeColour = backend.saves;
        require(editor->property("pending").toMap().empty(), "opening colour controls changed saved values");
        red->forceActiveFocus();
        red->setProperty("text", QStringLiteral("0.456789123"));
        editor->close();
        require(backend.saves == savesBeforeColour + 1 && backend.last.size() == 1 && backend.last.value("makoShader_41_436f6c6f7572_0").toDouble() == .456789123, "colour edit did not preserve the exact value or changed another channel");
        QMetaObject::invokeMethod(editor,"openEditor");
        red = findNamed(findNamed, editor->contentItem(), QStringLiteral("shader-parameter-makoShader_41_436f6c6f7572_0"));
        red->forceActiveFocus(); red->setProperty("text", QStringLiteral("0.8"));
        QMetaObject::invokeMethod(editor, "resetEffect");
        const auto reset = editor->property("pending").toMap();
        require(reset.value("makoShader_41_436f6c6f7572_0").isNull() && !reset.contains("makoShader_41_436f6c6f7572_3"), "Qt reset lost to the focused draft or reset a protected parameter");
        editor->close();
        QMetaObject::invokeMethod(editor,"openEditor");
        auto* input = findInput(findInput, editor->contentItem());
        require(input != nullptr, "numeric shader control was not created");
        input->forceActiveFocus();
        input->setProperty("text", QStringLiteral("0.65"));
        QPointer<QQuickItem> draftControl(input);
        QMetaObject::invokeMethod(editor,"setValues",Q_ARG(QVariant,QVariant(QVariantMap{{QStringLiteral("mode"), 1}})));
        QMetaObject::invokeMethod(editor,"flush");
        require(draftControl && draftControl->property("text").toString() == QStringLiteral("0.65"), "saving another option replaced the focused draft");
        editor->close();
        require(backend.last.value(QStringLiteral("strength")).toDouble() == 0.65, "closing lost the focused numeric draft");
        QMetaObject::invokeMethod(editor,"openEditor");
        input = findInput(findInput, editor->contentItem());
        input->forceActiveFocus();
        input->setProperty("text", QStringLiteral("0.7"));
        const auto expanded = editor->height();
        editor->setProperty("preview", true);
        QCoreApplication::processEvents();
        require(editor->height() < expanded, "preview did not collapse the editor");
        require(editor->property("pending").toMap().value(QStringLiteral("strength")).toDouble() == 0.7, "hiding controls lost the focused draft");
        editor->setProperty("preview", false);
        QCoreApplication::processEvents();
        require(editor->height() == expanded, "controls did not restore the editor size");
        editor->close();
        QMetaObject::invokeMethod(editor,"openEditor");
        input = findInput(findInput, editor->contentItem());
        input->forceActiveFocus();
        input->setProperty("text", QStringLiteral("0.8"));
        backend.fail = true;
        editor->close();
        const auto savesBeforeDiscard = backend.saves;
        require(editor->isVisible(), "failed save closed the editor before discard");
        input->setProperty("text", QStringLiteral("0.9"));
        QMetaObject::invokeMethod(editor,"discardAndClose");
        require(!editor->isVisible() && backend.saves == savesBeforeDiscard, "discard retried the focused unsaved draft");
        backend.noParameters = true;
        QMetaObject::invokeMethod(editor,"openEditor");
        QCoreApplication::processEvents();
        auto* empty = findNamed(findNamed, editor->contentItem(), QStringLiteral("shader-editor-empty"));
        require(empty && !empty->isVisible(), "discovery failure incorrectly claimed there were no editable parameters");
        editor->close();
        backend.fail = false;
        QMetaObject::invokeMethod(editor,"openEditor");
        QCoreApplication::processEvents();
        require(empty->isVisible(), "successful empty discovery lost its message");
        editor->close();
        backend.noParameters = false;
        if (argc > 1) {
            QMetaObject::invokeMethod(editor,"openEditor");
            QTimer::singleShot(250, &application, [&] { editor->grabWindow().save(QString::fromLocal8Bit(argv[1])); application.quit(); });
            return application.exec();
        }
    } catch (const std::exception& error) {
        std::cerr << "MAKO Renderer: shader editor test failed: " << error.what() << '\n'; return 1;
    }
    return 0;
}
#include "shader_parameters_window_tests.moc"
