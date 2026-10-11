import React from "react";
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import { afterEach, describe, expect, test, vi } from "vitest";

vi.mock("@decky/ui", () => ({
  PanelSectionRow: ({ children }: { children: React.ReactNode }) => (
    <div>{children}</div>
  ),
}));
vi.mock("../../src/components/MakoUi", () => ({
  MakoSectionHeader: ({
    children,
    description,
  }: {
    children: React.ReactNode;
    description?: React.ReactNode;
  }) => (
    <div>
      <h2>{children}</h2>
      {description}
    </div>
  ),
  MakoInlineTip: ({
    children,
    tone,
  }: {
    children: React.ReactNode;
    tone?: string;
  }) => (
    <div role="note" data-tone={tone}>
      {children}
    </div>
  ),
  MakoFocusable: ({
    children,
    onActivate,
    onGamepadFocus: _onGamepadFocus,
    onGamepadBlur: _onGamepadBlur,
    noFocusRing: _noFocusRing,
    "flow-children": flowChildren,
    ...props
  }: React.HTMLAttributes<HTMLDivElement> & {
    onActivate?: () => void;
    onGamepadFocus?: () => void;
    onGamepadBlur?: () => void;
    noFocusRing?: boolean;
    "flow-children"?: string;
  }) => (
    <div
      tabIndex={0}
      data-focus-flow={flowChildren}
      {...props}
      onKeyDown={(event) => {
        if (event.key === "Enter") onActivate?.();
      }}
    >
      {children}
    </div>
  ),
  makoDialogButtonStyle: () => ({}),
}));
vi.mock("../../src/components/FpsMultiplierControl", () => ({
  FpsMultiplierControl: () => <div>Frame Generation controls</div>,
}));
vi.mock("../../src/components/ScalingControl", () => ({
  ScalingControl: () => <div>Upscaling controls</div>,
}));
vi.mock("../../src/components/settings/PerformanceConfigurationGroup", () => ({
  PerformanceConfigurationGroup: () => <div>FG performance controls</div>,
}));
vi.mock("../../src/components/settings/ShadersConfigurationGroup", () => ({
  ShadersConfigurationGroup: () => <div>Shader controls</div>,
}));
vi.mock("../../src/components/ConfigurationSection", () => ({
  ConfigurationSection: ({ groups }: { groups: string[] }) => (
    <div>
      {groups.includes("advanced") && <div>FG advanced controls</div>}
      {groups.includes("compatibility") && <div>Compatibility controls</div>}
    </div>
  ),
}));
vi.mock("../../src/i18n/i18n", () => ({
  default: (_key: string, fallback: string) => fallback,
}));

import { FeatureSettings } from "../../src/components/FeatureSettings";
import { getDefaults } from "../../src/config/configSchema";
import { EMPTY_RUNTIME_SCALING_UI_STATE } from "../../src/utils/runtimeScalingUtils";

afterEach(cleanup);

describe("primary feature organization", () => {
  test("opens Frame-gen by default and switches six settings tabs in a three-column grid", () => {
    window.SP_REACT = React;
    const { container } = render(
      <FeatureSettings
        config={getDefaults()}
        runtimeState={EMPTY_RUNTIME_SCALING_UI_STATE}
        profileName="mako"
        vkBasaltConfigPath="/home/deck/.config/vkBasalt/vkBasalt.conf"
        onConfigChange={vi.fn(async () => undefined)}
        onConfigUpdate={vi.fn(async () => undefined)}
      />,
    );

    expect(screen.queryByRole("heading")).toBeNull();
    const sharedPerformanceInfo = screen.getByText(
      "Combining Frame Generation, Scaling, and Shaders may cost performance. Disable unused features; changing display mode can change input resolution and GPU cost.",
    );
    expect(sharedPerformanceInfo.closest('[data-tone="info"]')).toBeTruthy();
    expect(
      (
        sharedPerformanceInfo.closest(
          '[data-mako-image-processing-tip="true"]',
        ) as HTMLElement
      ).style.marginTop,
    ).toBe("3px");
    expect(
      screen.getByRole("tablist", { name: "Image Processing" }),
    ).toBeTruthy();
    expect(
      sharedPerformanceInfo.compareDocumentPosition(
        screen.getByRole("tablist", { name: "Image Processing" }),
      ) & Node.DOCUMENT_POSITION_FOLLOWING,
    ).not.toBe(0);
    expect(
      screen.getAllByRole("tab").map((tab) => tab.getAttribute("aria-label")),
    ).toEqual([
      "Frame-gen",
      "Scaling",
      "Shaders",
      "Performance",
      "Advanced",
      "Tweaks",
    ]);
    const tablist = screen.getByRole("tablist", { name: "Image Processing" });
    expect(tablist.style.gridTemplateColumns).toBe("repeat(3, minmax(0, 1fr))");
    expect(tablist.getAttribute("data-focus-flow")).toBe("grid");
    expect(
      screen
        .getByRole("tab", { name: "Frame-gen" })
        .getAttribute("aria-selected"),
    ).toBe("true");
    expect(
      screen.getByRole("tabpanel", { name: "Frame Generation" }),
    ).toBeTruthy();
    expect(screen.getByText("Frame Generation controls")).toBeTruthy();
    expect(screen.queryByText("FG performance controls")).toBeNull();

    fireEvent.click(screen.getByRole("tab", { name: "Performance" }));
    expect(screen.getByText("FG performance controls")).toBeTruthy();
    expect(screen.queryByText("Frame Generation controls")).toBeNull();
    expect(screen.queryByText("FG advanced controls")).toBeNull();
    expect(screen.queryByText("Compatibility controls")).toBeNull();

    fireEvent.click(screen.getByRole("tab", { name: "Frame-gen" }));
    expect(
      screen
        .getByRole("tab", { name: "Frame-gen" })
        .getAttribute("aria-selected"),
    ).toBe("true");
    expect(screen.getByRole("tab", { name: "Frame-gen" }).style.height).toBe(
      "46px",
    );
    expect(screen.getByRole("tab", { name: "Frame-gen" }).style.boxSizing).toBe(
      "border-box",
    );
    expect(
      container.querySelector('[data-mako-modality-ribbon="frame-generation"]')
        ?.textContent,
    ).toBe("Frame-gen");
    expect(
      (
        container.querySelector(
          '[data-mako-modality-selector="true"]',
        ) as HTMLElement
      ).style.margin,
    ).toBe("14px 0px 12px");
    const frameGenerationIcon = screen
      .getByRole("tab", { name: "Frame-gen" })
      .querySelector("svg") as SVGElement;
    expect(frameGenerationIcon.style.transform).toBe("translateY(0) scale(1)");
    expect(screen.getByText("Frame Generation controls")).toBeTruthy();
    expect(screen.queryByText("FG performance controls")).toBeNull();
    expect(screen.queryByText("FG advanced controls")).toBeNull();
    expect(screen.queryByText("Upscaling controls")).toBeNull();
    expect(screen.queryByText("Shader controls")).toBeNull();

    const spatialTab = screen.getByRole("tab", { name: "Scaling" });
    const spatialRibbon = container.querySelector(
      '[data-mako-modality-ribbon="spatial"]',
    ) as HTMLElement;
    expect(spatialRibbon.style.opacity).toBe("0");
    fireEvent.mouseEnter(spatialTab);
    expect(spatialRibbon.style.opacity).toBe("1");
    expect(
      (spatialTab.querySelector("svg") as SVGElement).style.transform,
    ).toBe("translateY(-5px) scale(0.94)");
    fireEvent.mouseLeave(spatialTab);
    expect(spatialRibbon.style.opacity).toBe("0");
    fireEvent.focus(spatialTab);
    expect(spatialRibbon.style.opacity).toBe("1");
    fireEvent.blur(spatialTab);
    expect(spatialRibbon.style.opacity).toBe("0");

    fireEvent.click(spatialTab);
    expect(screen.queryByText("Frame Generation controls")).toBeNull();
    expect(screen.getByText("Upscaling controls")).toBeTruthy();
    expect(screen.queryByText("Shader controls")).toBeNull();
    expect(spatialTab.getAttribute("aria-selected")).toBe("true");
    expect(screen.queryByText("FG performance controls")).toBeNull();
    expect(screen.queryByText("FG advanced controls")).toBeNull();

    const shadersTab = screen.getByRole("tab", { name: "Shaders" });
    fireEvent.click(shadersTab);
    expect(screen.queryByText("Frame Generation controls")).toBeNull();
    expect(screen.queryByText("Upscaling controls")).toBeNull();
    expect(screen.getByText("Shader controls")).toBeTruthy();
    expect(shadersTab.getAttribute("aria-selected")).toBe("true");

    const modalityPanel = container.querySelector(
      '[data-mako-modality-panel="shaders"]',
    ) as HTMLElement;
    expect(modalityPanel.style.marginTop).toBe("4px");
    const advancedTab = screen.getByRole("tab", { name: "Advanced" });
    fireEvent.keyDown(advancedTab, { key: "Enter" });
    expect(screen.getByText("FG advanced controls")).toBeTruthy();
    expect(screen.queryByText("Shader controls")).toBeNull();
    expect(screen.queryByText("FG performance controls")).toBeNull();
    expect(advancedTab.getAttribute("aria-selected")).toBe("true");

    const compatibilityTab = screen.getByRole("tab", { name: "Tweaks" });
    fireEvent.click(compatibilityTab);
    expect(screen.getByText("Compatibility controls")).toBeTruthy();
    expect(screen.queryByText("FG advanced controls")).toBeNull();
    expect(compatibilityTab.getAttribute("aria-controls")).toBe(
      screen.getByRole("tabpanel").id,
    );

    fireEvent.click(screen.getByRole("tab", { name: "Performance" }));
    expect(screen.getByText("FG performance controls")).toBeTruthy();
    expect(screen.queryByText("Compatibility controls")).toBeNull();
    expect(screen.getAllByRole("tab", { selected: true })).toHaveLength(1);
  });
});
