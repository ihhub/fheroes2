import {
  ReactNode,
  createContext,
  useContext,
  useEffect,
  useState,
} from "react";
import {
  AndroidInterface,
  WallpaperMapItem,
  WallpaperSettings,
} from "./global";

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

type BridgeContextValue = {
  settings: WallpaperSettings | undefined;
  mapsList: WallpaperMapItem[];
  ready: boolean;
  androidInterface: AndroidInterface | undefined;
};

const bridgeContext = createContext<BridgeContextValue>({
  settings: undefined,
  mapsList: [],
  ready: false,
  androidInterface: undefined,
});

type Props = {
  children: ReactNode;
};

export const BridgeContextProvider: React.FC<Props> = ({ children }) => {
  const [settings, setSettings] = useState<WallpaperSettings | undefined>(
    undefined
  );
  const [mapsList, setMapsList] = useState<WallpaperMapItem[]>([]);
  const [ready, setReady] = useState(false);
  const [androidInterface, setAndroidInterface] = useState<
    AndroidInterface | undefined
  >(undefined);

  useEffect(() => {
    window.Bridge?.init();

    window.dispatchWebViewEvent = (message) => {
      console.log("dispatchWebViewEvent:", JSON.stringify(message, null, 2));

      if (message.type === "settings") {
        setSettings(message.payload);
      }

      if (message.type === "maps-list") {
        setMapsList(message.payload);
      }
    };

    startApp(() => {
      setReady(true);
      setAndroidInterface(window.Bridge?.interfaces.Android);
    });
  }, []);

  return (
    <bridgeContext.Provider
      value={{
        settings,
        mapsList,
        ready,
        androidInterface,
      }}
    >
      {children}
    </bridgeContext.Provider>
  );
};

export const useBridgeContext = () => {
  return useContext(bridgeContext);
};
