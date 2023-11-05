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

export const useBridge = () => {
  const [settings, setSettings] = useState<WallpaperSettings | null>(null);

  useEffect(() => {
    window.Bridge?.init();

    window.dispatchWebViewEvent = (message) => {
      console.log("dispatchWebViewEvent:", JSON.stringify(message, null, 2));

      if (message.type === "settings") {
        setSettings(message.payload);
      }
    };

    startApp(() => {
      window.Android = window.Bridge?.interfaces.Android;

      console.log("getMapsList start")
      window.Android?.getMapsList()
        .then((result) => console.log("getMapsList", result))
        .catch((e) => console.log("getMapsList err", e));
    });
  }, []);

  return { settings };
};
