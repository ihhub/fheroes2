import { useEffect, useState } from "react";
import { WallpaperSettings } from "./global";

function startApp(callback: () => void) {
  if (!window.Bridge) {
    console.log("Bridge doesnt exists");
    return;
  }

  if (window.Bridge.initialized) {
    callback();
  } else {
    window.Bridge.afterInitialize = callback;
  }
}

// const initialState = {
//   brightness: 0,
//   mapUpdateInterval: 0,
//   scale: 0,
//   scaleType: 0,
//   useScroll: false,
// };

export const useBridge = () => {
  const [settings, setSettings] = useState<WallpaperSettings | null>(null);
  const [ready, setReady] = useState(false);

  useEffect(() => {
    window.Bridge?.init();

    window.dispatchWebViewEvent = (message) => {
      console.log("dispatchWebViewEvent:", JSON.stringify(message, null, 2));

      if (message.type === "settings") {
        setSettings(message.payload);
      }
    };

    startApp(() => {
      setReady(true);
      window.Android = window.Bridge?.interfaces.Android;
    });
  }, []);

  return { settings, ready };
};
