import { useEffect, useState } from "react";

function startApp(callback: () => void) {
  if (window.Bridge.initialized) {
    callback();
  } else {
    window.Bridge.afterInitialize = callback;
  }
}

export const useBridge = () => {
  const [state, setState] = useState({});

  useEffect(() => {
    window.Bridge.init();

    startApp(() => {
      // @ts-ignore
      window.Android = window.Bridge.interfaces.Android;

      // @ts-ignore
      window.dispatch = (msg: any) => {
        console.log("Got message:", msg);
        setState(msg);
      };
    });
  }, []);

  return { state };
};
