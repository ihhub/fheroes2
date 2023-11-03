declare global {
  interface Window {
    Android: {
      helloFullSync: (name: string) => string;
      helloWebPromise: (name: string) => Promise<string>;
      helloFullPromise: (name: string) => Promise<string>;

      setBrightness: (value: number) => void;
      setScale: (value: number) => void;
      setScaleType: (value: "nearest" | "linear") => void;
      toggleUseScroll: () => void;
    };

    Bridge: {
      initialized: boolean;
      afterInitialize: () => void;
      init: () => void;
      interfaces: Record<string, unknown>;
    };
  }
}

export {};
