import { Suspense, useEffect } from "react";
import logo from "./logo.svg";
import "./App.css";
import { useBridge } from "./useBridge";
// import { suspend } from "suspend-react";

const Child: React.FC<{ state: unknown }> = ({ state }) => {
  // const data = suspend(
  //   async () => window.Android.helloFullPromise("helloFullPromise"),
  //   []
  // );

  useEffect(() => {
    setTimeout(() => {
      console.log("setBrightness");
      window.Android.setBrightness(Math.floor(Math.random() * 100));
    }, 3000);
  }, []);

  return <pre>{JSON.stringify(state, null, 2)}</pre>;
};

export function App() {
  const { state } = useBridge();

  return (
    <div className="App">
      <header className="App-header">
        <img src={logo} className="App-logo" alt="logo" />
        
        <button onClick={() => window.Android.setWallpaper()}>
          Set wallpaper
        </button>

        <Suspense fallback={<div>Loading...</div>}>
          <Child state={state} />
        </Suspense>
      </header>
    </div>
  );
}
