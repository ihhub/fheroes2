declare global {
  interface Window {
    Android?: {
      helloFullSync: (name: string) => string;
      helloWebPromise: (name: string) => Promise<string>;
      helloFullPromise: (name: string) => Promise<string>;

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

    dispatchWebViewEvent: (message: any) => void;
  }
}

export {};
