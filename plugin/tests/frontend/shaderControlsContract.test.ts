import { readFileSync } from "node:fs";
import { resolve } from "node:path";
import vm from "node:vm";
import { expect, test } from "vitest";
import type { ShaderParameter } from "../../src/api/makoApi";
import {
  shaderControlGroups,
  shaderControlSections,
  shaderColorPreview,
  shaderSliderRange,
  shaderNumericValue,
  shaderSliderValue,
} from "../../src/components/settings/shaders/shaderControls";

const qt = vm.runInNewContext(
  readFileSync(
    resolve(
      __dirname,
      "../../../engine/mako-ui/rsc/dialogs/ShaderControlHelpers.js",
    ),
    "utf8",
  ).replace(".pragma library", "") +
    "\n({groups,sections,sliderRange,numericValue,sliderValue})",
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
test("Decky and Qt compact only consecutive declared colours in the same category", () => {
  const colour = (name: string, category = "Colour") => vector().map((p, component) => {
    const group = `makoShader_41_${Buffer.from(name).toString("hex")}_0`;
    return { ...p, category, key: group.slice(0, -1) + component,
      label: `${name} [${component + 1}]`,
      presentation: { ...p.presentation!, group, label: name } };
  });
  const parameters = [
    ...colour("First"), ...colour("Second"),
    ...vector(2, "drag"),
    ...colour("Third"), ...colour("Fourth", "Other"),
    ...colour("Legacy").map(({ presentation: _, ...p }) => p),
  ];
  const sections = shaderControlSections(parameters);
  expect(JSON.parse(JSON.stringify(qt.sections(parameters)))).toEqual(sections);
  expect(sections.map((section) => section.map((group) => group.label))).toEqual([
    ["First", "Second"], ["Tint"], ["Third"], ["Fourth"],
    ["Legacy [1]"], ["Legacy [2]"], ["Legacy [3]"],
  ]);
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

test.each([
  [0.5, 2, 0.01],
  [0, 1, 0.01],
  [0, 0.25, 0.001],
  [0.75, 1, 0.001],
  [-1, 1, 0.01],
  [0, 49, 0.1],
  [0, 1500, 10],
  [0, 0.00015, 0.000001],
  [0.2, 0.3, 0.001],
])(
  "fallback slider %s–%s uses a decimal step of %s in both editors",
  (min, max, step) => {
    const p = { ...vector()[0], min, max, step: null };
    expect(shaderSliderRange(p)).toEqual({ min, max, step });
    expect(qt.sliderRange(p, false)).toEqual(shaderSliderRange(p));
  },
);

test("authored fractional steps and boundaries retain their FP32 meaning", () => {
  const p = { ...vector()[0], min: 0, max: 0.980000019, step: 0.100000001 };
  expect(shaderSliderRange(p)).toEqual({ min: 0, max: 0.98, step: 0.1 });
  expect(qt.sliderRange(p, false)).toEqual(shaderSliderRange(p));
  p.step = 0.015;
  expect(shaderSliderRange(p)?.step).toBe(0.015);
  expect(qt.sliderRange(p, false)).toEqual(shaderSliderRange(p));
});

test.each([
  [1.05499995, 1.055],
  [1.04999995, 1.05],
  [0.980000019, 0.98],
  [3.4000001, 3.4],
  [-0.100000001, -0.1],
  [0.100000009, 0.10000001],
  [0.123456789, 0.123456789],
  [0.345678912, 0.345678912],
  [1e-12, 1e-12],
  [0, 0],
  [-0, -0],
  [4.444445, 4.444445],
])(
  "numeric readback %s displays as %s without losing shader precision",
  (value, expected) => {
    expect(shaderNumericValue(value, "float")).toBe(expected);
    expect(qt.numericValue(value, "float")).toBe(expected);
    expect(Math.fround(expected)).toBe(Math.fround(value));
  },
);

test("normalization preserves FP32 bits across magnitudes, signs and subnormals", () => {
  const bytes = new ArrayBuffer(4);
  const bits = new Uint32Array(bytes);
  const floats = new Float32Array(bytes);
  const seeds = [1, 0x007fffff, 0x00800000, 0x3f800001, 0x7f7fffff];
  for (let i = 1; i <= 1000; i++) seeds.push(Math.imul(i, 2654435761) >>> 0);
  for (const seed of seeds) {
    bits[0] = seed;
    const value = floats[0];
    if (!Number.isFinite(value)) continue;
    const readback = Number(value.toPrecision(9));
    const normalized = shaderNumericValue(readback, "float");
    expect(Math.fround(normalized)).toBe(value);
    expect(qt.numericValue(readback, "float")).toBe(normalized);
    expect(shaderNumericValue(normalized, "float")).toBe(normalized);
  }
  for (const value of [2147483647, -2147483648, 4294967295]) {
    expect(shaderNumericValue(value, "int")).toBe(value);
    expect(qt.numericValue(value, "uint")).toBe(value);
  }
});

test("slider movement snaps to declared steps, clamps endpoints and removes arithmetic tails", () => {
  for (const [range, value, expected] of [
    [{ min: 0.5, max: 2, step: 0.01 }, 1.0499999999999998, 1.05],
    [{ min: 0.5, max: 2, step: 0.01 }, 1.063, 1.06],
    [{ min: 0, max: 49, step: 0.1 }, 3.1000000000000005, 3.1],
    [{ min: -1, max: 1, step: 0.01 }, -0.29999999999999993, -0.3],
    [{ min: 0.5, max: 2, step: 0.015 }, 1.054, 1.055],
    [{ min: 0, max: 0.98, step: 0.1 }, 0.98, 0.98],
    [{ min: 0, max: 1, step: 0.01 }, 2, 1],
    [{ min: 0, max: 1, step: 0.01 }, -1, 0],
    [{ min: 0, max: 10, step: 1 }, 4.444445, 4],
  ] as const) {
    expect(shaderSliderValue(value, range)).toBe(expected);
    expect(qt.sliderValue(value, range)).toBe(expected);
  }
});
