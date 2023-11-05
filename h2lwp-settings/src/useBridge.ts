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
  const [state, setState] = useState({});

  console.log("STATE", JSON.stringify(state, null, 2));

  useEffect(() => {
    window.Bridge?.init();

    window.dispatchWebViewEvent = (message: any) => {
      console.log("dispatchWebViewEvent:", JSON.stringify(message, null, 2));
      setState(message);
    };

    setTimeout(() => {
      setState({
        field: 1
      })
    }, 2000)

    startApp(() => {
      window.Android = window.Bridge?.interfaces.Android;
    });
  }, []);

  return { state };
};
