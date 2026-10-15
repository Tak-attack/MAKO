import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import vm from "node:vm";
import { expect, test } from "vitest";
import type { ShaderParameter } from "../../src/api/makoApi";
import {
  shaderControlGroups,
  shaderColorPreview,
  shaderSliderRange,
} from "../../src/components/settings/shaders/shaderControls";

const qt = vm.runInNewContext(
  readFileSync(
    resolve(
      __dirname,
      "../../../engine/mako-ui/rsc/dialogs/ShaderControlHelpers.js",
    ),
    "utf8",
  ).replace(".pragma library", "") + "\n({groups,sliderRange})",
);
const both = (parameters: ShaderParameter[]) => {
  const actual = shaderControlGroups(parameters);
  expect(JSON.parse(JSON.stringify(qt.groups(parameters)))).toEqual(actual);
  return actual;
};
function vector(
  count = 3,
  widget = "color",
  type = "float",
): ShaderParameter[] {
  const group = "makoShader_41_54696e74_0";
  return Array.from({ length: count }, (_, component) => ({
    key: group.slice(0, -1) + component,
    label: `Tint [${component + 1}]`,
    description: "Authored help",
    category: "Colour",
    type,
    value: 0.123456789,
    default: 0.1,
    min: null,
    max: null,
    step: null,
    items: [],
    presentation: { widget, group, label: "Tint", component, count },
  })) as ShaderParameter[];
}
test("explicit colour vectors share a control without changing exact values", () => {
  const parameters = vector(4);
  parameters[3].value = 0.42;
  const before = structuredClone(parameters);
  const groups = both(parameters);
  expect(groups).toHaveLength(1);
  expect(groups[0].color).toBe(true);
  expect(shaderColorPreview(parameters)).toBe("rgba(31, 31, 31, 0.42)");
  expect(parameters).toEqual(before);
});
test("names and vector length cannot turn generic controls into colours", () => {
  for (const parameters of [
    vector(3, ""),
    vector(2, "color"),
    vector(4, "color", "int"),
  ]) {
    expect(both(parameters)[0].color).toBe(false);
  }
  const legacy = vector().map(({ presentation: _, ...p }) => p);
  expect(both(legacy)).toHaveLength(3);
});
test("incomplete, reordered, mixed, duplicate and malformed groups fall back to scalar controls", () => {
  const cases = [vector().slice(1), vector().slice(0, 2), vector().reverse()];
  for (const field of ["group", "label", "count", "widget", "component"]) {
    const params = vector();
    (params[1].presentation as any)[field] = "mismatch";
    cases.push(params);
  }
  const duplicate = vector();
  duplicate[1].key = duplicate[0].key;
  cases.push(duplicate);
  const wrongType = vector();
  wrongType[1].type = "int";
  cases.push(wrongType);
  const badLabel = vector();
  (badLabel[0].presentation as any).label = null;
  cases.push(badLabel);
  for (const parameters of cases)
    expect(
      both(parameters).every((g) => g.parameters.length === 1 && !g.color),
    ).toBe(true);
});
test("arrays and matrices with scalar presentation stay numbered", () => {
  const parameters = vector(4).map((p) => ({
    ...p,
    presentation: {
      ...p.presentation!,
      group: p.key,
      label: p.label,
      count: 1,
      component: 0,
    },
  }));
  expect(both(parameters).map((g) => g.label)).toEqual(
    parameters.map((p) => p.label),
  );
});
test("input hints and declared ranges are honoured; colour preview does not clamp stored HDR values", () => {
  const p = vector()[0];
  for (const color of [false, true]) {
    expect(qt.sliderRange(p, color)).toEqual(shaderSliderRange(p, color));
  }
  p.min = -2;
  p.max = 4;
  p.value = 2;
  expect(qt.sliderRange(p, true)).toEqual(shaderSliderRange(p, true));
  p.presentation!.widget = "input";
  expect(shaderSliderRange(p)).toBeNull();
  expect(qt.sliderRange(p, false)).toBeNull();
  const params = vector();
  params[0].value = 5;
  params[1].value = -1;
  expect(shaderColorPreview(params)).toBe("rgba(255, 0, 31, 1)");
  expect(params[0].value).toBe(5);
  expect(params[1].value).toBe(-1);
});

test("integer sliders never offer fractional or invalid steps and bounds", () => {
  const p = { ...vector(2, "slider", "int")[0], min: 0, max: 10, step: 1 };
  expect(qt.sliderRange(p, false)).toEqual(shaderSliderRange(p));
  expect(shaderSliderRange(p)).toEqual({ min: 0, max: 10, step: 1 });
  for (const change of [
    { step: 0.5 },
    { step: 0 },
    { step: -1 },
    { step: Infinity },
    { min: 0.5 },
    { max: 9.5 },
  ]) {
    const invalid = { ...p, ...change };
    expect(shaderSliderRange(invalid)).toBeNull();
    expect(qt.sliderRange(invalid, false)).toBeNull();
  }
});
test("group hints cannot replace unrelated labels or interpret checkbox lists as axes", () => {
  const parameters = vector();
  parameters[1].label = "Unrelated control";
  expect(both(parameters)).toHaveLength(3);
  expect(both(vector(3, "", "bool"))).toHaveLength(3);
  expect(both(vector(3, "future-widget"))).toHaveLength(3);
});
