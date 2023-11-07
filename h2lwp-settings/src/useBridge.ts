import { useEffect, useState } from "react";
import { WallpaperSettings } from "./global";

const isDev = true;

if (isDev) {
  window.Bridge = {
    init: () => null,
    initialized: true,
    interfaces: {
      // @ts-ignore
      Android: {
        getMapsList() {
          return Promise.resolve(
            JSON.stringify(
              [
                {
                  name: "pol_2nde Jeunesse.mp2",
                  title: "2nde Jeunesse",
                  width: 108,
                  height: 108,
                  isPoL: true,
                },
                {
                  name: "pol_4 mal Blutzeit.mp2",
                  title: "4 mal Blutzeit",
                  width: 144,
                  height: 144,
                  isPoL: true,
                },
                {
                  name: "pol_4 Tears.mp2",
                  title: "4 Tears",
                  width: 144,
                  height: 144,
                  isPoL: true,
                },
                {
                  name: "pol_5th Horseman.mp2",
                  title: "5th Horseman",
                  width: 144,
                  height: 144,
                  isPoL: true,
                },
                {
                  name: "pol_100 Years War.mp2",
                  title: "100 Years War",
                  width: 144,
                  height: 144,
                  isPoL: true,
                },
                {
                  name: "pol_A Grhag Keep.mp2",
                  title: "A'Grhag Keep",
                  width: 36,
                  height: 36,
                  isPoL: true,
                },
                {
                  name: "pol_Abode of Evil.mp2",
                  title: "Abode of Evil",
                  width: 72,
                  height: 72,
                  isPoL: true,
                },
                {
                  name: "sw_01_Dragons.mp2",
                  title: "01\\ Драконы\\",
                  width: 36,
                  height: 36,
                  isPoL: true,
                },
                {
                  name: "sw_01_Eren.mp2",
                  title: "Эрен 01\\",
                  width: 72,
                  height: 72,
                  isPoL: true,
                },
                {
                  name: "sw_2 Kingdoms.mp2",
                  title: "2 Kingdoms",
                  width: 36,
                  height: 36,
                  isPoL: false,
                },
                {
                  name: "sw_3 Continents.mp2",
                  title: "3 Continents (M)",
                  width: 144,
                  height: 144,
                  isPoL: false,
                },
                {
                  name: "sw_3-way Cross.mp2",
                  title: "3-way cross",
                  width: 72,
                  height: 72,
                  isPoL: false,
                },
                {
                  name: "sw_4 Warlords.mp2",
                  title: "4 Warlords",
                  width: 108,
                  height: 108,
                  isPoL: false,
                },
                {
                  name: "sw_1000 years.mp2",
                  title: "1000 лет",
                  width: 36,
                  height: 36,
                  isPoL: false,
                },
                {
                  name: "sw_unknown.mp2",
                  title: "!",
                  width: 144,
                  height: 144,
                  isPoL: false,
                },
              ],
              null,
              2
            )
          );
        },
      },
    },
  };

  setTimeout(() => {
    window.dispatchWebViewEvent({
      type: "settings",
      payload: {
        brightness: 0,
        mapUpdateInterval: 0,
        scale: 0,
        scaleType: 0,
        useScroll: false,
      },
    });
  }, 1000);
}

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
