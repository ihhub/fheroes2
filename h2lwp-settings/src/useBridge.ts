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
  const [state, setState] = useState({
    scaleType: "NEAREST",
    scale: 4,
    mapUpdateInterval: 2,
    brightness: 75,
    useScroll: true,
  });

  useEffect(() => {
    window.Bridge?.init();

    startApp(() => {
      window.Android = window.Bridge?.interfaces.Android;

      window.dispatchWebViewEvent = (message: any) => {
        console.log("dispatchWebViewEvent:", message);
        setState(message);
      };
    });
  }, []);

  return { state };
};
