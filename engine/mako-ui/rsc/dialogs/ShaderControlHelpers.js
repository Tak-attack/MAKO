.pragma library

// Keep the independent Qt binding aligned with Decky's shaderControls.ts.
function groups(parameters) {
    var result = [];
    for (var index = 0; index < parameters.length;) {
        var p = parameters[index], hint = p.presentation, members = [p];
        if (hint && p.type !== "bool" && ["", "input", "drag", "slider", "color"].indexOf(hint.widget) >= 0 && typeof hint.label === "string" && hint.label.length > 0 && hint.component === 0 && Number.isInteger(hint.count) && hint.count >= 2 && hint.count <= 4 && hint.group === p.key && p.key.endsWith("_0")) {
            var candidates = parameters.slice(index, index + hint.count);
            if (candidates.length === hint.count && candidates.every(function(candidate, component) {
                var other = candidate.presentation;
                return other && other.group === hint.group && other.label === hint.label && other.widget === hint.widget && other.count === hint.count && other.component === component &&
                    candidate.key === hint.group.slice(0, -1) + component && candidate.type === p.type && candidate.label === hint.label + " [" + (component + 1) + "]" && !candidate.items.length &&
                    candidate.category === p.category && candidate.description === p.description;
            })) members = candidates;
        }
        var grouped = members.length > 1;
        result.push({key: p.key, label: grouped ? hint.label : p.label, description: p.description, category: p.category,
            color: grouped && members.length >= 3 && p.type === "float" && hint.widget === "color", parameters: members});
        index += members.length;
    }
    return result;
}

function sliderRange(p, color) {
    if (p.presentation && p.presentation.widget === "input") return null;
    var min = p.min == null ? (color ? 0 : null) : p.min;
    var max = p.max == null ? (color ? 1 : null) : p.max;
    if (min === null || max === null || !Number.isFinite(min) || !Number.isFinite(max) || max <= min) return null;
    var step = p.step == null ? (p.type === "float" ? (max - min) / 100 : 1) : p.step;
    if (!Number.isFinite(step) || step <= 0 || (p.type !== "float" && (!Number.isInteger(min) || !Number.isInteger(max) || !Number.isInteger(step)))) return null;
    return {min: min, max: max, step: step};
}
