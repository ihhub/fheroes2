import { useEffect, useState } from "react";

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
  const [settings, setSettings] = useState({});

  useEffect(() => {
    window.Bridge?.init();

    window.dispatchWebViewEvent = (message) => {
      console.log("dispatchWebViewEvent:", JSON.stringify(message, null, 2));

      if (message.type === "settings") {
        setSettings(message);
      }
    };

    startApp(() => {
      window.Android = window.Bridge?.interfaces.Android;
    });
  }, []);

  return { settings };
};
