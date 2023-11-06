export enum WallpaperMapUpdateInterval {
  EVERY_SWITCH = 0,
  MINUTES_10 = 1,
  MINUTES_30 = 2,
  HOURS_2 = 3,
  HOURS_24 = 4,
}

export enum WallpaperScaleType {
  NEAREST = 0,
  LINEAR = 1,
}

export enum WallpaperScale {
  DPI = 0,
  X1 = 1,
  X2 = 2,
  X3 = 3,
  X4 = 4,
  X5 = 5,
}

export type WallpaperSettings = {
  scale: WallpaperScale;
  scaleType: WallpaperScaleType;
  mapUpdateInterval: WallpaperMapUpdateInterval;
  useScroll: boolean;
  brightness: number;
};

export type WebViewSettingsEvent = {
  type: "settings";
  payload: WallpaperSettings;
};

export type WallpaperMapItem = {
  name: string;
  title: string;
  width: number;
  height: number;
  isPoL: boolean;
};

export type WebViewMapsListEvent = {
  type: "maps-list";
  payload: WallpaperMapItem[];
};

declare global {
  interface Window {
    Android?: {
      helloFullSync: (name: string) => string;
      helloWebPromise: (name: string) => Promise<string>;
      helloFullPromise: (name: string) => Promise<string>;

      getMapsList: () => Promise<string>;

      setBrightness: (value: number) => void;
      setScale: (value: number) => void;
      setScaleType: (value: number) => void;
      setMapUpdateInterval: (value: number) => void;
      toggleUseScroll: () => void;
      setWallpaper: () => void;
    };

    Bridge?: {
      initialized: boolean;
      afterInitialize: () => void;
      init: () => void;
      interfaces: {
        Android?: Window["Android"];
      };
    };

    dispatchWebViewEvent: (
      message: WebViewSettingsEvent | WebViewMapsListEvent
    ) => void;
  }
}

export {};
