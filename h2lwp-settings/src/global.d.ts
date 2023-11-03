declare global {
  interface Window {
    Android: {
      helloFullSync: (name: string) => string;
      helloWebPromise: (name: string) => Promise<string>;
      helloFullPromise: (name: string) => Promise<string>;
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
