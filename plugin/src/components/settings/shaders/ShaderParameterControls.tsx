import { useEffect, useMemo, useState } from "react";
import {
  Dropdown,
  Field,
  SliderField,
  TextField,
  ToggleField,
} from "@decky/ui";
import type { ShaderParameter } from "../../../api/makoApi";
import t from "../../../i18n/i18n";
import {
  shaderColorPreview,
  shaderControlGroups,
  shaderSliderRange,
} from "./shaderControls";

function NumericParameter({
  parameter,
  onCommit,
  showLabel = true,
}: {
  parameter: ShaderParameter;
  onCommit: (value: number) => void;
  showLabel?: boolean;
}) {
  const [draft, setDraft] = useState(String(parameter.value));
  useEffect(() => setDraft(String(parameter.value)), [parameter.value]);
  const commit = () => {
    const value = Number(draft);
    if (draft.trim() && Number.isFinite(value) && value !== parameter.value)
      onCommit(value);
    else setDraft(String(parameter.value));
  };
  return (
    <TextField
      label={showLabel ? parameter.label : undefined}
      aria-label={parameter.label}
      value={draft}
      onChange={(e) => setDraft(e.target.value)}
      onBlur={commit}
      onKeyDown={(e) => {
        if (e.key === "Enter") commit();
      }}
    />
  );
}

function ParameterControl({
  parameter: p,
  color,
  onChange,
}: {
  parameter: ShaderParameter;
  color: boolean;
  onChange: (values: Record<string, number>) => void;
}) {
  const set = (value: number) => {
    if (Number.isFinite(value)) onChange({ [p.key]: value });
  };
  if (p.type === "bool")
    return (
      <ToggleField
        label={p.label}
        checked={p.value !== 0}
        onChange={(value) => set(value ? 1 : 0)}
      />
    );
  if (
    (p.type === "int" || p.type === "uint") &&
    p.items.length &&
    Number.isInteger(p.value) &&
    p.value >= 0 &&
    p.value < p.items.length
  )
    return (
      <Field label={p.label} childrenLayout="below">
        <Dropdown
          selectedOption={p.value}
          rgOptions={p.items.map((label, data) => ({
            label: label || String(data),
            data,
          }))}
          onChange={(option) => set(Number(option.data))}
        />
      </Field>
    );
  const range = shaderSliderRange(p, color);
  return (
    <>
      {range && (
        <SliderField
          label={`${p.label}: ${p.type === "float" ? Number(p.value.toPrecision(6)) : p.value}`}
          value={Math.max(range.min, Math.min(range.max, p.value))}
          {...range}
          onChange={set}
        />
      )}
      <NumericParameter parameter={p} onCommit={set} showLabel={!range} />
    </>
  );
}

export function ShaderParameterControls({
  parameters,
  onChange,
}: {
  parameters: ShaderParameter[];
  onChange: (values: Record<string, number>) => void;
}) {
  const groups = useMemo(() => shaderControlGroups(parameters), [parameters]);
  const colorLabels = [
    t("SHADER_EDITOR_RED", "Red"),
    t("SHADER_EDITOR_GREEN", "Green"),
    t("SHADER_EDITOR_BLUE", "Blue"),
    t("SHADER_EDITOR_ALPHA", "Alpha"),
  ];
  return (
    <>
      {groups.map((group, index) => (
        <div key={group.key} style={{ marginTop: 16 }}>
          {group.category && group.category !== groups[index - 1]?.category && (
            <div style={{ color: "#aebac6", margin: "12px 0" }}>
              {group.category}
            </div>
          )}
          {group.parameters.length > 1 && (
            <div style={{ fontWeight: 600, marginBottom: 8 }}>
              {group.label}
            </div>
          )}
          {group.description && (
            <div
              style={{
                color: "#aebac6",
                fontSize: 12,
                lineHeight: "16px",
                marginBottom: 8,
              }}
            >
              {group.description}
            </div>
          )}
          {group.color && (
            <>
              <div
                role="img"
                aria-label={t("SHADER_EDITOR_COLOR_PREVIEW", "Colour preview")}
                style={{
                  height: 32,
                  marginBottom: 8,
                  border: "1px solid #8393a5",
                  background:
                    "repeating-conic-gradient(#55616e 0% 25%, #26313c 0% 50%) 0 / 12px 12px",
                }}
              >
                <div
                  style={{
                    height: "100%",
                    background: shaderColorPreview(group.parameters),
                  }}
                />
              </div>
              {group.parameters.some((p) => p.value < 0 || p.value > 1) && (
                <div
                  style={{ color: "#aebac6", fontSize: 12, marginBottom: 8 }}
                >
                  {t(
                    "SHADER_EDITOR_COLOR_RANGE",
                    "Preview is limited to 0–1. Exact values are preserved.",
                  )}
                </div>
              )}
            </>
          )}
          <div style={{ display: "flex", flexDirection: "column", gap: 16 }}>
            {group.parameters.map((p, component) => (
              <div
                key={p.key}
                className="mako-shader-parameter"
                style={{
                  display: "flex",
                  flexDirection: "column",
                  gap: 10,
                  minWidth: 0,
                }}
              >
                <ParameterControl
                  parameter={{
                    ...p,
                    label:
                      group.parameters.length > 1
                        ? group.color
                          ? colorLabels[component]
                          : ["X", "Y", "Z", "W"][component]
                        : p.label,
                  }}
                  color={group.color}
                  onChange={onChange}
                />
              </div>
            ))}
          </div>
        </div>
      ))}
    </>
  );
}
