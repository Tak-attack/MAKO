import React from "react";
import {
  act,
  cleanup,
  fireEvent,
  render,
  screen,
} from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { GamepadButton, type GamepadEvent } from "@decky/ui";

const navigation = vi.hoisted(() => ({
  virtualFocus: false,
  handlesAvailable: true,
  canTakeFocus: true,
  focused: null as HTMLElement | null,
  takeFocus: vi.fn(),
}));

vi.mock("@decky/ui", () => ({
  GamepadButton: { DIR_UP: 9, DIR_DOWN: 10, DIR_LEFT: 11, DIR_RIGHT: 12 },
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
    onKeyDown,
    onButtonDown,
    onGamepadFocus,
    onGamepadBlur,
    navRef,
    noFocusRing: _noFocusRing,
    "flow-children": flowChildren,
    ...props
  }: React.HTMLAttributes<HTMLDivElement> & {
    onActivate?: () => void;
    onButtonDown?: (event: GamepadEvent) => void;
    onGamepadFocus?: () => void;
    onGamepadBlur?: () => void;
    navRef?: React.Ref<{ TakeFocus: (button?: GamepadButton) => boolean }>;
    noFocusRing?: boolean;
    "flow-children"?: string;
  }) => {
    const ref = React.useRef<HTMLDivElement>(null);
    React.useImperativeHandle(
      navigation.handlesAvailable ? navRef : undefined,
      () => ({
        TakeFocus(button) {
          const element = ref.current!;
          navigation.takeFocus(element, button);
          if (!navigation.canTakeFocus || element.closest("[hidden]"))
            return false;
          const previous = navigation.focused;
          navigation.focused = element;
          previous?.dispatchEvent(new CustomEvent("vgp_onblur"));
          element.dispatchEvent(new CustomEvent("vgp_onfocus"));
          if (!navigation.virtualFocus) element.focus();
          return true;
        },
      }),
    );
    React.useEffect(() => {
      const element = ref.current!;
      const handle = (event: Event) => onButtonDown?.(event as GamepadEvent);
      const activate = () => onActivate?.();
      const focus = () => onGamepadFocus?.();
      const blur = () => onGamepadBlur?.();
      element.addEventListener("vgp_onbuttondown", handle);
      element.addEventListener("vgp_onok", activate);
      element.addEventListener("vgp_onfocus", focus);
      element.addEventListener("vgp_onblur", blur);
      return () => {
        element.removeEventListener("vgp_onbuttondown", handle);
        element.removeEventListener("vgp_onok", activate);
        element.removeEventListener("vgp_onfocus", focus);
        element.removeEventListener("vgp_onblur", blur);
      };
    }, [onActivate, onButtonDown, onGamepadFocus, onGamepadBlur]);
    return (
      <div
        ref={ref}
        tabIndex={navigation.virtualFocus ? undefined : 0}
        data-focus-flow={flowChildren}
        {...props}
        onKeyDown={(event) => {
          onKeyDown?.(event);
          if (!event.defaultPrevented && event.key === "Enter") onActivate?.();
        }}
      >
        {children}
      </div>
    );
  },
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
import { ModalityTabs } from "../../src/components/ModalityTabs";
import { getDefaults } from "../../src/config/configSchema";
import { EMPTY_RUNTIME_SCALING_UI_STATE } from "../../src/utils/runtimeScalingUtils";

beforeEach(() => {
  localStorage.clear();
  window.SP_REACT = React;
  navigation.virtualFocus = false;
  navigation.handlesAvailable = true;
  navigation.canTakeFocus = true;
  navigation.focused = null;
  navigation.takeFocus.mockClear();
});

afterEach(cleanup);

test.each([
  ["ArrowLeft", GamepadButton.DIR_LEFT, [null, 0, 1, null, 3, 4]],
  ["ArrowRight", GamepadButton.DIR_RIGHT, [1, 2, null, 4, 5, null]],
  ["ArrowUp", GamepadButton.DIR_UP, [null, null, null, 0, 1, 2]],
  ["ArrowDown", GamepadButton.DIR_DOWN, [3, 4, 5, null, null, null]],
] as const)(
  "%s moves within rows/columns and leaves grid exits to Steam",
  (key, button, destinations) => {
    window.SP_REACT = React;
    const select = vi.fn();
    const { rerender } = render(
      <ModalityTabs
        activeModality="frame-generation"
        onModalityChange={select}
      />,
    );
    for (const mode of ["keyboard", "gamepad", "virtual"] as const) {
      const gamepad = mode !== "keyboard";
      navigation.virtualFocus = mode === "virtual";
      rerender(
        <ModalityTabs
          activeModality="frame-generation"
          onModalityChange={select}
        />,
      );
      const tabs = screen.getAllByRole("tab");
      destinations.forEach((destination, index) => {
        if (gamepad) {
          navigation.focused = tabs[index];
          fireEvent(tabs[index], new CustomEvent("vgp_onfocus"));
        } else {
          act(() => tabs[index].focus());
        }
        const event = gamepad
          ? new CustomEvent("vgp_onbuttondown", {
              bubbles: true,
              cancelable: true,
              detail: { button, source: 0 },
            })
          : new KeyboardEvent("keydown", {
              key,
              bubbles: true,
              cancelable: true,
            });
        fireEvent(tabs[index], event);
        if (gamepad) {
          expect(navigation.focused).toBe(tabs[destination ?? index]);
        } else {
          expect(document.activeElement).toBe(tabs[destination ?? index]);
        }
        expect(event.defaultPrevented).toBe(destination !== null);
      });
    }
    expect(select).not.toHaveBeenCalled();
  },
);

test("held direction continues navigation while activation remains explicit", () => {
  window.SP_REACT = React;
  const select = vi.fn();
  render(
    <ModalityTabs
      activeModality="frame-generation"
      onModalityChange={select}
    />,
  );
  const tabs = screen.getAllByRole("tab");
  act(() => tabs[0].focus());
  for (let index = 0; index < 2; index++) {
    fireEvent(
      tabs[index],
      new CustomEvent("vgp_onbuttondown", {
        bubbles: true,
        cancelable: true,
        detail: {
          button: GamepadButton.DIR_RIGHT,
          is_repeat: index > 0,
          source: 0,
        },
      }),
    );
    expect(document.activeElement).toBe(tabs[index + 1]);
  }
  expect(select).not.toHaveBeenCalled();
  fireEvent.keyDown(tabs[2], { key: "Enter" });
  expect(select).toHaveBeenCalledExactlyOnceWith("shaders");
});

test("controller navigation moves virtual focus when browser focus cannot move", () => {
  navigation.virtualFocus = true;
  const select = vi.fn();
  render(
    <ModalityTabs activeModality="frame-generation" onModalityChange={select} />,
  );
  const tabs = screen.getAllByRole("tab");
  navigation.focused = tabs[0];
  fireEvent(tabs[0], new CustomEvent("vgp_onfocus"));
  const browserFocus = document.activeElement;
  // Steam's in-game tree uses virtual focus: these divs have no DOM tabIndex.
  tabs[1].focus();
  expect(document.activeElement).toBe(browserFocus);

  for (const [button, destination] of [
    [GamepadButton.DIR_RIGHT, 1],
    [GamepadButton.DIR_DOWN, 4],
    [GamepadButton.DIR_LEFT, 3],
    [GamepadButton.DIR_UP, 0],
    [GamepadButton.DIR_DOWN, 3],
  ] as const) {
    const event = new CustomEvent("vgp_onbuttondown", {
      bubbles: true,
      cancelable: true,
      detail: { button, source: 0 },
    });
    fireEvent(navigation.focused!, event);
    expect(navigation.focused).toBe(tabs[destination]);
    expect(navigation.takeFocus).toHaveBeenLastCalledWith(
      tabs[destination],
      button,
    );
    expect(event.defaultPrevented).toBe(true);
    expect(document.activeElement).toBe(browserFocus);
  }
  expect(select).not.toHaveBeenCalled();
  fireEvent(navigation.focused!, new CustomEvent("vgp_onok"));
  expect(select).toHaveBeenCalledExactlyOnceWith("performance");
});

test.each(["missing", "rejected"] as const)(
  "does not swallow gamepad directions when Steam navigation is %s",
  (availability) => {
    navigation.virtualFocus = true;
    navigation.handlesAvailable = availability !== "missing";
    navigation.canTakeFocus = availability !== "rejected";
    const select = vi.fn();
    render(
      <ModalityTabs activeModality="frame-generation" onModalityChange={select} />,
    );
    const tab = screen.getByRole("tab", { name: "Frame-gen" });
    navigation.focused = tab;
    const steamNavigation = vi.fn();
    screen
      .getByRole("tablist")
      .addEventListener("vgp_onbuttondown", steamNavigation);
    const event = new CustomEvent("vgp_onbuttondown", {
      bubbles: true,
      cancelable: true,
      detail: { button: GamepadButton.DIR_RIGHT, source: 0 },
    });
    fireEvent(tab, event);
    expect(event.defaultPrevented).toBe(false);
    expect(steamNavigation).toHaveBeenCalledOnce();
    expect(navigation.focused).toBe(tab);
    expect(select).not.toHaveBeenCalled();
  },
);

test("keyboard navigation works without a Steam tree", () => {
  navigation.handlesAvailable = false;
  render(
    <ModalityTabs activeModality="frame-generation" onModalityChange={vi.fn()} />,
  );
  const first = screen.getByRole("tab", { name: "Frame-gen" });
  act(() => first.focus());
  fireEvent.keyDown(first, { key: "ArrowRight" });
  expect(document.activeElement).toBe(
    screen.getByRole("tab", { name: "Scaling" }),
  );
});

test("virtual tab focus survives a profile reload and runtime panel insertion", () => {
  navigation.virtualFocus = true;
  const change = vi.fn(async () => undefined);
  const settings = (running: boolean, loading: boolean) => (
    <>
      {running && <div>Live Status</div>}
      <div hidden={loading}>
        <FeatureSettings
          config={getDefaults()}
          runtimeState={EMPTY_RUNTIME_SCALING_UI_STATE}
          profileName={running ? "running-game" : "mako"}
          vkBasaltConfigPath="/home/deck/.config/vkBasalt/vkBasalt.conf"
          onConfigChange={change}
          onConfigUpdate={change}
        />
      </div>
    </>
  );
  const { rerender } = render(settings(false, false));
  const frameGeneration = screen.getByRole("tab", { name: "Frame-gen" });
  navigation.focused = frameGeneration;
  fireEvent(frameGeneration, new CustomEvent("vgp_onfocus"));
  rerender(settings(true, true));
  rerender(settings(true, false));

  fireEvent(
    navigation.focused!,
    new CustomEvent("vgp_onbuttondown", {
      bubbles: true,
      cancelable: true,
      detail: { button: GamepadButton.DIR_DOWN, source: 0 },
    }),
  );
  expect(navigation.focused).toBe(
    screen.getByRole("tab", { name: "Performance" }),
  );
  fireEvent(navigation.focused!, new CustomEvent("vgp_onok"));
  expect(screen.getByText("FG performance controls")).toBeTruthy();
  expect(localStorage.getItem("mako-active-modality")).toBe("performance");
  expect(change).not.toHaveBeenCalled();
});

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

    const sectionHeading = screen.getByRole("heading", {
      name: "Image Processing",
    });
    expect(screen.getAllByRole("heading")).toHaveLength(1);
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
    expect(
      sectionHeading.compareDocumentPosition(tablist) &
        Node.DOCUMENT_POSITION_FOLLOWING,
    ).not.toBe(0);
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
    expect(screen.getAllByRole("heading")).toEqual([sectionHeading]);
  });
});

describe("tab selection persistence", () => {
  const settings = (profileName = "mako") => (
    <FeatureSettings
      config={getDefaults()}
      runtimeState={EMPTY_RUNTIME_SCALING_UI_STATE}
      profileName={profileName}
      vkBasaltConfigPath="/home/deck/.config/vkBasalt/vkBasalt.conf"
      onConfigChange={vi.fn(async () => undefined)}
      onConfigUpdate={vi.fn(async () => undefined)}
    />
  );

  test.each([
    "Frame-gen",
    "Scaling",
    "Shaders",
    "Performance",
    "Advanced",
    "Tweaks",
  ])("remembers %s after reopening and across profiles", (name) => {
    const initial = render(settings());
    fireEvent.click(screen.getByRole("tab", { name: "Performance" }));
    fireEvent.keyDown(screen.getByRole("tab", { name }), { key: "Enter" });
    initial.rerender(settings("another-game"));
    expect(
      screen.getByRole("tab", { selected: true }).getAttribute("aria-label"),
    ).toBe(name);
    initial.unmount();

    render(settings("another-game"));
    const selected = screen.getByRole("tab", { selected: true });
    expect(selected.getAttribute("aria-label")).toBe(name);
    expect(selected.getAttribute("aria-controls")).toBe(
      screen.getByRole("tabpanel").id,
    );
  });

  test("moving focus does not replace the remembered selection", () => {
    const initial = render(settings());
    const shaders = screen.getByRole("tab", { name: "Shaders" });
    fireEvent.click(shaders);
    act(() => shaders.focus());
    fireEvent.keyDown(shaders, { key: "ArrowDown" });
    expect(document.activeElement).toBe(
      screen.getByRole("tab", { name: "Tweaks" }),
    );
    initial.unmount();

    render(settings());
    expect(
      screen.getByRole("tab", { name: "Shaders", selected: true }),
    ).toBeTruthy();
  });

  test.each(["", "removed-tab", "null", "{}", "toString", "__proto__"])(
    "falls back to Frame-gen for an invalid stored tab: %s",
    (value) => {
      localStorage.setItem("mako-active-modality", value);
      render(settings());
      expect(
        screen.getByRole("tab", { name: "Frame-gen", selected: true }),
      ).toBeTruthy();
      fireEvent.click(screen.getByRole("tab", { name: "Scaling" }));
      expect(
        screen.getByRole("tab", { name: "Scaling", selected: true }),
      ).toBeTruthy();
    },
  );

  test("settings remain usable when browser storage is unavailable", () => {
    vi.spyOn(Storage.prototype, "getItem").mockImplementation(() => {
      throw new Error("storage unavailable");
    });
    vi.spyOn(Storage.prototype, "setItem").mockImplementation(() => {
      throw new Error("storage unavailable");
    });
    vi.spyOn(console, "warn").mockImplementation(() => {});
    const props = settings().props;
    render(<FeatureSettings {...props} />);
    expect(
      screen.getByRole("tab", { name: "Frame-gen", selected: true }),
    ).toBeTruthy();
    fireEvent.click(screen.getByRole("tab", { name: "Shaders" }));
    expect(screen.getByText("Shader controls")).toBeTruthy();
    expect(props.onConfigChange).not.toHaveBeenCalled();
    expect(props.onConfigUpdate).not.toHaveBeenCalled();
  });
});
