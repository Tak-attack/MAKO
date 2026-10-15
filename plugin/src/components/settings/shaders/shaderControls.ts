import type { ShaderParameter } from "../../../api/makoApi";

export interface ShaderControlGroup {
  key: string;
  label: string;
  description: string;
  category: string;
  color: boolean;
  parameters: ShaderParameter[];
}

// Group only a complete compiler-declared vector. Labels/names never imply type.
export function shaderControlGroups(
  parameters: ShaderParameter[],
): ShaderControlGroup[] {
  const groups: ShaderControlGroup[] = [];
  for (let index = 0; index < parameters.length;) {
    const p = parameters[index];
    const hint = p.presentation;
    let members = [p];
    if (
      hint &&
      p.type !== "bool" &&
      ["", "input", "drag", "slider", "color"].includes(hint.widget) &&
      typeof hint.label === "string" &&
      hint.label.length > 0 &&
      hint.component === 0 &&
      Number.isInteger(hint.count) &&
      hint.count >= 2 &&
      hint.count <= 4 &&
      hint.group === p.key &&
      p.key.endsWith("_0")
    ) {
      const candidates = parameters.slice(index, index + hint.count);
      if (
        candidates.length === hint.count &&
        candidates.every((candidate, component) => {
          const other = candidate.presentation;
          return (
            other &&
            other.group === hint.group &&
            other.label === hint.label &&
            other.widget === hint.widget &&
            other.count === hint.count &&
            other.component === component &&
            candidate.key === hint.group.slice(0, -1) + component &&
            candidate.type === p.type &&
            candidate.label === `${hint.label} [${component + 1}]` &&
            !candidate.items.length &&
            candidate.category === p.category &&
            candidate.description === p.description
          );
        })
      )
        members = candidates;
    }
    const grouped = members.length > 1;
    groups.push({
      key: p.key,
      label: grouped ? hint!.label : p.label,
      description: p.description,
      category: p.category,
      color:
        grouped &&
        members.length >= 3 &&
        p.type === "float" &&
        hint!.widget === "color",
      parameters: members,
    });
    index += members.length;
  }
  return groups;
}

export function shaderSliderRange(p: ShaderParameter, color = false) {
  if (p.presentation?.widget === "input") return null;
  const min = p.min ?? (color ? 0 : null);
  const max = p.max ?? (color ? 1 : null);
  if (
    min === null ||
    max === null ||
    !Number.isFinite(min) ||
    !Number.isFinite(max) ||
    max <= min
  )
    return null;
  const step = p.step ?? (p.type === "float" ? (max - min) / 100 : 1);
  if (
    !Number.isFinite(step) ||
    step <= 0 ||
    (p.type !== "float" &&
      (!Number.isInteger(min) ||
        !Number.isInteger(max) ||
        !Number.isInteger(step)))
  )
    return null;
  return { min, max, step };
}

export function shaderColorPreview(parameters: ShaderParameter[]): string {
  const channel = (index: number) =>
    Math.max(0, Math.min(1, parameters[index]?.value ?? 1));
  return `rgba(${[0, 1, 2].map((i) => Math.round(channel(i) * 255)).join(", ")}, ${parameters.length === 4 ? channel(3) : 1})`;
}
